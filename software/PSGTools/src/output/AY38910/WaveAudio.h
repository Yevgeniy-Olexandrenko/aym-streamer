#pragma once

#include <string>
#include <windows.h>
//#include <mmsystem.h>

class WaveAudio
{
public:
	WaveAudio(std::string deviceID);
	~WaveAudio();

public:
	void Start();
	void Stop();
	bool IsPlaying();
	void SetCurrentDevice(std::string deviceID);

protected:
	virtual void FillBuffer(unsigned char* buffer, unsigned long size) = 0;

private:
	HWAVEOUT m_waveout;
	WAVEFORMATEX m_format;
	WAVEHDR m_buffers[4];

	static void CALLBACK WaveOutProc(
		HWAVEOUT hwo, UINT uMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR dwParam1,
		DWORD_PTR dwParam2
	);

	void OnBufferDone(WAVEHDR* hdr);
	bool m_isPlaying;
};