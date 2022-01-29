#pragma once

#include "../Decoder.h"

class DecodePSG : public Decoder
{
public:
	bool InitModule(std::ifstream& file, Module& module) override;
	bool DecodeFrame(std::ifstream& file, Frame& frame) override;

private:
	int m_skipFrames;
};