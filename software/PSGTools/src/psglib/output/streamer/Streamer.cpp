#include <thread>
#include "Streamer.h"
#include "stream/Stream.h"

Streamer::Streamer(int comPortIndex)
	: m_portIndex(comPortIndex)
{
}

Streamer::~Streamer()
{
	CloseDevice();
}

bool Streamer::OpenDevice()
{
	m_port.Open(m_portIndex);
	if (m_port.SetBaudRate(SerialPort::BaudRate::_57600))
	{
		// wait before AYM Streame become ready
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return true;
	}
	return false;
}

bool Streamer::InitDstChip(const Chip& srcChip, Chip& dstChip)
{
#if AY8930_FORCE_TO_CHOOSE
	dstChip.model(Chip::Model::AY8930);
#else
	dstChip.model(Chip::Model::Compatible);
#endif
	dstChip.count(Chip::Count::OneChip);
	dstChip.clock(Chip::Clock::F1773400);
	dstChip.output(Chip::Output::Stereo);
	dstChip.stereo(srcChip.stereoKnown() ? srcChip.stereo() : Chip::Stereo::ABC);
	return true;
}

bool Streamer::WriteToChip(int chip, const std::vector<uint8_t>& data)
{
	auto dataSize = (int)data.size();
	auto dataBuff = (const char*)data.data();
	auto sentSize = m_port.SendBinary(dataBuff, dataSize);
	return (sentSize == dataSize);
}

const std::string Streamer::GetDeviceName() const
{
	return "Streamer";
}

void Streamer::CloseDevice()
{
	Write(!Frame());
	m_port.Close();
}
