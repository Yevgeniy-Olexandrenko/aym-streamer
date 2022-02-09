#pragma once

#include "Decoder.h"
#include "module/Module.h"

class DecodeVTX : public Decoder
{
	#pragma pack(push, 1)
	struct VTXHeader
	{
		uint16_t signature;
		uint8_t  stereo;
		uint16_t loop;
		uint32_t chipFreq;
		uint8_t  frameFreq;
		uint16_t year;
		uint32_t dataSize;
	};
	#pragma pack(pop)

	enum VTXStereo
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
	bool Open   (Module& module) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Module& module) override;

private:
	//

private:
	FrameId m_loopFrameId;
};