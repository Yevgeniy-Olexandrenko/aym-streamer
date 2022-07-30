#pragma once

#include "output/Output.h"
#include "SerialPort.h"

class Streamer : public Output
{
public:
	Streamer(int comPortIndex);
	virtual ~Streamer();

protected:
	bool OpenDevice() override;
	bool InitDstChip(const Chip& srcChip, Chip& dstChip) override;
	bool WriteToChip(int chip, const std::vector<uint8_t>& data) override;
	const std::string GetDeviceName() const override;
	void CloseDevice() override;

private:
	int m_portIndex;
	SerialPort m_port;
};
