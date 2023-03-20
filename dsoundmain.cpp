
//============================================================
//
//  sound.c - Win32 implementation of MAME sound routines
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================


#include "dsoundmain.h"

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#undef WINNT
#include <dsound.h>
#include "framework.h"
#include "log.h"


#pragma comment (lib, "dsound.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "winmm.lib")


//============================================================
//  LOCAL VARIABLES
//============================================================

// DirectSound objects
LPDIRECTSOUND		        dsound;
static DSCAPS				dsound_caps;

// sound buffers
static LPDIRECTSOUNDBUFFER	primary_buffer;
static DSBUFFERDESC			primary_desc;
static WAVEFORMATEX			primary_format;

LPDIRECTSOUND get_dsound_pointer()
{
	return dsound;
}

void dsound_stop()
{
//Check here that all playing stream buffers are ended and destroyed

	wrlog("DIRECTSOUND STOP CALLED");

	//Then release the Primary Buffer
	if (primary_buffer != NULL)
		IDirectSoundBuffer_Release(primary_buffer);
	primary_buffer = NULL;

// Then release the object
	if (dsound != NULL)
		IDirectSound_Release(dsound);
	dsound = NULL;
}


//============================================================
//  dsound_init
//============================================================

HRESULT dsound_init(int sample_rate, int channels)
{
	HRESULT result;

	// create the DirectSound object
	result = DirectSoundCreate(NULL, &dsound, NULL);
	if (result != DS_OK)
	{
		wrlog("Error creating DirectSound: %08x\n", (UINT32)result);
		goto error;
	}

	// get the capabilities
	dsound_caps.dwSize = sizeof(dsound_caps);
	result = IDirectSound_GetCaps(dsound, &dsound_caps);
	if (result != DS_OK)
	{
		wrlog("Error getting DirectSound capabilities: %08x\n", (UINT32)result);
		goto error;
	}

	// set the cooperative level
	result = IDirectSound_SetCooperativeLevel(dsound, win_get_window(), DSSCL_PRIORITY);
	if (result != DS_OK)
	{
		wrlog("Error setting DirectSound cooperative level: %08x\n", (UINT32)result);
		goto error;
	}

	// Create a buffer desc for the primary buffer
	memset(&primary_desc, 0, sizeof(primary_desc));
	primary_desc.dwSize = sizeof(primary_desc);
	primary_desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
	primary_desc.lpwfxFormat = NULL;

	// Set the format description of the Primary Buffer
	primary_format.wBitsPerSample = 16;
	primary_format.wFormatTag = WAVE_FORMAT_PCM;
	primary_format.nChannels = channels;
	primary_format.nSamplesPerSec = sample_rate;
	primary_format.nBlockAlign = primary_format.wBitsPerSample * primary_format.nChannels / 8;
	primary_format.nAvgBytesPerSec = primary_format.nSamplesPerSec * primary_format.nBlockAlign;


	// create the primary buffer
	result = IDirectSound_CreateSoundBuffer(dsound, &primary_desc, &primary_buffer, NULL);
	if (result != DS_OK)
	{
		wrlog("Error creating primary DirectSound buffer: %08x", (UINT32)result);
		goto error;
	}

	// attempt to set the primary format
	result = IDirectSoundBuffer_SetFormat(primary_buffer, &primary_format);
	if (result != DS_OK)
	{
		wrlog("Error setting primary DirectSound buffer format: %08x\n", (UINT32)result);
		goto error;
	}

	// get the primary format
	result = IDirectSoundBuffer_GetFormat(primary_buffer, &primary_format, sizeof(primary_format), NULL);
	if (result != DS_OK)
	{
		wrlog("Error getting primary DirectSound buffer format: %08x", (UINT32)result);
		goto error;
	}

	wrlog("DirectSound: Primary buffer: %d Hz, %d bits, %d channels",
		(int)primary_format.nSamplesPerSec, (int)primary_format.wBitsPerSample, (int)primary_format.nChannels);

	return DS_OK;
	
	// error handling
error: 
	dsound_stop();
	return result;
}

