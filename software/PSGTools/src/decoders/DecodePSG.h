#pragma once

#include "Decode.h"

class DecodePSG : public Decoder
{
	#pragma pack(push, 1)
	struct PSGHeader
	{
		uint8_t m_psg[3];
		uint8_t m_1Ah;
		uint8_t m_ver;
		uint8_t m_fps;
		uint8_t m_bin[10];
	};
	#pragma pack(pop)

public:
	bool Open   (Stream& stream) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Stream& stream) override;

private:
	std::ifstream m_fileStream;
	int m_skipFrames = 0;
};