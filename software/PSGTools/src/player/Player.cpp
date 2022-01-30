#include "Player.h"
#include "../module/Module.h"

Player::Player(int comPortIndex)
	: m_comPort(nullptr)
	, m_module(nullptr)
	, m_frame(0)
	, m_isMuted(false)
	, m_wasMuted(false)
{
	m_comPort = OpenPort(comPortIndex);

	if (m_comPort)
	{
		SetPortBoudRate(m_comPort, CP_BOUD_RATE_57600);
	}
}

Player::~Player()
{
	if (m_comPort)
	{
		Mute(true);
		ClosePort(m_comPort);
	}
}

bool Player::InitWithModule(const Module& module)
{
	Mute(true);
	m_module = nullptr;
	m_frame  = 0;

	if (m_comPort && module.GetFrameCount() > 0)
	{
		m_module = &module;
		Mute(false);
		return true;
	}
	return false;
}

bool Player::PlayModuleFrame()
{
	if (m_comPort && m_module)
	{
		if (m_isMuted) return true;

		if (m_frame < m_module->GetFrameCount())
		{
			const Frame& frame = m_module->GetFrame(m_frame);
			m_frame++;

			// frame output ignoring differential
			// mode just after unmute
			OutFrame(frame, m_wasMuted);
			m_wasMuted = false;

			return true;
		}
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
			OutFrame(Frame(), true);
		}

		m_wasMuted = m_isMuted;
		m_isMuted = on;
	}
}

void Player::OutFrame(const Frame& frame, bool force)
{
	if (m_comPort)
	{
		char buffer[size_t(Register::Index::COUNT) * 2 + 1];
		int  bufPtr = 0;

		for (size_t i = 0; i < size_t(Register::Index::COUNT); ++i)
		{
			const Register& reg = frame[i];
			if (force || reg.IsChanged())
			{
				// register number and value
				buffer[bufPtr++] = char(i);
				buffer[bufPtr++] = char(reg.GetData());
			}
		}

		// frame end marker
		buffer[bufPtr++] = char(0xFF);
		int wasSent = SendData(m_comPort, buffer, bufPtr);

		if (wasSent != bufPtr)
		{
			std::cout << wasSent << " != " << bufPtr << std::endl;
		}
	}
}


