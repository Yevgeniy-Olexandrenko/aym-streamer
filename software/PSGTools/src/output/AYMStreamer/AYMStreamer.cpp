#include "AYMStreamer.h"
#include "../../module/Frame.h"

AYMStreamer::AYMStreamer(int portIndex)
	: m_portIndex(portIndex)
	, m_isPortOK(false)
{
}

AYMStreamer::~AYMStreamer()
{
	Close();
}

void AYMStreamer::Open()
{
	m_port.Open(m_portIndex);
	m_isPortOK = m_port.SetBaudRate(SerialPort::BaudRate::_57600);
}

bool AYMStreamer::IsOpened() const
{
	return m_isPortOK;
}

bool AYMStreamer::OutFrame(const Frame& frame, bool force)
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
		if (bytesSent != bufPtr) m_isPortOK = false;
	}
	return m_isPortOK;
}

void AYMStreamer::Close()
{
	m_port.Close();
}
