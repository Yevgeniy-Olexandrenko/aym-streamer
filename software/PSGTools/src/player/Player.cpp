#include "Player.h"
#include "../module/Module.h"

Player::Player(int comPortIndex)
	: m_isPortOK(false)
	, m_module(nullptr)
	, m_frame(0)
	, m_isMuted(false)
	, m_wasMuted(false)
{
	m_port.Open(comPortIndex);
	m_isPortOK =  m_port.SetBaudRate(SerialPort::BaudRate::_57600);
}

Player::~Player()
{
	Mute(true);
	m_port.Close();
}

bool Player::InitWithModule(const Module& module)
{
	Mute(true);
	m_module = nullptr;
	m_frame  = 0;

	if (m_isPortOK && module.GetFrameCount() > 0)
	{
		m_module = &module;
		Mute(false);
		return true;
	}
	return false;
}

bool Player::PlayModuleFrame()
{
	if (m_isPortOK && m_module)
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
	if (m_isPortOK)
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
		int bytesSent = m_port.SendBinary(buffer, bufPtr);

		if (bytesSent != bufPtr)
		{
			m_isPortOK = false;
		}
	}
}


