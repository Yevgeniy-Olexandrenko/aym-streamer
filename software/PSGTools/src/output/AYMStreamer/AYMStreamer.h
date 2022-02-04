#pragma once

#include "../Output.h"
#include "SerialPort.h"

class AYMStreamer : public Output
{
public:
	AYMStreamer(int comPortIndex);
	~AYMStreamer();

public:
	void Open() override;
	bool IsOpened() const override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

private:
	int m_portIndex;
	SerialPort m_port;
	bool m_isPortOK;
};
