#pragma once

#include <stdint.h>

class Frame;
class Stream;

class ChipSim
{
public:
	enum class Type { AY8910, RP2A03, SN76489 };

	ChipSim(Type type) : m_type(type) {}
	Type type() const { return m_type; }

public:
	virtual void Reset() = 0;
	virtual void Write(uint8_t chip, uint8_t reg, uint8_t data) = 0;
	virtual void Simulate(int samples) = 0;
	virtual void ConvertToPSG(Frame& frame) = 0;
	virtual void PostProcess(Stream& stream) = 0;

private:
	Type m_type;
};
