#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <math.h>
#include "wav_resample.h"


// Function to convert 16-bit integer to fraction
double to_fraction(int16_t sample) 
{
	return static_cast<double>(sample) / 32768.0;
}

// USAGE: float dB = 6.0;  adjust_volume_dB(samples, num_samples, dB); 
void adjust_volume_dB(int16_t* samples, size_t num_samples, float dB) 
{
	// Convert dB to a linear scale factor
	float volume_factor = powf(10, dB / 20);

	for (size_t i = 0; i < num_samples; ++i) {
		// Adjust the volume
		int32_t adjusted_sample = (int32_t)(samples[i] * volume_factor);

		// Clamp the value to avoid overflow
		if (adjusted_sample > INT16_MAX) {
			adjusted_sample = INT16_MAX;
		}
		else if (adjusted_sample < INT16_MIN) {
			adjusted_sample = INT16_MIN;
		}
		samples[i] = (int16_t)adjusted_sample;
	}
}

int16_t linear_interpolate(int16_t a, int16_t b, float t) 
{
	return a + t * (b - a);
}

void linear_interpolation_16(int16_t* input_data, int32_t input_samples, int16_t** output_data, int32_t* output_samples, float ratio) 
{
	*output_samples = (int32_t)(input_samples * ratio);
	*output_data = (int16_t*)malloc(*output_samples * sizeof(int16_t));

	for (int i = 0; i < *output_samples; ++i) {
		float t = i / ratio;
		int index = (int)t;
		float frac = t - index;
		int16_t a = input_data[index];
		int16_t b = input_data[index + 1];
		(*output_data)[i] = linear_interpolate(a, b, frac);
	}
}

void linear_interpolation_8(uint8_t* input, uint8_t* output, int input_size, int output_size) {
	for (int i = 0; i < output_size; ++i) {
		float ratio = (float)i * (input_size - 1) / (output_size - 1);
		int index = (int)ratio;
		float fraction = ratio - index;

		if (index + 1 < input_size) {
			output[i] = (uint8_t)((1 - fraction) * input[index] + fraction * input[index + 1]);
		}
		else {
			output[i] = input[index];
		}
	}
}

/*
void resample_wav(int sound_num)
{
	int16_t* input_data, * output_data;
	int32_t input_samples, output_samples;
	float resample_ratio = 96000.0f / 44100.0f;

	//read_wav(input_filename, &header, &input_data);
	//input_samples = header.subchunk2Size / (header.numChannels * header.bitsPerSample / 8);

	linear_interpolation_16(input_data, input_samples, &output_data, &output_samples, resample_ratio);

}
*/
/*
// Fast Resampling: 
// Example:  int inputRate = 44100; int outputRate = 96000; std::vector<int16_t> input = ..... 
//           std::vector<int16_t> output = resample(input, inputRate, outputRate);
std::vector<int16_t> resample(const std::vector<int16_t>& input, int inputRate, int outputRate) {
	std::vector<int16_t> output;
	float resampleRatio = static_cast<float>(inputRate) / outputRate;

	for (size_t i = 0; i < input.size() - 1; ++i) {
		float t = 0.0f;
		while (t < 1.0f) {
			float sample = linearInterpolate(input[i], input[i + 1], t);
			output.push_back(static_cast<int16_t>(sample));
			t += resampleRatio;
		}
	}

	return output;
}

*/

/* Example Usage
int main() {
	int numSamples;
	int16_t* samples = readAudioSamples("input.raw", &numSamples);
	if (!samples) {return 1;}

	double originalSampleRate = 44100.0; // Original sample rate
	double newSampleRate = 48000.0; // New sample rate
	int newNumSamples;
	int16_t* newSamples = resample(samples, numSamples, newSampleRate, originalSampleRate, &newNumSamples);
	if (!newSamples) { free(samples); return 1; }

	writeAudioSamples("output.raw", newSamples, newNumSamples);

	free(samples);
	free(newSamples);
	return 0;
}
*/