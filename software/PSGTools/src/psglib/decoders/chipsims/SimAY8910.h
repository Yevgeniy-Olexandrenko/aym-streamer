#pragma once

#include "ChipSim.h"

class SimAY8910 : public ChipSim
{
public:
	SimAY8910();

	void Reset() override;
	void Write(int chip, Register reg, uint8_t data) override;
	void Simulate(int samples) override;
	void Convert(Frame& frame) override;

private:
	Frame m_frame;
};