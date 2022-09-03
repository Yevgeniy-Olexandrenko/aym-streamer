#pragma once

#include "stream/Chip.h"
#include "stream/Frame.h"

//#define Enable_FixAY8930Envelope
//#define Enable_ConvertExpToComp
//#define Enable_ConvertToNewClock
//#define Enable_DisableChannels
//#define Enable_SwapChannelsOrder

class Processing
{
public:
	virtual void Reset();
	virtual void Update(const Frame& frame);
	virtual const Frame& operator()(const Chip& chip, const Frame& frame) = 0;

protected:
	Frame m_frame;
};

class FixAY8930Envelope : public Processing
{
public:
	const Frame& operator()(const Chip& chip, const Frame& frame) override;
};

class ConvertExpToComp : public Processing
{
public:
	const Frame& operator()(const Chip& chip, const Frame& frame) override;
};

class ConvertToNewClock : public Processing
{
public:
	const Frame& operator()(const Chip& chip, const Frame& frame) override;
};

class SwapChannelsOrder : public Processing
{
public:
	const Frame& operator()(const Chip& chip, const Frame& frame) override;
};

class DisableChannels : public Processing
{
public:
	const Frame& operator()(const Chip& chip, const Frame& frame) override;
};
