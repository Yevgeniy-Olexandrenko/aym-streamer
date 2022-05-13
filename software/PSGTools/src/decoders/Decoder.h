#pragma once

#include <string>
#include <fstream>
#include <stdint.h>
#include "stream/Stream.h"

class Decoder
{
public:
	virtual bool Open  (Stream& stream) = 0;
	virtual bool Decode(Frame&  frame ) = 0;
	virtual void Close (Stream& stream) = 0;

protected:
	bool CheckFileExt(const Stream& stream, const std::string& ext) const;
	std::string ReadString(uint8_t* ptr, uint8_t size) const;
};

class ModuleDecoder : public Decoder
{
public:
	bool Decode(Frame&  frame ) override;
	void Close (Stream& stream) override;

protected:
	virtual void Init() = 0;
	virtual void Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition);
	virtual bool Play() = 0;

protected:
	bool m_isTS = false;

	FrameId m_loop  = 0;
	FrameId m_frame = 0;

	uint8_t* m_data = nullptr;
	uint8_t  m_regs[2][16];
};
