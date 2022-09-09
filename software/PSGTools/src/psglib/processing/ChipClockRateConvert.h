#pragma once

#include "Processing.h"

class ChipClockRateConvert : public Processing
{
    float m_ratio;
    int   m_count;

public:
    ChipClockRateConvert(const Chip& schip, const Chip& dchip)
        : m_ratio(float(dchip.clockValue()) / float(schip.clockValue()))
        , m_count(dchip.count())
    {}

#ifdef Enable_ChipClockRateConvert
	const Frame& operator()(const Frame& frame) override
    {
        static const PeriodRegister c_com[]
        {
            A_Period, B_Period, C_Period,
            E_Period, N_Period
        };

        static const PeriodRegister c_exp[]
        {
            A_Period,  B_Period,  C_Period,
            EA_Period, EB_Period, EC_Period,
            N_Period
        };

        if (m_ratio != 1.0f)
        {
            Update(frame);
            for (int chip = 0; chip < m_count; ++chip)
            {
                bool isExpMode = m_frame[chip].IsExpMode();
                for (size_t i = 0; i < (isExpMode ? sizeof(c_exp) : sizeof(c_com)); ++i)
                {
                    PeriodRegister preg = (isExpMode ? c_exp[i] : c_com[i]);
                    uint16_t period = m_frame[chip].ReadPeriod(preg);

                    period = uint16_t(period * m_ratio + 0.5f);
                    m_frame[chip].UpdatePeriod(preg, period);
                }
            }
            return m_frame;
        }
        return frame;
    }
#endif
};
