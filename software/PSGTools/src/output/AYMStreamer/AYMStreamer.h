#pragma once

#include "output/Output.h"
#include "SerialPort.h"

class AYMStreamer : public Output
{
public:
	AYMStreamer(const Module& module, int comPortIndex);
	virtual ~AYMStreamer();

public:
	void Open() override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

private:
	int m_portIndex;
	SerialPort m_port;
};
