#pragma once

#include "encoders/Encoder.h"

class EncodeTXT : public Encoder
{
public:
	bool Open  (const Stream& stream) override;
	void Encode(const Frame&  frame ) override;
	void Close (const Stream& stream) override;

private:
	//
};