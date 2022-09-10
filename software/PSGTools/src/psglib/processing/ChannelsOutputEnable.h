#pragma once

#include <array>
#include "Processing.h"

class ChannelsOutputEnable : public Processing
{
    Chip m_chip;
    std::array<bool, 5> m_enables;

public:
    ChannelsOutputEnable(const Chip& dstChip)
        : m_chip(dstChip)
    {
        for (auto& enable : m_enables) enable = true;
    }

    void SetEnables(const std::array<bool, 5>& enables)
    {
        m_enables = enables;
    }

#ifdef Enable_ChannelsOutputEnable
	const Frame& operator()(const Frame& frame) override
    {
        for (const auto& enable : m_enables)
        {
            if (!enable)
            {
                const auto ApplyEnable = [&](int chip, int chan, bool enableT, bool enableN, bool enableE)
                {
                    uint8_t mixer = m_frame[chip].Read(Mixer);
                    uint8_t vol_e = m_frame[chip].Read(A_Volume + chan);

                    // disable tone and noise
                    if (!enableT) mixer |= m_frame[chip].tmask() << chan;
                    if (!enableN) mixer |= m_frame[chip].nmask() << chan;

                    // disable envelope and volume if possible
                    if (!enableT && !enableN) vol_e &= m_frame[chip].emask();
                    if (!enableE) vol_e &= ~m_frame[chip].emask();

                    m_frame[chip].Update(Mixer, mixer);
                    m_frame[chip].Update(A_Volume + chan, vol_e);
                };

                Update(frame);
                for (int chip = 0; chip < m_chip.count(); ++chip)
                {
                    for (int chan = 0; chan < 3; ++chan)
                    {
                        ApplyEnable(chip, chan, m_enables[chan], m_enables[3], m_enables[4]);
                    }
                }
                return m_frame;
            }
        }
        return frame;
    }
#endif
};
