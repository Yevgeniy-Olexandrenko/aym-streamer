#include <Windows.h>
#include "Player.h"
#include "output/Output.h"

////////////////////////////////////////////////////////////////////////////////

double GetTime()
{
	LARGE_INTEGER counter, frequency;
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&frequency);
	return counter.QuadPart / double(frequency.QuadPart);
}

void WaitFor(const double period)
{
	static double accumulator = 0;
	double start, finish, elapsed;

	accumulator += period;
	finish = (GetTime() + accumulator);

	while (true)
	{
		start = GetTime(); Sleep(1U);
		elapsed = (GetTime() - start);

		if ((accumulator -= elapsed) < 0) return;

		if (accumulator < elapsed)
		{
			accumulator = 0;
			while (GetTime() < finish) SwitchToThread();
			return;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

Player::Player(Output& output)
	: m_output(output)
	, m_module(nullptr)
	, m_playbackStep(1)
	, m_isPlaying(false)
	, m_isPaused(false)
	, m_frameId(0)
{
	m_output.Open();
}

Player::~Player()
{
	Stop();
	m_output.Close();
}

////////////////////////////////////////////////////////////////////////////////

bool Player::Init(const Module& module)
{
	Stop();
	m_isPlaying = false;

	if (m_output.IsOpened() && module.frames.available())
	{
		m_module = &module;
		m_isPlaying = true;
		m_frameId = 0;
	}

	return m_isPlaying;
}

void Player::Play(int playbackStep)
{
	if (playbackStep >= -10 && playbackStep <= +10)
	{
		m_playbackStep = playbackStep;
	}

	if (m_isPaused)
	{
		m_isPaused = false;
		m_playback = std::thread([this] { PlaybackThread(); });
	}
}

void Player::Stop()
{
	if (!m_isPaused)
	{
		m_isPaused = true;
		if (m_playback.joinable()) m_playback.join();
	}
}

FrameId Player::GetFrameId() const
{
	return m_frameId;
}

bool Player::IsPlaying() const
{
	return m_isPlaying;
}

bool Player::IsPaused() const
{
	return m_isPaused;
}

////////////////////////////////////////////////////////////////////////////////

void Player::PlaybackThread()
{
	auto hndl = reinterpret_cast<HANDLE>(m_playback.native_handle());
	SetThreadPriority(hndl, THREAD_PRIORITY_TIME_CRITICAL);

	auto framePeriod = 1.0 / m_module->playback.frameRate();
	bool firstFrame  = true;

	while (!m_isPaused && m_output.IsOpened())
	{
		double payload = GetTime();

		// play current frame
		const Frame& frame = m_module->frames.get(m_frameId);
		m_output.OutFrame(frame, firstFrame);
		firstFrame = false;

		// go to next frame
		m_frameId += m_playbackStep;
		if (int(m_frameId) < 0 || m_frameId > m_module->playback.lastFrameId())
		{
			m_isPlaying = false;
			break;
		}

		// next frame timestamp waiting
		payload = GetTime() - payload;
		WaitFor(framePeriod - payload);
	}

	// silence output when job is done
	m_output.OutFrame(Frame(), true);
}
