#include "AYMStreamer.h"
#include "../../module/Frame.h"

AYMStreamer::AYMStreamer(int portIndex)
	: m_portIndex(portIndex)
	, m_isOpened(false)
{
}

AYMStreamer::~AYMStreamer()
{
	Close();
}

void AYMStreamer::Open()
{
	m_port.Open(m_portIndex);
	m_isOpened = m_port.SetBaudRate(SerialPort::BaudRate::_57600);
}

bool AYMStreamer::IsOpened() const
{
	return m_isOpened;
}

bool AYMStreamer::OutFrame(const Frame& frame, bool force)
{
	if (m_isOpened)
	{
		char buffer[16 * 2 + 1];
		int  bufPtr = 0;

		for (uint8_t i = 0; i < 16; ++i)
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
		if (bytesSent != bufPtr) m_isOpened = false;
	}
	return m_isOpened;
}

void AYMStreamer::Close()
{
	m_port.Close();
}
