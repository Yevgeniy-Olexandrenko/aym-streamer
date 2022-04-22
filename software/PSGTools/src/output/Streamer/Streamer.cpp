#include <thread>
#include "Streamer.h"
#include "stream/Frame.h"

Streamer::Streamer(int comPortIndex)
	: m_portIndex(comPortIndex)
{
}

Streamer::~Streamer()
{
	Close();
}

std::string Streamer::name() const
{
	return ("Streamer -> " + chip.toString());
}

bool Streamer::Open()
{
	m_port.Open(m_portIndex);
	if (m_isOpened = m_port.SetBaudRate(SerialPort::BaudRate::_57600))
	{
		// wait before AYM Streame become ready
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
	return m_isOpened;
}

bool Streamer::Init(const Stream& stream)
{
	chip.count(Chip::Count::SingleChip);
	chip.model(Chip::Model::Compatible);
	chip.frequency(Chip::Frequency::F1773400);
	chip.channels(Chip::Channels::ABC);
	return true;
}

bool Streamer::OutFrame(const Frame& frame, bool force)
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

void Streamer::Close()
{
	m_port.Close();
}

