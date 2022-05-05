#include "SimSN76489.h"

SimSN76489::SimSN76489()
	: ChipSim(Type::SN76489)
{
	Reset();
}

void SimSN76489::Reset()
{
	// TODO
}

void SimSN76489::Write(uint8_t reg, uint8_t data)
{
	// TODO
}

void SimSN76489::Simulate(int samples)
{
	// TODO
}

void SimSN76489::ConvertToPSG(Frame& frame)
{
	// TODO
}

void SimSN76489::PostProcess(Stream& stream)
{
	// TODO
}
