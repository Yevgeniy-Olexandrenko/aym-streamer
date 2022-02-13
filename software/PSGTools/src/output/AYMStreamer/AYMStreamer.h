#pragma once

#include "output/Output.h"
#include "SerialPort.h"

class AYMStreamer : public Output
{
public:
	AYMStreamer(int comPortIndex);
	virtual ~AYMStreamer();

public:
	bool Open() override;
	bool Init(const Module& module) override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

private:
	int m_portIndex;
	SerialPort m_port;
};
