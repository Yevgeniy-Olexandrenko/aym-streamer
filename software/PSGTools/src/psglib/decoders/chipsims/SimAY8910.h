#pragma once

#include "ChipSim.h"
#include "stream/Frame.h"

class SimAY8910 : public ChipSim
{
public:
	SimAY8910();

	void Reset();
	void Write(uint8_t chip, uint8_t reg, uint8_t data);
	void Simulate(int samples);
	void ConvertToPSG(Frame& frame);
	void PostProcess(Stream& stream);

private:
	Frame m_frame;
};