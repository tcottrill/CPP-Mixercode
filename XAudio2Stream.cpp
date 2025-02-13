#include "XAudio2Stream.h"
#include "mixer.h"
#include "log.h"

// Error handling macro
#define HR(hr) if (FAILED(hr)) { wrlog("Error at line %d: HRESULT = 0x%08X\n", __LINE__, hr); return hr; }

#pragma comment(lib, "xaudio2.lib")

// Global variables
const int NUM_BUFFERS = 5;
IXAudio2* pXAudio2 = NULL;
IXAudio2MasteringVoice* pMasterVoice = NULL;
IXAudio2SourceVoice* pSourceVoice = NULL;
BYTE* audioBuffers[NUM_BUFFERS];
DWORD bufferSize;
WAVEFORMATEXTENSIBLE wfx = {};


// Number of audio updates per second
int UpdatesPerSecond = 60;
int SamplesPerSecond = 44100;
int BufferDurationMs = 1000 / UpdatesPerSecond;
int SamplesPerBuffer = SamplesPerSecond / UpdatesPerSecond;

int currentBufferIndex = 0;

BYTE* GetNextBuffer()
{
    wrlog("Now using buffer %d", currentBufferIndex);
    return audioBuffers[currentBufferIndex];
}

HRESULT xaudio2_init(int rate, int fps)
{
    HRESULT hr;

    UpdatesPerSecond = fps;
    SamplesPerSecond = rate;
    BufferDurationMs = 1000 / UpdatesPerSecond;
    SamplesPerBuffer = SamplesPerSecond / UpdatesPerSecond;

    HR(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    // Initialize XAudio2
    HR(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR));
    
    // Create a mastering voice
  //  HR(pXAudio2->CreateMasteringVoice(&pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, 0));
    HR(pXAudio2->CreateMasteringVoice(&pMasterVoice, XAUDIO2_DEFAULT_CHANNELS, SamplesPerSecond, 0, 0));
    
    pMasterVoice->SetVolume(1.0f);
   
    wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
    wfx.Format.nSamplesPerSec = SamplesPerSecond;
    wfx.Format.nChannels = 1;
    wfx.Format.wBitsPerSample = 16;
    wfx.Format.nBlockAlign = wfx.Format.nChannels* wfx.Format.wBitsPerSample / 8;
    wfx.Format.nAvgBytesPerSec =  wfx.Format.nSamplesPerSec* wfx.Format.nBlockAlign;
    wfx.Format.cbSize =sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    wfx.Samples.wValidBitsPerSample = 16;
    wfx.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    // Create the source voice
    hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfx.Format,  XAUDIO2_VOICE_NOPITCH, 1.0f,NULL,NULL,NULL);
    if (FAILED(hr))
    {
        wrlog("Failed to create source voice: %#X\n", hr);
        return hr;
    }

    wrlog("SamplesPerBuffer here %d", SamplesPerBuffer);
    // Allocate the audio buffers
    bufferSize = SamplesPerBuffer * 2;
    for (int i = 0; i < NUM_BUFFERS; ++i)
    {
        audioBuffers[i] = new BYTE[bufferSize];
    }

    // Start the source voice
   HR(pSourceVoice->Start());
   
    currentBufferIndex = 0;

    return S_OK;
}


HRESULT xaudio2_update(BYTE* buffera, DWORD bufferLength)
{
    HRESULT hr;
    //Debugging
    XAUDIO2_VOICE_STATE VoiceState;
    //  wrlog("Bufferlength %d, Buffersize %d buffernum %d",bufferLength,bufferSize, currentBufferIndex);
    // Submit the buffer to the source voice
    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = bufferSize;
    buffer.pContext = audioBuffers[currentBufferIndex];
    buffer.pAudioData = audioBuffers[currentBufferIndex];
   // buffer.Flags = XAUDIO2_END_OF_STREAM;
  
    hr = pSourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        wrlog("Failed to submit source buffer: %#X\n", hr);
    }

    if (hr == XAUDIO2_E_DEVICE_INVALIDATED) {
        /* !!! FIXME: possibly disconnected or temporary lost. Recover? */
        wrlog("Lost the XAudio2 source buffer: %#X\n", hr);
    }

    if (hr != S_OK) {  /* uhoh, panic! */

        pSourceVoice->FlushSourceBuffers();
        wrlog("Panic, some odd error submitting the XAudio2 source buffer: %#X\n", hr);
              
    }

    pSourceVoice->GetState(&VoiceState);
    //wrlog("Buffers in Queue: %d Samples Played: %d", VoiceState.BuffersQueued, VoiceState.SamplesPlayed);
    // Move to the next buffer
    currentBufferIndex = (currentBufferIndex + 1) % NUM_BUFFERS;
   
    return hr;
}


void xaudio2_stop()
{
    // Clean up XAudio2
    if (pSourceVoice) pSourceVoice->DestroyVoice();
    if (pMasterVoice) pMasterVoice->DestroyVoice();
    if (pXAudio2) pXAudio2->Release();

    for (int i = 0; i < 3; ++i)
    {
        delete[] audioBuffers[i];
    }
}
