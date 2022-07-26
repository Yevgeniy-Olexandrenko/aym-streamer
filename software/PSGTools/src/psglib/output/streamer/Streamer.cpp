#include <thread>
#include "Streamer.h"
#include "stream/Frame.h"

Streamer::Streamer(int comPortIndex)
	: Output()
	, m_portIndex(comPortIndex)
{
}

Streamer::~Streamer()
{
	Close();
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
#if AY8930_FORCE_TO_CHOOSE
	m_chip.model(Chip::Model::AY8930);
#else
	m_chip.model(Chip::Model::Compatible);
#endif
	m_chip.count(Chip::Count::OneChip);
	m_chip.frequency(Chip::Frequency::F1773400);
	m_chip.channels(Chip::Channels::ABC);
	return true;
}

void Streamer::Close()
{
	Write(!Frame());
	m_port.Close();
}

void Streamer::WriteToChip(int chip, const std::vector<uint8_t>& data)
{
	auto dataSize = (int)data.size();
	auto dataBuff = (const char*)data.data();
	auto sentSize = m_port.SendBinary(dataBuff, dataSize);
	if (sentSize != dataSize) m_isOpened = false;
}

const std::string Streamer::GetOutputDeviceName() const
{
	return "Streamer";
}
