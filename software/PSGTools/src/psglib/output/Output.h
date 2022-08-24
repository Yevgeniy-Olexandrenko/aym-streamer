#pragma once

#include <vector>
#include "Processing.h"

#define AY8930_FORCE_TO_CHOOSE  0
#define AY8930_FORCE_TO_DISCARD 0

class Stream;
class Frame;

class Output : private Processing
{
public:
	Output();
	virtual ~Output();
	
	bool Open();
	bool Init(const Stream& stream);
	bool Write(const Frame& frame);
	void Close();

	const Frame& GetFrame() const;
	std::string toString() const;

protected:
	virtual bool OpenDevice() = 0;
	virtual bool InitDstChip(const Chip& srcChip, Chip& dstChip) = 0;
	virtual bool WriteToChip(int chip, const std::vector<uint8_t>& data) = 0;
	virtual const std::string GetDeviceName() const = 0;
	virtual void CloseDevice() = 0;

private:
	void Reset() override;
	const Frame& operator()(const Chip& chip, const Frame& frame) override;

private:
	bool m_isOpened;
	Chip m_chip;

	FixAY8930Envelope m_fixAY8930Envelope;
	ConvertExpToComp  m_convertExpToComp;
	ConvertToNewClock m_convertToNewClock;
	SwapChannelsOrder m_swapChannelsOrder;
	DisableChannels   m_disableChannels;
};
