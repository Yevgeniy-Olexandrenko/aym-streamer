#pragma once

#include <fstream>
#include "stream/Stream.h"

class Encoder
{
public:
	virtual bool Open(const Stream& stream) = 0;
	virtual void Encode(FrameId id, const Frame& frame) = 0;
	virtual void Close(const Stream& stream) = 0;

protected:
	bool CheckFileExt(const Stream& stream, const std::string& ext) const;
};
