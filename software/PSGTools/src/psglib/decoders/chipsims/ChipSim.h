#pragma once

#include "stream/Frame.h"

class ChipSim
{
public:
	enum class Type { AY8910, RP2A03, SN76489 };

	ChipSim(Type type) : m_type(type) {}
	Type type() const { return m_type; }

	virtual void ConfigureClock(int srcClock, int dstClock)
	{
		m_clockRatio = float(dstClock) / float(srcClock);
	}

public:
	virtual void Reset() = 0;
	virtual void Write(int chip, Register reg, uint8_t data) = 0;
	virtual void Simulate(int samples) = 0;
	virtual void Convert(Frame& frame) = 0;

protected:
	uint16_t ConvertPeriod(uint16_t period) const
	{
		auto converted = period * m_clockRatio;
		return uint16_t(converted + 0.5f);
	}

	void EnableTone(uint8_t& mixer, int chan) const
	{
		mixer &= ~(1 << chan);
	}

	void EnableNoise(uint8_t& mixer, int chan) const
	{
		mixer &= ~(1 << (3 + chan));
	}

protected:
	Type  m_type;
	float m_clockRatio;
};
