#include "SimAY8910.h"
#include "stream/Frame.h"
#include <cstring>

SimAY8910::SimAY8910()
	: ChipSim(Type::AY8910)
{
	Reset();
}

void SimAY8910::Reset()
{
	m_frame.ResetData();
	m_frame.ResetChanges();
}

void SimAY8910::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
	m_frame.Update(chip, reg, data);
}

void SimAY8910::Simulate(int samples)
{
	// do nothing
}

void SimAY8910::ConvertToPSG(Frame& frame)
{
	frame = m_frame;
	m_frame.ResetChanges();
}

void SimAY8910::PostProcess(Stream& stream)
{
	// do nothing
}
