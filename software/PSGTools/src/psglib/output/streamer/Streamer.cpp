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

bool Streamer::ConfigureChip(const Chip& schip, Chip& dchip)
{
	dchip.first.model(Chip::Model::Compatible);
	dchip.second.model(Chip::Model::Unknown);

	dchip.clock(Chip::Clock::F1773400);
	dchip.output(Chip::Output::Stereo);

	if (!dchip.stereoKnown())
	{
		dchip.stereo(schip.stereoKnown() ? schip.stereo() : Chip::Stereo::ABC);
	}

	return true;
}

bool Streamer::WriteToChip(int chip, const Data& data)
{
	// prepare packet
	std::vector<uint8_t> binary(64);
	for (const auto& pair : data)
	{
		const uint8_t& reg = pair.first;
		const uint8_t& val = pair.second;
		binary.push_back(reg);
		binary.push_back(val);
	}
	binary.push_back(0xFF);

	// send packet
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
