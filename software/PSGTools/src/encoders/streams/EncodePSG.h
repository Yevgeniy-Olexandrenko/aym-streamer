#pragma once

#include "encoders/Encoder.h"

class EncodePSG : public Encoder
{
public:
	bool Open(const Stream& stream) override;
	void Encode(FrameId id, const Frame& frame) override;
	void Close(const Stream& stream) override;

private:
	void WriteFrameBeginOrSkip();

private:
	std::ofstream m_output;
	uint16_t m_skip;
	bool m_isTS;
};