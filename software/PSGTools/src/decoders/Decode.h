#pragma once

#include <string>
#include <fstream>
#include <stdint.h>

class Stream;
class Frame;

class Decoder
{
public:
	virtual bool Open  (Stream& stream) = 0;
	virtual bool Decode(Frame&  frame ) = 0;
	virtual void Close (Stream& stream) = 0;
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
	std::string ReadString(uint8_t* ptr, uint8_t size) const;

protected:
	uint8_t* m_data = nullptr;
	uint32_t m_loop = 0;
	uint32_t m_tick = 0;
	uint8_t  m_regs[16];
};
