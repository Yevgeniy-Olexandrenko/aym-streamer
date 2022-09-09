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

const std::string Streamer::GetDeviceName() const
{
	return "Streamer";
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
	dstChip.first.model(Chip::Model::AY8930);
#else
	dstChip.first.model(Chip::Model::Compatible);
#endif
	dstChip.clock(Chip::Clock::F1773400);
	dstChip.output(Chip::Output::Stereo);
	dstChip.stereo(srcChip.stereoKnown() ? srcChip.stereo() : Chip::Stereo::ABC);
	return true;
}

bool Streamer::WriteToChip(int chip, const Data& data)
{
	// prepare
	std::vector<uint8_t> binary(64);
	for (const auto& pair : data)
	{
		const uint8_t& reg = pair.first;
		const uint8_t& val = pair.second;
		binary.push_back(reg);
		binary.push_back(val);
	}
	binary.push_back(0xFF);

	// send
	auto dataSize = (int)binary.size();
	auto dataBuff = (const char*)binary.data();
	auto sentSize = m_port.SendBinary(dataBuff, dataSize);
	return (sentSize == dataSize);
}

void Streamer::CloseDevice()
{
	Write(!Frame());
	m_port.Close();
}
