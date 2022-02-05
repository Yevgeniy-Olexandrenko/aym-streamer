#include "WaveAudio.h"

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define BUFFER_SIZE 960
#define CHECK_ERROR(res, msg) \
	if(res != MMSYSERR_NOERROR) \
	{ \
		wchar_t _buf[1024]; \
		waveOutGetErrorTextW(res, _buf, 1024); \
		::printf(msg ": %ws (MMRESULT=0x%08X)", _buf, res); \
	}


WaveAudio::WaveAudio(std::string deviceID)
{
	m_isPlaying = false;
	m_waveout = NULL;

	for (int i = 0; i < 4; i++) {
		ZeroMemory(&m_buffers[i], sizeof(WAVEHDR));
		m_buffers[i].dwBufferLength = BUFFER_SIZE * 2;
		m_buffers[i].lpData = (char*)malloc(BUFFER_SIZE * 2);
	}

	SetCurrentDevice(deviceID);
}

WaveAudio::~WaveAudio()
{
	for (int i = 0; i < 4; i++) {
		free(m_buffers[i].lpData);
	}
	waveOutClose(m_waveout);
}

void WaveAudio::Start()
{
	if (!m_isPlaying) {
		m_isPlaying = true;

		for (int i = 0; i < 4; i++) {
			MMRESULT res = waveOutPrepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CHECK_ERROR(res, "waveOutPrepareHeader failed");
			//InvokeCallback((unsigned char*)buffers[i].lpData, buffers[i].dwBufferLength);
			ZeroMemory(m_buffers[i].lpData, m_buffers[i].dwBufferLength);
			res = waveOutWrite(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CHECK_ERROR(res, "waveOutWrite failed");
		}
	}
}

void WaveAudio::Stop()
{
	if (m_isPlaying) {
		m_isPlaying = false;

		MMRESULT res = waveOutReset(m_waveout);
		CHECK_ERROR(res, "waveOutReset failed");
		for (int i = 0; i < 4; i++) {
			res = waveOutUnprepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CHECK_ERROR(res, "waveOutUnprepareHeader failed");
		}
	}
}

bool WaveAudio::IsPlaying()
{
	return m_isPlaying;
}

void WaveAudio::SetCurrentDevice(std::string deviceID)
{
	//@@currentDevice = deviceID;

	bool wasPlaying = m_isPlaying;
	m_isPlaying = false;
	::printf("closing, hWaveOut=%d", (int)m_waveout);
	if (m_waveout) {
		MMRESULT res;
		if (m_isPlaying) {
			res = waveOutReset(m_waveout);
			CHECK_ERROR(res, "waveOutReset failed");
			for (int i = 0; i < 4; i++) {
				res = waveOutUnprepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
				CHECK_ERROR(res, "waveOutUnprepareHeader failed");
			}
		}
		res = waveOutClose(m_waveout);
		CHECK_ERROR(res, "waveOutClose failed");
	}

	ZeroMemory(&m_format, sizeof(m_format));
	m_format.cbSize = 0;
	m_format.wFormatTag = WAVE_FORMAT_PCM;
	m_format.nSamplesPerSec = 48000;
	m_format.wBitsPerSample = 16;
	m_format.nChannels = 1;
	m_format.nBlockAlign = 2;

	::printf("before open device %s", deviceID.c_str());

	if (deviceID == "default") {
		MMRESULT res = waveOutOpen(&m_waveout, WAVE_MAPPER, &m_format, (DWORD_PTR)WaveAudio::WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
		CHECK_ERROR(res, "waveOutOpen failed");
	}
	else {
		UINT num = waveOutGetNumDevs();
		WAVEOUTCAPSW caps;
		char nameBuf[512];
		m_waveout = NULL;
		for (UINT i = 0; i < num; i++) {
			waveOutGetDevCapsW(i, &caps, sizeof(caps));
			WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, nameBuf, sizeof(nameBuf), NULL, NULL);
			std::string name = std::string(nameBuf);
			if (name == deviceID) {
				MMRESULT res = waveOutOpen(&m_waveout, i, &m_format, (DWORD_PTR)WaveAudio::WaveOutProc, (DWORD_PTR)this, CALLBACK_FUNCTION | WAVE_MAPPED);
				CHECK_ERROR(res, "waveOutOpen failed");
				::printf("Opened device %s", nameBuf);
				break;
			}
		}
		if (!m_waveout) {
			SetCurrentDevice("default");
			return;
		}
	}

	m_isPlaying = wasPlaying;

	if (m_isPlaying) {
		MMRESULT res;
		for (int i = 0; i < 4; i++) {
			res = waveOutPrepareHeader(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CHECK_ERROR(res, "waveOutPrepareHeader failed");
			res = waveOutWrite(m_waveout, &m_buffers[i], sizeof(WAVEHDR));
			CHECK_ERROR(res, "waveOutWrite failed");
		}
	}
}

void WaveAudio::WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) {
		((WaveAudio*)dwInstance)->OnBufferDone((WAVEHDR*)dwParam1);
	}
}

void WaveAudio::OnBufferDone(WAVEHDR* hdr)
{
	if (!m_isPlaying)	return;

	FillBuffer((unsigned char*)hdr->lpData, hdr->dwBufferLength);
	hdr->dwFlags &= ~WHDR_DONE;
	MMRESULT res = waveOutWrite(m_waveout, hdr, sizeof(WAVEHDR));
}
