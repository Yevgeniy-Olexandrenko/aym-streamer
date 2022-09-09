#pragma once

#include "Processing.h"

class ChipClockRateConvert : public Processing
{
    int m_srcClockRate;
    int m_dstClockRate;

public:
    ChipClockRateConvert(const Chip& schip, const Chip& dchip)
        : m_srcClockRate(schip.clockValue())
        , m_dstClockRate(dchip.clockValue())
    {}

#ifdef Enable_ChipClockRateConvert
	const Frame& operator()(const Frame& frame) override
    {
        if (m_dstClockRate != m_srcClockRate)
        {
            // TODO
        }
        return frame;
    }
#endif
};
