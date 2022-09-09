#pragma once

#include "Processing.h"

class ChipClockRateConvert : public Processing
{
    int m_srcClockRate;
    int m_dstClockRate;

public:
    ChipClockRateConvert(const Chip& srcChip, const Chip& dstChip)
        : m_srcClockRate(srcChip.clockValue())
        , m_dstClockRate(dstChip.clockValue())
    {}

#ifdef Enable_ChipClockRateConvert
	const Frame& operator()(const Frame& frame) override
    {
        // TODO

        return frame;
    }
#endif
};
