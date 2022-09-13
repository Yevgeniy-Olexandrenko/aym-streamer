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

private:
	uint8_t ConvertVolume(uint8_t  volume) const;

private:
	struct SN76489_t
	{
		int Registers[8];
		int LatchedRegister;
		int NoiseShiftRegister;
		int NoiseFreq;
	};

	SN76489_t SN76489;
};