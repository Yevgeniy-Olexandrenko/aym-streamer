#pragma once

#include "output/Output.h"
#include "SerialPort.h"

class Streamer : public Output
{
public:
	Streamer(int comPortIndex);
	virtual ~Streamer();
	std::string name() const override;

public:
	bool Open() override;
	bool Init(const Stream& stream) override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

private:
	Frame ProcessForAY8930(const Frame& frame) const;

private:
	int m_portIndex;
	SerialPort m_port;
};
