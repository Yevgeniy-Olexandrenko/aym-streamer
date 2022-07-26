#pragma once

#include <vector>
#include "Stream/Chip.h"

class Stream;
class Frame;

class Output
{
public:
	Output();
	std::string toString() const;

	virtual bool Open() = 0;
	virtual bool Init(const Stream& stream) = 0;
	virtual bool Write(const Frame& frame);
	virtual void Close() = 0;

protected:
	virtual void WriteToChip(int chip, const Frame& frame);
	virtual void WriteToChip(int chip, const std::vector<uint8_t>& data) = 0;
	virtual const std::string GetOutputDeviceName() const = 0;

protected:
	bool m_isOpened;
	Chip m_chip;
};
