#pragma once

#include "decoders/Decoder.h"

class DecodePSG : public Decoder
{
	#pragma pack(push, 1)
	struct Header
	{
		uint32_t m_sig;
		uint8_t  m_ver;
		uint8_t  m_fps;
		uint8_t  m_bin[10];
	};
	#pragma pack(pop)

public:
	bool Open   (Stream& stream) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Stream& stream) override;

private:
	std::ifstream m_input;
	int m_skip;
};