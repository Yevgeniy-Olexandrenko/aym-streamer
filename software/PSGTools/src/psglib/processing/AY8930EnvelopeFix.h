#pragma once

#include "Processing.h"

class AY8930EnvelopeFix : public Processing
{
    Chip m_chip;

public:
    AY8930EnvelopeFix(Chip dstChip)
        : m_chip(dstChip)
    {}

#ifdef Enable_AY8930EnvelopeFix
	const Frame& operator()(const Frame& frame) override
    {
        Update(frame);
        for (int chip = 0; chip <  m_chip.count(); ++chip)
        {
            if (m_chip.model(chip) == Chip::Model::AY8930)
            {
                for (int chan = 0; chan < 3; ++chan)
                {
                    uint8_t mixer = m_frame[chip].Read(Mixer) >> chan;
                    uint8_t vol_e = m_frame[chip].Read(A_Volume + chan);

                    // envelope flag position correction
                    if (m_frame[chip].IsExpMode()) vol_e >>= 1;

                    bool enableT = !(mixer & 0x01);
                    bool enableN = !(mixer & 0x08);
                    bool enableE =  (vol_e & 0x10);

                    if (enableE && !(enableT || enableN))
                    {
                        // enable tone and set period to zero
                        mixer = m_frame[chip].Read(Mixer);
                        m_frame[chip].Update(Mixer, mixer & ~(0x01 << chan));
                        m_frame[chip].UpdatePeriod(A_Period + 2 * chan, 0x0);
                        m_frame[chip].Update(A_Duty + chan, 0x8);
                    }
                }
            }
        }
        return m_frame;
    }
#endif
};
