#pragma once

#include <stdint.h>
#include <string>

class Module;
class Frame;

class Decoder
{
public:
	virtual bool Open  (Module& module) = 0;
	virtual bool Decode(Frame&  frame ) = 0;
	virtual void Close (Module& module) = 0;
};

class ModuleDecoder : public Decoder
{
public:
	bool Decode(Frame&  frame ) override;
	void Close (Module& module) override;

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
