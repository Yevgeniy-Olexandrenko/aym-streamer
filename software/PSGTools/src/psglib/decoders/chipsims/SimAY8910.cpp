#include "SimAY8910.h"

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

void SimAY8910::Write(int chip, Register reg, uint8_t data)
{
	m_frame.Update(chip, reg, data);
}

void SimAY8910::Simulate(int samples)
{
	// do nothing
}

void SimAY8910::Convert(Frame& frame)
{
	frame = m_frame;
	m_frame.ResetChanges();
}
