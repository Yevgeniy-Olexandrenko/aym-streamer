#pragma once

#include "ChipSim.h"

class SimSN76489 : public ChipSim
{
public:
	SimSN76489();

	void Reset();
	void Write(uint8_t reg, uint8_t data);
	void Simulate(int samples);
	void ConvertToPSG(Frame& frame);
	void PostProcess(Stream& stream);
};