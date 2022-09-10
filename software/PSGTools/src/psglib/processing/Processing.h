#pragma once

#include "stream/Chip.h"
#include "stream/Frame.h"

#define Enable_ChannelsOutputEnable
#define Enable_ChipClockRateConvert
#define Enable_ChannelsLayoutChange
#define Enable_AY8930EnvelopeFix

class Processing
{
public:
	virtual void  Reset();
	virtual void  Update(const Frame& frame);
	virtual const Frame& operator()(const Frame& frame);

protected:
	Frame m_frame;
};
