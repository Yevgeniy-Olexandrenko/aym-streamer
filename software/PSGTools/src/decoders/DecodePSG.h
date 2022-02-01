#pragma once

#include <fstream>
#include "Decoder.h"

class DecodePSG : public Decoder
{
public:
	bool Open   (Module& module) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Module& module) override;

private:
	std::ifstream m_fileStream;
	int m_skipFrames = 0;
};