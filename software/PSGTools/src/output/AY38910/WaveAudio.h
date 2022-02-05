#pragma once

#include <string>
#include <windows.h>
//#include <mmsystem.h>

class WaveAudio
{
public:
	WaveAudio();
	virtual ~WaveAudio();

	void Start();
	void Stop();

protected:
	virtual void FillBuffer(unsigned char* buffer, unsigned long size) = 0;

private:
	void CheckForError(MMRESULT res, const char* msg);
	void InitAudioBuffers();
	void FreeAudioBuffers();
	void InitAudioDevice();
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