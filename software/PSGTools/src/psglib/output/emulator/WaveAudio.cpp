#include "WaveAudio.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

WaveAudio::WaveAudio()
	: m_waveout(NULL)
	, m_buffers{}
	, m_format{}
{
}

WaveAudio::~WaveAudio()
{
	Close();
}

bool WaveAudio::Open(int sampleRate, int frameRate, int sampleChannels, int sampleBytes)
{
	if (!m_waveout)
	{
		int samplesPerFrame = sampleRate / frameRate;
		int bytesPerSample = sampleBytes * sampleChannels;

		// create buffers
		for (int i = 0; i < 4; ++i)
		{
			memset(&m_buffers[i], 0, sizeof(WAVEHDR));
			m_buffers[i].dwBufferLength = samplesPerFrame * bytesPerSample;
			m_buffers[i].lpData = (char*)malloc(samplesPerFrame * bytesPerSample);
			memset(m_buffers[i].lpData, 0, m_buffers[i].dwBufferLength);
		}

		// init audio format
		memset(&m_format, 0, sizeof(WAVEFORMATEX));
		m_format.wFormatTag = WAVE_FORMAT_PCM;
		m_format.nSamplesPerSec = sampleRate;
		m_format.wBitsPerSample = (bytesPerSample / sampleChannels) * 8;
		m_format.nChannels = sampleChannels;
		m_format.nBlockAlign = bytesPerSample;
		m_format.cbSize = 0;

		// open audio device
		HWAVEOUT hwo = NULL;
		MMRESULT res = waveOutOpen(&hwo, WAVE_MAPPER, &m_format, (DWORD_PTR)WaveAudio::WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
		if (res != MMSYSERR_NOERROR) return false;

		// assign all available buffers to audio playback
		for (int i = 0; i < 4; ++i)
		{
			MMRESULT res = waveOutPrepareHeader(hwo, &m_buffers[i], sizeof(WAVEHDR));
			if (res != MMSYSERR_NOERROR) return false;

			res = waveOutWrite(hwo, &m_buffers[i], sizeof(WAVEHDR));
			if (res != MMSYSERR_NOERROR) return false;
		}

		// everything if fine!
		m_waveout = hwo;
	}
	return (m_waveout != NULL);
}

void WaveAudio::Close()
{
	if (m_waveout)
	{
		HWAVEOUT hwo = m_waveout;
		m_waveout = NULL;

		std::lock_guard<std::mutex> lock(m_mutex);
		MMRESULT res = waveOutReset(hwo);

		for (int i = 0; i < 4; ++i)
		{
			MMRESULT res = waveOutUnprepareHeader(hwo, &m_buffers[i], sizeof(WAVEHDR));
			free(m_buffers[i].lpData);
		}

		waveOutClose(hwo);
	}
}

void WaveAudio::OnBufferDone(WAVEHDR* hdr)
{
	if (m_waveout)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		FillBuffer((unsigned char*)hdr->lpData, hdr->dwBufferLength);
		hdr->dwFlags &= ~WHDR_DONE;
		MMRESULT res = waveOutWrite(m_waveout, hdr, sizeof(WAVEHDR));
	}
}

void WaveAudio::WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) 
	{
		((WaveAudio*)dwInstance)->OnBufferDone((WAVEHDR*)dwParam1);
	}
}
