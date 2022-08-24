#pragma once

#include "ChipSim.h"

class SimSN76489 : public ChipSim
{
public:
	SimSN76489();

	void Reset() override;
	void Write(int chip, Register reg, uint8_t data) override;
	void Simulate(int samples) override;
	void Convert(Frame& frame) override;
};