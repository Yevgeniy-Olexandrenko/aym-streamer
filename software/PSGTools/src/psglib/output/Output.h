#pragma once

#include <vector>
#include "Processing.h"

#define AY8930_FORCE_TO_CHOOSE 0

class Stream;
class Frame;

class Output
{
public:
	Output();
	virtual ~Output();
	std::string toString() const;

	virtual bool Open() = 0;
	virtual bool Init(const Stream& stream);
	virtual bool Write(const Frame& frame);
	virtual void Close() = 0;

protected:
	virtual void WriteToChip(int chip, const Frame& frame);
	virtual void WriteToChip(int chip, const std::vector<uint8_t>& data) = 0;
	virtual const std::string GetOutputDeviceName() const = 0;

protected:
	bool m_isOpened;
	Chip m_chip;

private:
	FixAY8930Envelope m_fixAY8930Envelope;
	ConvertExpToComp  m_convertExpToComp;
	SwapChannels      m_swapChannels;
	DisableChannels   m_disableChannels;
};
