#pragma once

#include "Decoder.h"

class DecodePT3 : public Decoder
{
public:
	bool Open   (Module& module) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Module& module) override;
};