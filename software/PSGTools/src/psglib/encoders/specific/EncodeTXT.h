#pragma once

#include "encoders/Encoder.h"

enum class DisplayType
{
	Dump, Grouped, Readable
};

class EncodeTXT : public Encoder
{
public:
	bool Open(const Stream& stream) override;
	void Encode(FrameId id, const Frame& frame) override;
	void Close(const Stream& stream) override;

private:
	void PrintFrameHeader();
	void PrintChipRegisters(const Frame& frame, int chip);

	void PrintChipHeader();
	void PrintFrameRegisters(FrameId id, const Frame& frame);
	void PrintDelimiter();

	void PrintNibble(uint8_t nibble);
	void PrintChipRegister(const Frame& frame, int chip, int reg);
	void PrintChipRegisterDelta(const Frame& frame, int chip, int reg, int bits);
	void PrintChannelMixer(const Frame& frame, int chip, int chan);

private:
	bool m_isTS;
	Frame m_prevFrame;
	FrameId m_loop;
	FrameRate m_frameRate;
	std::ofstream m_output;
	DisplayType m_displayType;
};