/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

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
	int channels;		        //<  Number of channels
	unsigned short sampleRate;	/**<  Sample rate */
	unsigned long sampleCount;	/**<  Sample count */
	unsigned long dataLength;	/**<  Data length */
	int16_t bitPerSample;	    /**<  The bit rate of the WAV */
	int state;                  //Sound loaded or sound empty
	int num;
	std::string name;
	union {
		uint8_t *u8;            /* data for 8 bit samples */
		int16_t *u16;           /* data for 16 bit samples */
		void *buffer;           /* generic data pointer to the actual wave data*/
	} data;
}SAMPLE;


void init_mixer(int rate, int fps);
void update_mixer();
void end_mixer();
int load_sample(char *archname, char *filename);
void sample_stop(int chanid);
void sample_start(int chanid, int samplenum, int loop);
void sample_start(int chanid, const std::string name, int loop);
void sample_set_position(int chanid, int pos);
void sample_set_volume(int chan, int volume);
void sample_set_freq(int channid, int freq);
int sample_playing(int chanid);
void sample_end(int chanid);
void sample_remove(int samplenum);
//Streaming audio functions added/tacked on.
void stream_start(int chanid, int stream);
void stream_stop(int chanid, int stream);
void stream_update(int chanid, int stream, short *data);
//Internal functions
int sample_up16(int num);
int snumlookup(int snum);
int create_sample(const std::string &name, int bits, int channels, int freq, int len);
int nameToNum(std::string name);
std::string numToName(int num);





