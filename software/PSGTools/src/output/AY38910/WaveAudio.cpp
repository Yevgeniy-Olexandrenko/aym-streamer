#include "WaveAudio.h"

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

WaveAudio::WaveAudio(int sampleRate, int frameRate, int sampleChannels, int sampleBytes)
	: m_working(false)
	, m_waveout(NULL)
{
	int samplesPerFrame = sampleRate / frameRate;
	int bytesPerSample = sampleBytes * sampleChannels;

	InitAudioBuffers(samplesPerFrame, bytesPerSample);
	InitAudioDevice(sampleRate, sampleChannels, bytesPerSample);
}

WaveAudio::~WaveAudio()
{
	FreeAudioDevice();
	FreeAudioBuffers();
}

void WaveAudio::Start()
{
	if (!m_working) 
	{
		m_working = true;
		for (int i = 0; i < 4; ++i)
		{
			MMRESULT res = waveOutPrepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CheckForError(res, "waveOutPrepareHeader failed");
			
			ZeroMemory(m_buffers[i].lpData, m_buffers[i].dwBufferLength);

			res = waveOutWrite(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CheckForError(res, "waveOutWrite failed");
		}
	}
}

void WaveAudio::Stop()
{
	if (m_working) 
	{
		m_working = false;

		MMRESULT res = waveOutReset(m_waveout);
		CheckForError(res, "waveOutReset failed");

		for (int i = 0; i < 4; ++i)
		{
			res = waveOutUnprepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CheckForError(res, "waveOutUnprepareHeader failed");
		}
	}
}

void WaveAudio::CheckForError(MMRESULT res, const char* msg)
{
	if (res != MMSYSERR_NOERROR)
	{
		m_working = false;

		wchar_t buf[1024];
		waveOutGetErrorTextW(res, buf, 1024);

		::printf("%s: %ws (MMRESULT=0x%08X)", msg, buf, res);
	}
}

void WaveAudio::InitAudioBuffers(int samplesPerFrame, int bytesPerSample)
{
	for (int i = 0; i < 4; ++i)
	{
		ZeroMemory(&m_buffers[i], sizeof(WAVEHDR));
		m_buffers[i].dwBufferLength = samplesPerFrame * bytesPerSample;
		m_buffers[i].lpData = (char*)malloc(samplesPerFrame * bytesPerSample);
	}
}

void WaveAudio::FreeAudioBuffers()
{
	for (int i = 0; i < 4; i++) 
	{
		free(m_buffers[i].lpData);
	}
}

void WaveAudio::InitAudioDevice(int sampleRate, int sampleChannels, int bytesPerSample)
{
	ZeroMemory(&m_format, sizeof(m_format));
	m_format.wFormatTag = WAVE_FORMAT_PCM;
	m_format.nSamplesPerSec = sampleRate;
	m_format.wBitsPerSample = (bytesPerSample / sampleChannels) * 8;
	m_format.nChannels = sampleChannels;
	m_format.nBlockAlign = bytesPerSample;
	m_format.cbSize = 0;

	MMRESULT res = waveOutOpen(&m_waveout, WAVE_MAPPER, &m_format, (DWORD_PTR)WaveAudio::WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	CheckForError(res, "waveOutOpen failed");
}

void WaveAudio::FreeAudioDevice()
{
	waveOutClose(m_waveout);
}

void WaveAudio::OnBufferDone(WAVEHDR* hdr)
{
	if (m_working)
	{
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

