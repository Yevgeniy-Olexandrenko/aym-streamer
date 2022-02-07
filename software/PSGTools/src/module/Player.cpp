#include "Player.h"
#include "../output/Output.h"

Player::Player(Output& output)
	: m_output(output)
	, m_module(nullptr)
	, m_step(+1)
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

bool Player::Init(const Module& module)
{
	Stop();
	m_isPlaying = false;

	if (m_output.IsOpened() && module.GetFrameCount() > 0)
	{
		m_module = &module;
		m_isPlaying = true;
		m_frameId = 0;
	}

	return m_isPlaying;
}

void Player::Step(int step)
{
	m_step = step;
}

void Player::Play()
{
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

void Player::PlaybackThread()
{
	Time timestamp = Clock::now();
	Duration framePeriod(1.0 / m_module->GetFrameRate());

	bool firstFrame = true;
	while (!m_isPaused && m_output.IsOpened())
	{
		Time now = Clock::now();
		Duration timeSpan = std::chrono::duration_cast<Duration>(now - timestamp);

		if (timeSpan >= framePeriod)
		{
			timestamp = now;

			const Frame& frame = m_module->GetFrame(m_frameId);
			m_output.OutFrame(frame, firstFrame);
			firstFrame = false;

			if (!GotoNextFrame())
			{
				m_isPlaying = false;
				break;
			}
		}
		std::this_thread::yield();
	}
	m_output.OutFrame(Frame(), true);
}

bool Player::GotoNextFrame()
{
	int step = m_step;
	if (step > 0)
	{
		// move forward
		if ((m_frameId += step) >= m_module->GetFrameCount())
		{
			if (!m_module->HasLoop()) return false;
			m_frameId = m_module->GetLoopFrameId();
		}
	}
	else if (step < 0)
	{
		// move backward
		int firstId = m_module->HasLoop() ? m_module->GetLoopFrameId() : 0;
		int frameId = int(m_frameId) + step;
		
		if (frameId < firstId)
		{
			if (frameId < 0) return false;
			if (m_frameId >= firstId) 
				frameId = m_module->GetFrameCount() - 1;
		}
		m_frameId = FrameId(frameId);
	}
	return true;
}
