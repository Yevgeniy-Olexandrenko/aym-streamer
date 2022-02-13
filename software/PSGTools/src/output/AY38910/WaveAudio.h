#pragma once

#include <mutex>
#include <windows.h>

class WaveAudio
{
protected:
	WaveAudio();
	virtual ~WaveAudio();

	bool Open(int sampleRate, int frameRate, int sampleChannels, int sampleBytes);
	void Close();

	virtual void FillBuffer(unsigned char* buffer, unsigned long size) = 0;

private:
	void OnBufferDone(WAVEHDR* hdr);
	static void CALLBACK WaveOutProc(
		HWAVEOUT hwo, UINT uMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR dwParam1,
		DWORD_PTR dwParam2
	);

protected:
	HWAVEOUT m_waveout;
	WAVEFORMATEX m_format;
	WAVEHDR m_buffers[4];
	std::mutex m_mutex;
};
