#include "Player.h"
#include "../output/Output.h"
#include "../module/Module.h"

Player::Player(Output& output)
	: m_output(output)
	, m_module(nullptr)
	, m_frame(0)
	, m_isMuted(false)
	, m_wasMuted(false)
{
	m_output.Open();
}

Player::~Player()
{
	Mute(true);
	m_output.Close();
}

bool Player::InitWithModule(const Module& module)
{
	Mute(true);
	m_module = nullptr;
	m_frame  = 0;

	if (m_output.IsOpened() && module.GetFrameCount() > 0)
	{
		m_module = &module;
		Mute(false);
		return true;
	}
	return false;
}

bool Player::PlayModuleFrame()
{
	if (m_output.IsOpened() && m_module)
	{
		if (m_isMuted) return true;

		if (m_frame == m_module->GetFrameCount())
		{
			if (m_module->HasLoopFrameIndex())
			{
				m_frame = m_module->GetLoopFrameIndex();
			}
			else
			{
				// no loop, so stop playback
				return false;
			}
		}

		const Frame& frame = m_module->GetFrame(m_frame);
		m_frame++;

		// frame output ignoring differential
		// mode just after unmute
		m_output.OutFrame(frame, m_wasMuted);
		m_wasMuted = false;

		return true;
	}
	return false;
}

void Player::Mute(bool on)
{
	if (m_isMuted != on)
	{
		if (on)
		{
			// silence output ignoring
			// differential mode
			m_output.OutFrame(Frame(), true);
		}

		m_wasMuted = m_isMuted;
		m_isMuted = on;
	}
}
