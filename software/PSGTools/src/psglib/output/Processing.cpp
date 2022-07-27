#include "Processing.h"

void Processing::Reset()
{
    m_frame.ResetData();
    m_frame.ResetChanges();
}

void Processing::Update(const Frame& frame)
{
    m_frame.ResetChanges();
    m_frame += frame;
}

const Frame& FixAY8930Envelope::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_FixAY8930Envelope
    if (chip.model() == Chip::Model::AY8930)
    {
        Update(frame);
        for (int count = chip.countValue(), chip = 0; chip < count; ++chip)
        {
            for (int chan = 0; chan < 3; ++chan)
            {
                uint8_t mixer = m_frame.Read(chip, Mixer) >> chan;
                uint8_t vol_e = m_frame.Read(chip, A_Volume + chan);

                // envelope flag position correction
                if (m_frame.IsExpMode(chip)) vol_e >>= 1;

                bool enableT = !(mixer & 0x01);
                bool enableN = !(mixer & 0x08);
                bool enableE =  (vol_e & 0x10);

                if (enableE && !(enableT || enableN))
                {
                    // enable tone and set period to zero
                    mixer = m_frame.Read(chip, Mixer);
                    m_frame.Update(chip, Mixer, mixer & ~(0x01 << chan));
                    m_frame.UpdatePeriod(chip, A_Period + 2 * chan, 0x0);
                }
            }
        }
        return m_frame;
    }
#endif
    return frame;
}

const Frame& ConvertExpToComp::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_ConvertExpToComp
    if (chip.model() != Chip::Model::AY8930)
    {
        // TODO

        for (int count = chip.countValue(), chip = 0; chip < count; ++chip)
        {
            if (frame->IsExpMode(chip))
            {
                // TODO
            }
        }
    }
#endif
    return frame;
}

const Frame& SwapChannels::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_SwapChannels

    // TODO

#endif
    return frame;
}

const Frame& DisableChannels::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_DisableChannels

    // TODO

#endif
    return frame;
}
