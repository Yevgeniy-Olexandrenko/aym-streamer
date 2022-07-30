#pragma once

#include <vector>
#include "stream/Processing.h"

#define AY8930_FORCE_TO_CHOOSE 0

class Stream;
class Frame;

class Output : private Processing
{
public:
	Output();
	virtual ~Output();
	
	virtual bool Open() = 0;
	virtual bool Init(const Stream& stream);
	virtual bool Write(const Frame& frame);
	virtual void Close() = 0;

	const Frame& GetFrame() const;
	std::string toString() const;

protected:
	virtual void WriteToChip(int chip, const Frame& frame);
	virtual void WriteToChip(int chip, const std::vector<uint8_t>& data) = 0;
	virtual const std::string GetOutputDeviceName() const = 0;

private:
	void Reset() override;
	const Frame& operator()(const Chip& chip, const Frame& frame) override;

protected:
	bool m_isOpened;
	Chip m_chip;

private:
	FixAY8930Envelope m_fixAY8930Envelope;
	ConvertExpToComp  m_convertExpToComp;
	ConvertToNewClock m_convertToNewClock;
	SwapChannelsOrder m_swapChannelsOrder;
	DisableChannels   m_disableChannels;
};
