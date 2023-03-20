#include "dsoundstream.h"

//============================================================
//
//  sound.c - Win32 implementation of MAME sound routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <mmsystem.h>

#undef WINNT
#include <dsound.h>
#include "dsoundmain.h"
#include "framework.h"
#include "log.h"


//============================================================
//  DEBUGGING
//============================================================

#define LOG_SOUND				0

UINT32				stream_buffer_size;
UINT32				stream_buffer_in;

// descriptors and formats
LPDIRECTSOUNDBUFFER	stream_buffer;
DSBUFFERDESC			stream_desc;
WAVEFORMATEX			stream_format;

// buffer over/underflow counts
int					buffer_underflows;
int					buffer_overflows;

//============================================================
//  PROTOTYPES
//============================================================


//static HRESULT		dsound_create_buffers();
static void			dsound_destroy_buffers();



//============================================================
//  dsound_destroy_buffers
//============================================================

static void dsound_destroy_buffers()
{
	// stop any playback
	if (stream_buffer != NULL)
		IDirectSoundBuffer_Stop(stream_buffer);

	// release the stream buffer
	if (stream_buffer != NULL)
		IDirectSoundBuffer_Release(stream_buffer);
	stream_buffer = NULL;

}


//============================================================
//  sound_exit
//============================================================

void stream_stop()
{
	// kill the buffers and dsound
	dsound_destroy_buffers();

	// print out over/underflow stats
	if (buffer_overflows || buffer_underflows)
		wrlog("Sound: buffer overflows=%d underflows=%d\n", buffer_overflows, buffer_underflows);
}


//============================================================
//  copy_sample_data
//============================================================

static void copy_sample_data(INT16 *data, int bytes_to_copy)
{
	void *buffer1, *buffer2;
	DWORD length1, length2;
	HRESULT result;
	int cur_bytes;

	// attempt to lock the stream buffer
	result = IDirectSoundBuffer_Lock(stream_buffer, stream_buffer_in, bytes_to_copy, &buffer1, &length1, &buffer2, &length2, 0);

	// if we failed, assume it was an underflow (i.e.,
	if (result != DS_OK)
	{
		buffer_underflows++;
		return;
	}

	// adjust the input pointer
	stream_buffer_in = (stream_buffer_in + bytes_to_copy) % stream_buffer_size;

	// copy the first chunk
	cur_bytes = (bytes_to_copy > length1) ? length1 : bytes_to_copy;
	memcpy(buffer1, data, cur_bytes);

	// adjust for the number of bytes
	bytes_to_copy -= cur_bytes;
	data = (INT16 *)((UINT8 *)data + cur_bytes);

	// copy the second chunk
	if (bytes_to_copy != 0)
	{
		cur_bytes = (bytes_to_copy > length2) ? length2 : bytes_to_copy;
		memcpy(buffer2, data, cur_bytes);
	}

	// unlock
	result = IDirectSoundBuffer_Unlock(stream_buffer, buffer1, length1, buffer2, length2);
}


//============================================================
//  osd_update_audio_stream
//============================================================

void osd_update_audio_stream(INT16 *buffer, int samples_this_frame)
{
	int bytes_this_frame = samples_this_frame * stream_format.nBlockAlign;
	DWORD play_position, write_position;
	HRESULT result;

	// if no sound, there is no buffer
	if (stream_buffer == NULL)
		return;

	// determine the current play position
	result = IDirectSoundBuffer_GetCurrentPosition(stream_buffer, &play_position, &write_position);
	if (result == DS_OK)
	{
		DWORD stream_in;

		// normalize the write position so it is always after the play position
		if (write_position < play_position)
			write_position += stream_buffer_size;

		// normalize the stream in position so it is always after the write position
		stream_in = stream_buffer_in;
		if (stream_in < write_position)
			stream_in += stream_buffer_size;

		// now we should have, in order:
		//    <------pp---wp---si--------------->

		// if we're between play and write positions, then bump forward, but only in full chunks
		while (stream_in < write_position)
		{
			if (LOG_SOUND) { wrlog("Underflow: PP=%d  WP=%d(%d)  SI=%d(%d)  BTF=%d\n", (int)play_position, (int)write_position, (int)stream_in, (int)bytes_this_frame); }
			buffer_underflows++;
			stream_in += samples_this_frame;
		}

		// if we're going to overlap the play position, just skip this chunk
		if (stream_in + bytes_this_frame > play_position + stream_buffer_size)
		{
			if (LOG_SOUND) { wrlog("Overflow: PP=%d  WP=%d(%d)  SI=%d(%d)  BTF=%d\n", (int)play_position, (int)write_position, (int)stream_in, (int)bytes_this_frame); }
			buffer_overflows++;
			return;
		}

		// now we know where to copy; let's do it
		stream_buffer_in = stream_in % stream_buffer_size;
		copy_sample_data(buffer, bytes_this_frame);
	}
}


//============================================================
//  osd_set_mastervolume
//============================================================

void osd_set_mastervolume(int attenuation)
{
	// clamp the attenuation to 0-32 range
	if (attenuation > 0)
		attenuation = 0;
	if (attenuation < -32)
		attenuation = -32;

	// set the stream volume
	if (stream_buffer != NULL)
		IDirectSoundBuffer_SetVolume(stream_buffer, (attenuation == -32) ? DSBVOLUME_MIN : attenuation * 100);
}



//============================================================
//  dsound_create_buffers
//============================================================

HRESULT stream_init(int sample_rate, int channels)
{
	HRESULT result;
	void *buffer;
	DWORD locked;

	wrlog("Starting Stream Creation");

	stream_format.wBitsPerSample = 16;
	stream_format.wFormatTag = WAVE_FORMAT_PCM;
	stream_format.nChannels = channels;
	stream_format.nSamplesPerSec = sample_rate; 
	stream_format.nBlockAlign = stream_format.wBitsPerSample * stream_format.nChannels / 8;
	stream_format.nAvgBytesPerSec = stream_format.nSamplesPerSec * stream_format.nBlockAlign;

	// compute the buffer size based on the output sample rate
	stream_buffer_size = stream_format.nSamplesPerSec * stream_format.nBlockAlign * 2 / 10; //3 here is the number of sample frames + 30
	stream_buffer_size = (stream_buffer_size / 1024) * 1024; 
	if (stream_buffer_size < 1024)	stream_buffer_size = 1024;
	wrlog("Stream buffer computed size is %d", stream_buffer_size);

	memset(&stream_desc, 0, sizeof(stream_desc));
	stream_desc.dwSize = sizeof(stream_desc);
	stream_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	stream_desc.dwBufferBytes = stream_buffer_size;
	stream_desc.lpwfxFormat = &stream_format;

	wrlog("Stream Created - stream_buffer_size = %d\n", stream_buffer_size);

	// create the stream buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &stream_desc, &stream_buffer, NULL);
	if (result != DS_OK)
	{
		wrlog("Error creating DirectSound stream buffer: %08x", (UINT32)result);
		goto error;
	}

	// lock the buffer
	result = IDirectSoundBuffer_Lock(stream_buffer, 0, stream_buffer_size, &buffer, &locked, NULL, NULL, 0);
	if (result != DS_OK)
	{
		wrlog("Error locking DirectSound stream buffer: %08x", (UINT32)result);
		goto error;
	}

	// clear the stream buffer and unlock it
	memset(buffer, 0, locked);
	IDirectSoundBuffer_Unlock(stream_buffer, buffer, locked, NULL, 0);

	// start playing
	result = IDirectSoundBuffer_Play(stream_buffer, 0, 0, DSBPLAY_LOOPING);
	if (result != DS_OK)
	{
		wrlog("Error playing: %08x", (UINT32)result);
		goto error;
	}
	return DS_OK;


	// error handling
error:
	wrlog("Some sort of error occured");
	dsound_destroy_buffers();
	return result;
}




