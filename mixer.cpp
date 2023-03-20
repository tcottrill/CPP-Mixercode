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

//Known issues

// 1. Limited to mono audio streams (stereo coming when I feel like adding it)
// 2. Samples are never freed, this is a big memory leak
// 3. Why aren't samples pointers to begin with, and why the limitation?
// 4. Arbitrary limit on # of channels
//Notes:

//Names are stored and called without extension so file1.wav/ogg is "file1"

#define NOMINMAX
#include "framework.h"
#include "mixer.h"
#include "wavfile.h"
#include "fileio.h"
#include "dsoundmain.h"
#include "dsoundstream.h"
#include <cstdint>
#include <math.h>
#include <algorithm>        // std::max
#include <stdexcept>        // std::out_of_range
#include <list>
#include <vector>

#define LOG_AUDIO_DEBUG 1

//#define SMP_START 0x2c
#define MAX_CHANNELS   16 
#define MAX_SOUNDS     130
#define SOUND_NULL     0
#define SOUND_LOADED   9
#define SOUND_PLAYING  5
#define SOUND_STOPPED  1
#define SOUND_PCM      4
#define SOUND_STREAM   5

//Variables
static int16_t BUFFER_SIZE = 0;
static int SYS_FREQ = 44100;
static double dbvolume[201];
int16_t* soundbuffer;
CHANNEL channel[MAX_CHANNELS];
//SAMPLE *sound;

//List of actively playing samples
std::list<int> audio_list;
//List of loaded samples, so we can track, call by name, and delete when done;
std::vector<SAMPLE*> lsamples;

//Functions

std::string remove_extension(const std::string& filename) {
	size_t lastdot = filename.find_last_of(".");
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}


std::string base_name(const std::string& path)
{
	return path.substr(path.find_last_of("/\\") + 1);
}

float scaleVolume(int sliderValue) {

	double dSliderValue = sliderValue;
	double logSliderValue = log10(dSliderValue / 10);
	double logMaxSliderValue = log10(10);
	float scaledVolume = (float)(logSliderValue / logMaxSliderValue);

	return scaledVolume;

}


inline double dBToAmplitude(double db)
{
	return pow(10.0f, db / 20.0f);
}

inline double AmplitudeTodB(double amplitude)
{
	return 20.0f * log10(amplitude);
}

void buildvolramp() //This only goes down, not up. Need to build an up vol to 200 percent as well. This needs some work
{
	dbvolume[0] = 0;
	double k = 0;
	int i;

	for (i = 99; i > 0; i--)
	{
		k = k - .44f;
		dbvolume[i] = dBToAmplitude(k);
		//wrlog("Value at %i is %f", i, dbvolume[i]);
	}

	dbvolume[100] = 1.00;
	k = 1.0;

	for (i = 101; i < 200; i++)
	{

		k = k + .05f;
		dbvolume[i] = dBToAmplitude(k);
		//wrlog("Value at %i is %f", i, dbvolume[i]);
	}
}


unsigned char Make8bit(int16_t sample)
{
	sample >>= 8;  // drop the low 8 bits
	sample ^= 0x80;  // toggle the sign bit
	return (sample & 0xFF);
}


short Make16bit(uint8_t sample)
{
	short sample16 = (int16_t)(sample - 0x80) << 8;
	return sample16;
}


static void byteswap(unsigned char& byte1, unsigned char& byte2)
{
	byte1 ^= byte2;
	byte2 ^= byte1;
	byte1 ^= byte2;
}


bool ends_with(const std::string& s, const std::string& ending)
{
	return (s.size() >= ending.size()) && equal(ending.rbegin(), ending.rend(), s.rbegin());
}

/*
//This function takes a loaded 8 bit MONO sample and upscales it to 16 bit so it can be mixed.
int sample_up16(int num)
{
	int	old_id = num;              //ID of original sound
	int new_id = -1;               // return val from create sample;
	SAMPLE p;
	SAMPLE n;

	p = sound[old_id];
	new_id = create_sample(p.name, 16, p.channels, p.sampleRate, p.sampleCount);
	n = sound[new_id];

	//Copy all the data to the new buffer and upscale it.
	for (unsigned long i = 0; i < sound[old_id].sampleCount; i++)
	{
		sound[new_id].data.u16[i] = (int16_t)(((sound[old_id].data.u8[i] - 128) << 8));
	}
	sound[old_id].channels = 0;
	sound[old_id].sampleRate = 0;
	sound[old_id].bitPerSample = 0;
	sound[old_id].dataLength = 0;
	sound[old_id].sampleCount = 0;
	sound[old_id].state = SOUND_NULL;
	sound[old_id].name = "";
	sound[old_id].num = -1;
	if (sound[old_id].data.buffer)   //Delete the old data
	{
		free(sound[old_id].data.buffer);
	}
	return new_id;
}
*/

int load_sample(char* archname, char* filename)
{
	int	sound_id = -1;      // id of sound to be loaded
	//int  index;               // looping variable
	unsigned char* sample_temp = nullptr;
	int result;

	SAMPLE* new_sample = new(SAMPLE);

	//LOAD FILE - Please add some error handling here!!!!!!!!!
	if (archname)
	{
		sample_temp = load_generic_zip(archname, filename);
		//Create Wav data
		result = WavFileLoadInternal(sample_temp, (int)get_last_zip_file_size());
	}
	else
	{
		sample_temp = load_file(filename);
		//Create Wav data
		result = WavFileLoadInternal(sample_temp, get_last_file_size());
		if (result == 0)
		{
			wrlog("Returning Error, check loaded file format.");
			if (sample_temp) {
				free(sample_temp);
			}
			return -1;
		}
	}
	wrlog("Continuing load");

	//If sample loaded successfully proceed!
	/*
	// step one: are there any open id's ?
	for (index = 0; index < MAX_SOUNDS; index++)
	{
		// make sure this sound is unused
		if (sound[index].state == SOUND_NULL)
		{
			sound_id = index;
			break;
		}
	} // end for index
	  // did we get a free id? If not,fail.
	if (sound_id == -1)
	{
		wrlog("No free sound id's for sample %s", filename); return(-1);
	}
	//SOUND
	wrlog("Loading file %s with sound id %d", filename, sound_id);
	*/
	// set rate and size in data structure
	/**/
	if (new_sample)
	{
		new_sample->channels = Wave.channels;
		new_sample->sampleRate = Wave.sampleRate;
		new_sample->bitPerSample = Wave.bitPerSample;
		new_sample->dataLength = Wave.dataLength;
		new_sample->sampleCount = Wave.sampleCount;
		new_sample->state = SOUND_LOADED;
		new_sample->name = filename;
		new_sample->name = remove_extension(base_name(new_sample->name));
		new_sample->num = 1; //For now
	}
	else
	{
		wrlog("Sample load failed");
		return -1;
	}

	//	wrlog("File %s loaded with sound id: %d and state is: %d", filename, sound_id, sound[sound_id].state);
		//wrlog("Loading WAV #: %d", sound_id);
		//wrlog("Stored filename is %s", sound[sound_id].name.c_str());
	wrlog("Channels #: %d", Wave.channels);
	wrlog("Samplerate #: %d", Wave.sampleRate);
	wrlog("Length #: %d", Wave.dataLength);
	wrlog("BPS #: %d", Wave.bitPerSample);
	wrlog("Samplecount #: %d", Wave.sampleCount);

	wrlog("Starting Data copy");
	// Add rate/stereo conversion here. This is done before
	if (Wave.loadtype == 1) //Wav file
	{
		new_sample->data.buffer = (unsigned char*)malloc(Wave.dataLength);
		//	memcpy(sound[sound_id].data.buffer, sample_temp + 0x2c, Wave.dataLength); //Have to cut out the header data from the wave data
		memcpy(new_sample->data.buffer, Wave.data, Wave.dataLength);
	}

	if (Wave.loadtype == 2) // Ogg file
	{
		new_sample->data.buffer = (unsigned char*)malloc(Wave.dataLength);
		memcpy(new_sample->data.buffer, Wave.data, Wave.dataLength);
		//Since we're using a different BUFFER here, we need to delete the Wave.data. It's not a pointer here, it's an actual buffer.
		free(Wave.data);
	}

	wrlog("Done Data copy");
	//We don't need the original data any more.
	free(sample_temp);
	//Upconvert 8 to 16 bit sample for ease of code maintenance
//	if (sound[sound_id].bitPerSample == 8) { sound_id = sample_up16(sound_id); }
	//Return Sound ID
	wrlog("Loaded sound success");
	//Add this sample to the loaded samples list
	lsamples.push_back(new_sample);
	return(sound_id);
}

//Set buffer size as part of mixer setup based on frame rate and freq.
void init_mixer(int rate, int fps)
{
	int i = 0;
	BUFFER_SIZE = rate / fps;
	SYS_FREQ = rate;

	wrlog("FREQ: %d, BUFFER LEN %d", rate, fps);
	wrlog("Buffer Size %d", BUFFER_SIZE);

	soundbuffer = (short*)malloc(BUFFER_SIZE * 2);  //Create our 16 bit output buffer
	memset(soundbuffer, 0, BUFFER_SIZE * 2);         //Clear our 16 bit output buffer
	dsound_init(rate, 1);							//Start the directsound engine
	stream_init(rate, 1);						    //Start a stream playing for our data
	osd_set_mastervolume(-3);						//Set volume *(Make this adjustable)*

	//Clear and init Sample Channels
	for (i = 0; i < MAX_CHANNELS; i++)
	{
		channel[i].loaded_sample_num = -1;
		channel[i].state = SOUND_STOPPED;
		channel[i].looping = 0;
		channel[i].pos = 0;
		channel[i].vol = 1.0;
		sample_set_volume(i, 100);
		//wrlog("Channel default volume is %f", channel[i].vol);
	}

	//Set all samples to empty for start
	//for (i = 0; i < MAX_SOUNDS; i++) {sound[i].state = SOUND_NULL;}
	//Build the volume table.
	buildvolramp();

	int a = audio_list.empty();
}


void update_mixer()
{
	int32_t smix = 0;    //Sample mix buffer
	int32_t fmix = 0;   // Final sample mix buffer

	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		fmix = 0; //Set mix buffer to zero (silence for 16 bit audio) 
		smix = 0;

		for (std::list<int>::iterator it = audio_list.begin(); it != audio_list.end(); ++it)
		{
			if (channel[*it].state == SOUND_PLAYING)
			{
				SAMPLE* p = lsamples[channel[*it].loaded_sample_num]; //To shorten 

				channel[*it].pos += p->channels; //If it's stereo only play the left channel.

				if (channel[*it].pos >= p->sampleCount)//p.dataLength)
				{
					channel[*it].pos = 0;
					if (channel[*it].looping == 0) { channel[*it].state = SOUND_STOPPED; audio_list.erase(it); }
				}
				// 16 bit mono
				if (p->bitPerSample == 16)
				{
					smix = (short)p->data.u16[channel[*it].pos];
					smix = (int32_t)lround(smix = static_cast<int32_t> (smix * channel[*it].vol));
				}
				// 8 bit mono
				else if (p->bitPerSample == 8)
				{
					smix = (short)(((p->data.u8[channel[*it].pos] - 128) << 8));
					smix = (int32_t)lround(smix = static_cast<int32_t> (smix * channel[*it].vol));
				}

				smix = static_cast<int32_t> (smix * .60); //Reduce volume to avoid clipping. This number can vary depending on the samples.
				fmix = fmix + smix;
			}
		}
		if (fmix)
		{
			if (fmix > 32767) { fmix = 32767; if (LOG_AUDIO_DEBUG) wrlog("truncating high"); };
			if (fmix < -32768) { fmix = -32768; if (LOG_AUDIO_DEBUG) wrlog("truncating low"); }
		}
		soundbuffer[i] = static_cast<short>(fmix);
	}
	osd_update_audio_stream(soundbuffer, BUFFER_SIZE);
}


void end_mixer()
{
	dsound_stop();
	//for (int i = 0; i < MAX_SOUNDS; i++)
	for (std::size_t i = 0; i < lsamples.size(); ++i)

	{
		if (lsamples[i]->data.buffer)
		{
			free(lsamples[i]->data.buffer);
			wrlog("Freeing sample #%d named %s", i, lsamples[i]->name.c_str());
		}
	}
}


void sample_stop(int chanid)
{
	channel[chanid].state = SOUND_STOPPED;
	channel[chanid].looping = 0;
	channel[chanid].pos = 0;
	audio_list.remove(chanid);
}


void sample_start(int chanid, const std::string name, int loop)
{
	int lname = nameToNum(name);
	if (lname == -1) { wrlog("Name not found in lookup, cannot play?"); }
	else
	{
		sample_start(chanid, lname, loop);
	}
}


void sample_start(int chanid, int samplenum, int loop)
{
	//First check that it's a valid sample! **This may not be nessary with the changes to add a vector pushback **
	if (lsamples[samplenum]->state != SOUND_LOADED)
	{
		wrlog("error, attempting to play invalid sample on channel %d state: %d", chanid, channel[chanid].state);
		return;
	}

	if (channel[chanid].state == SOUND_PLAYING)
	{
		wrlog("error, sound already playing on this channel %d state: %d", chanid, channel[chanid].state);
		return;
	}

	wrlog("Starting Sample %d on Channel %d", samplenum, chanid);
	channel[chanid].state = SOUND_PLAYING;
	channel[chanid].stream_type = SOUND_PCM;
	channel[chanid].loaded_sample_num = samplenum;
	channel[chanid].looping = loop;
	channel[chanid].pos = 0;
	channel[chanid].vol = 1.0;
	audio_list.emplace_back(chanid);
}


int sample_get_position(int chanid)
{
	return channel[chanid].pos;
}


void sample_set_volume(int chanid, int volume)
{

	channel[chanid].vol = dbvolume[volume];

	wrlog("Setting channel %i to with volume %i setting bvolume %f", chanid, volume, channel[chanid].vol);

	//channel[chanid].vol = 100 * (1 - (log(volume) / log(0.5)));
	// vol = CLAMP(0, pow(10, (vol/2000.0))*255.0 - DSBVOLUME_MAX, 255);
	/*
		To increase the gain of a sample by X db, multiply the PCM value by pow( 2.0, X/6.014 ). i.e. gain +6dB means doubling the value of the sample, -6dB means halving it.
		inline double amp2dB(const double amp)
	{
		// input must be positive +1.0 = 0dB
		if (amp < 0.0000000001) { return -200.0; }
		return (20.0 * log10(amp));
	}
	inline double dB2amp(const double dB)
	{
	  // 0dB = 1.0
	  //return pow(10.0,(dB * 0.05)); // 10^(dB/20)
	  return exp(dB * 0.115129254649702195134608473381376825273036956787109375);
	}



	0. (init) double max = 0.0; double tmp;
	1. convert your integer audio to floating point (i would use double, but whatever)
	2. for every sample do:
	CODE: SELECT ALL

	tmp = amp2dB(fabs(x));
	max = (tmp > max ? tmp : max); // store the highest dB peak..
	3. at the end, when you have processed the whole audio - max gives the maximum peak level in dB, so to normalize to 0dB you can do this:
	0. (precalculate) double scale = dB2amp(max * -1.0);
	1. multiply each sample by "scale"
	2. convert back to integer if you want..

	OR
	float volume_control(float signal, float gain) {

		return signal * pow( 10.0f, db * 0.05f );

	}

	OR
	Yes, gain is just multiplying by a factor. A gain of 1.0 makes no change to the volume (0 dB), 0.5 reduces it by a factor of 2 (-6 dB), 2.0 increases it by a factor of 2 (+6 dB).

	To convert dB gain to a suitable factor which you can apply to your sample values:

	double gain_factor = pow(10.0, gain_dB / 20.0);

	OR
	inline float AmplitudeTodB(float amplitude)
	{
	  return 20.0f * log10(amplitude);
	}

	inline float dBToAmplitude(float dB)
	{
	  return pow(10.0f, db/20.0f);
	}

	Decreasing Volume:

	dB	Amplitude
	-1	0.891
	-3	0.708
	-6	0.501
	-12	0.251
	-18	0.126
	-20	0.1
	-40	0.01
	-60	0.001
	-96	0.00002
	Increasing Volume:

	dB	Amplitude
	1	1.122
	3	1.413
	6	1.995
	12	3.981
	18	7.943
	20	10
	40	100
	60	1000
	96	63095.734
	int alleg_to_dsound_volume[256];
	int v = 0;
	//setup volume lookup table
	alleg_to_dsound_volume[0] = -1000;
	for (v = 1; v < 256; v++) {
		int dB = 0 + 2000.0 * log10(v / 255.0);
		alleg_to_dsound_volume[v] = std::max(-1000, dB);
		wrlog("Value at %d is %d", v, dB);
	}

	1

You will want to work with what Wikipedia calls "field quantities."

Your engineer is telling you to work in steps of .1dB, which would be an amplitude ratio of .1=20log10(x). To get x you do 10^(.1/20) = 1.01158

From x dB to ratio (amplitude): 10^(x/20)

From x ratio (amplitude) to dB: 20Log10(x)

If you can calculate the ratio, then you should be able to calculate the correction needed, round to the next .1dB step, and apply it in one step.



*/
};

int sample_get_volume(int chanid) { return 0; };
void sample_set_position(int chanid, int pos) {};
void sample_set_freq(int channid, int freq) {};


int sample_playing(int chanid)
{
	if (channel[chanid].state == SOUND_PLAYING)
		return 1;
	else return 0;
}


void sample_end(int chanid)
{
	channel[chanid].looping = 0;
}

// Streams are designed to be started with the mixer and updated throughout the life of the program
// Why no return value on this one?
void stream_start(int chanid, int stream)
{
	int stream_sample = 0;
	static int stream_num = 0;

	const char* names[] = { "STREAM1","STREAM2","STREAM3","STREAM4","STREAM5","STREAM6","STREAM7","STREAM8" };
	if (stream_num < 8)
	{
		stream_sample = create_sample(names[stream_num], 16, 1, SYS_FREQ, BUFFER_SIZE);
		stream_num++;
	}
	else
	{
		wrlog("error, no more streams allowed, why so many?");
	}

	if (channel[chanid].state == SOUND_PLAYING)
	{
		wrlog("error, sound already playing on this channel %d state: %d", chanid, channel[chanid].state);
		return;
	}

	channel[chanid].state = SOUND_PLAYING;
	channel[chanid].loaded_sample_num = stream_sample;
	channel[chanid].looping = 1;
	channel[chanid].pos = 0;
	channel[chanid].stream_type = SOUND_STREAM;
	audio_list.push_back(chanid);
}


void stream_stop(int chanid, int stream)
{
	channel[stream].state = SOUND_STOPPED;
	channel[stream].loaded_sample_num = 0;
	channel[stream].looping = 0;
	channel[stream].pos = 0;
	audio_list.remove(chanid);
	//Warning, This doesn't delete the created sample	
}

//Streams MUST be 16bit mono
void stream_update(int chanid, int stream, short* data)
{
	if (channel[chanid].state == SOUND_PLAYING)
	{
		SAMPLE* p = lsamples[channel[chanid].loaded_sample_num];
		memcpy(p->data.buffer, data, p->dataLength);
	}

}

void sample_remove(int samplenum)
{
	lsamples.erase(lsamples.begin() + samplenum);
}


//soundbuffer[i] = ((short)(sound[j].data[sample_pos + y] << 8) | (sound[j].data[sample_pos + (y + 1)]));
/*
short int mix_sample(short int sample1, short int sample2) {
	const int32_t result(static_cast<int32_t>(sample1) + static_cast<int32_t>(sample2));
	typedef std::numeric_limits<short int> Range;
	if (Range::max() < result)
		return Range::max();
	else if (Range::min() > result)
		return Range::min();
	else
		return result;
}
*/


// create_sample:
// *  Constructs a new sample structure of the specified type.
// Len is the number of Samples, not data length.

int create_sample(const std::string& name, int bits, int channels, int freq, int len)
{

	SAMPLE* new_sample = (SAMPLE*)malloc;
	int	sound_id = -1;      // id of sound to be loaded
	int  index;               // looping variable
	int chan = channels;
	int multi = bits / 8;
	// step one: are there any open id's ?
	/*
	for (index = 0; index < MAX_SOUNDS; index++)
	{
		// make sure this sound is unused
		if (sound[index].state == SOUND_NULL)
		{
			sound_id = index;
			break;
		} // end if

	} // end for index
	  // did we get a free id? If not,fail.
	if (sound_id == -1) {
		wrlog("No free sound id's for creation of new sample?"); return(-1);
	}
	*/
	// Fill out data structure and create empty sample
	new_sample->sampleRate = freq;
	new_sample->bitPerSample = bits;
	new_sample->channels = chan;
	new_sample->dataLength = (len * chan) * multi;
	new_sample->sampleCount = len;
	new_sample->state = SOUND_LOADED;
	new_sample->name = name;
	new_sample->num = sound_id;
	new_sample->data.buffer = (unsigned char*)malloc((len * chan) * multi);
	memset(new_sample->data.buffer, 0, (len * chan) * multi);
	wrlog("Datalength %d", (len * chan) * multi);
	wrlog("Creating and returning Stream Audio Sample with sound id %d, channels %d, bitmulti %d, and samplecount of %d", sound_id, chan, multi, len);
	lsamples.push_back(new_sample);
	return true;
}


//Find a loaded sample number in a list.
int snumlookup(int snum)
{

	for (auto it = lsamples.begin(); it != lsamples.end(); ++it)
		//for (std::vector<SAMPLE*>::iterator it = std::begin(lsamples); it != std::end(lsamples); ++it) {
	{

		if (snum == ((*it)->num)) { return (*it)->num; }
	}

	wrlog("Attempted lookup of sample number, it was not found in loaded samples?");
	return 0;
}

//Be careful that you call this with a real sample->num, not just the loaded sample number 
std::string numToName(int num)
{

	try {
		auto it = lsamples.at(num);      // vector::at throws an out-of-range
		return it->name;
	}

	catch (const std::out_of_range& err)
	{
		wrlog("Out of Range error: get loaded sample name: %s \n", err.what());
	}
	return ("notfound");
}


int nameToNum(std::string name)
{

	for (auto i = lsamples.begin(); i != lsamples.end(); ++i)
	{
		//if (name == i->name) { return i - lsamples.begin(); }

		if ((name.compare((*i)->name) == 0)) { return (*i)->num; }
	}

	wrlog("Sample: %s not found, returning 0\n", name.c_str());

	return -1;
}


/*
Conversion Table
Here are some dB values and corresponding amplitude values to help you better understand how dB and amplitude are related.

Decreasing Volume:

dB	Amplitude
-1	0.891
-3	0.708
-6	0.501
-12	0.251
-18	0.126
-20	0.1
-40	0.01
-60	0.001
-96	0.00002
Increasing Volume:

dB	Amplitude
1	1.122
3	1.413
6	1.995
12	3.981
18	7.943
20	10
40	100
60	1000
96	63095.734


Use a slider value range of 0.0001 – 1
instead of -80db – 0db (the 0.0001 is important, and stops the slider breaking at zero)
When passing the slider value to the SetFloat function, convert it using Mathf.Log10(value) * 20;
e.g. mixer.SetFloat(“MusicVol”, Mathf.Log10(sliderValue) * 20);

This is a javascript function i have for a logarithmic scale for dbm. The input is a percentage (0.00 to 1.00) and the max value (my implementation uses 12db)

The mid point is set to 0.5 and that will be 0db.

When the percentage is zero, the output is negative Infinity.

function percentageToDb(p, max) {
	 return max * (1 - (Math.log(p) / Math.log(0.5)));
};



double MeasureDecibels(byte[] samples, int length, int bitsPerSample,int numChannels, params int[] channelsToMeasure)
{
	if (samples == null || length == 0 || samples.Length == 0)
	{
		throw new ArgumentException("Missing samples to measure.");
	}
	//check bits are 8 or 16.
	if (bitsPerSample != 8 && bitsPerSample != 16)
	{
		throw new ArgumentException("Only 8 and 16 bit samples allowed.");
	}
	//check channels are valid
	if (channelsToMeasure == null || channelsToMeasure.Length == 0)
	{
		throw new ArgumentException("Must have target channels.");
	}
	//check each channel is in proper range.
	foreach(int channel in channelsToMeasure)
	{
		if (channel < 0 || channel >= numChannels)
		{
			throw new ArgumentException("Invalid channel requested.");
		}
	}

	//ensure we have only full blocks. A half a block isn't considered valid.
	int sampleSizeInBytes = bitsPerSample / 8;
	int blockSizeInBytes = sampleSizeInBytes * numChannels;
	if (length % blockSizeInBytes != 0)
	{
		throw new ArgumentException("Non-integral number of bytes passed for given audio format.");
	}

	double sum = 0;
	for (var i = 0; i < length; i = i + blockSizeInBytes)
	{
		double sumOfChannels = 0;
		for (int j = 0; j < channelsToMeasure.Length; j++)
		{
			int channelOffset = channelsToMeasure[j] * sampleSizeInBytes;
			int channelIndex = i + channelOffset;
			if (bitsPerSample == 8)
			{
				sumOfChannels = (127 - samples[channelIndex]) / byte.MaxValue;
			}
			else
			{
				double sampleValue = BitConverter.ToInt16(samples, channelIndex);
				sumOfChannels += (sampleValue / short.MaxValue);
			}
		}
		double averageOfChannels = sumOfChannels / channelsToMeasure.Length;
		sum += (averageOfChannels * averageOfChannels);
	}
	int numberSamples = length / blockSizeInBytes;
	double rootMeanSquared = Math.Sqrt(sum / numberSamples);
	if (rootMeanSquared == 0)
	{
		return 0;
	}
	else
	{
		double logvalue = Math.Log10(rootMeanSquared);
		double decibel = 20 * logvalue;
		return decibel;
	}
}

*/