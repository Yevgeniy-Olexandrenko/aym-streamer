#pragma once

#include "Decode.h"

class DecodeVTX : public Decoder
{
	#pragma pack(push, 1)
	struct Header
	{
		uint16_t signature; //
		uint8_t  stereo;    //
		uint16_t loop;      //
		uint32_t chipFreq;  //
		uint8_t  frameFreq; //
		uint16_t year;      // store in extras
		uint32_t dataSize;  //
	};
	#pragma pack(pop)

	enum Stereo
	{
		MONO = 0x00,
		ABC  = 0x01,
		ACB  = 0x02,
		BAC  = 0x03,
		BCA  = 0x04,
		CAB  = 0x05,
		CBA  = 0x06
	};

public:
	bool Open   (Stream& stream) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Stream& stream) override;

private:
	uint8_t* m_data;
	FrameId  m_loopFrame;
	FrameId  m_nextFrame;
	uint32_t m_frameCount;
};