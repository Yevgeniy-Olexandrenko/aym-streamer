#include <thread>
#include "AYMStreamer.h"
#include "module/Frame.h"

AYMStreamer::AYMStreamer(int comPortIndex)
	: m_portIndex(comPortIndex)
{
}

AYMStreamer::~AYMStreamer()
{
	Close();
}

bool AYMStreamer::Open()
{
	m_port.Open(m_portIndex);
	if (m_isOpened = m_port.SetBaudRate(SerialPort::BaudRate::_57600))
	{
		// wait before AYM Streame become ready
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	return m_isOpened;
}

bool AYMStreamer::Init(const Module& module)
{
	// do nothing for now
	return true;
}

bool AYMStreamer::OutFrame(const Frame& frame, bool force)
{
	if (m_isOpened)
	{
		char buffer[16 * 2 + 1];
		int  bufPtr = 0;

		for (uint8_t i = 0; i < 16; ++i)
		{
			const Register& reg = frame[i].first;
			if (force || reg.changed())
			{
				// register number and value
				buffer[bufPtr++] = char(i);
				buffer[bufPtr++] = char(reg.data());
			}
		}

		// frame end marker
		buffer[bufPtr++] = char(0xFF);
		int bytesSent = m_port.SendBinary(buffer, bufPtr);
		if (bytesSent != bufPtr) m_isOpened = false;
	}
	return m_isOpened;
}

void AYMStreamer::Close()
{
	m_port.Close();
}
