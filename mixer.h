#pragma once

#include <string>
#include <cstdint>

typedef struct
{
	int state;                      // state of the sound ?? Playing/Stopped
	unsigned int pos;
	int loaded_sample_num;
	int id;
	int looping;
	double vol;
	int stream_type;


} CHANNEL;


typedef struct
{
	int channels;		     //<  Number of channels
	unsigned short sampleRate;	/**<  Sample rate */
	unsigned long sampleCount;	/**<  Sample count */
	unsigned long dataLength;	/**<  Data length */
	int16_t bitPerSample;	/**<  The bit rate of the WAV */
	int state;                  //Sound loaded or sound empty
	int num;
	std::string name;
	union {
		uint8_t *u8;      /* data for 8 bit samples */
		int16_t *u16;    /* data for 16 bit samples */
		void *buffer;           /* generic data pointer to the actual wave data*/
	} data;


}SAMPLE;

void mute_audio();
void restore_audio();
void pause_audio();
void resume_audio();
void mixer_init(int rate, int fps);
void mixer_update();
void mixer_end();
int load_sample(char *archname, char *filename);
void sample_stop(int chanid);
void sample_start(int chanid, int samplenum, int loop);
void sample_set_position(int chanid, int pos);
void sample_set_volume(int chan, int volume);
void sample_set_freq(int channid, int freq);
int sample_playing(int chanid);
void sample_end(int chanid);
void sample_remove(int samplenum);
//Streaming audio functions added on.
void stream_start(int chanid, int stream, int bits, int frame_rate);
void stream_stop(int chanid, int stream);
void stream_update(int chanid, short *data);
void stream_update(int chanid, unsigned char* data);
int create_sample(int bits, bool is_stereo, int freq, int len);






