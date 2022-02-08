#pragma once

#include <windows.h>

class WaveAudio
{
public:
	WaveAudio(int sampleRate, int frameRate, int sampleChannels, int sampleBytes);
	virtual ~WaveAudio();

	void Start();
	void Stop();

protected:
	virtual void FillBuffer(unsigned char* buffer, unsigned long size) = 0;

private:
	void CheckForError(MMRESULT res, const char* msg);
	void InitAudioBuffers(int samplesPerFrame, int bytesPerSample);
	void FreeAudioBuffers();
	void InitAudioDevice(int sampleRate, int sampleChannels, int bytesPerSample);
	void FreeAudioDevice();
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
	bool m_working;
};