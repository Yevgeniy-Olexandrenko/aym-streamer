#include "Processing.h"
#include <cassert>

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
    assert(chip.modelKnown());

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
            if (frame.IsExpMode(chip))
            {
                // TODO
            }
        }
    }
#endif
    return frame;
}

const Frame& ConvertToNewClock::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_ConvertToNewClock

    // TODO

#endif
    return frame;
}

const Frame& SwapChannelsOrder::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_SwapChannelsOrder
    assert(chip.outputKnown());
    assert(chip.stereoKnown());

    if (chip.output() == Chip::Output::Stereo && chip.stereo() != Chip::Stereo::ABC)
    {
        auto SwapChannels = [&](int chip, int l, int r)
        {
            Frame::Channel lchan = m_frame.ReadChannel(chip, l);
            Frame::Channel rchan = m_frame.ReadChannel(chip, r);
            m_frame.UpdateChannel(chip, l, rchan);
            m_frame.UpdateChannel(chip, r, lchan);
        };

        Update(frame);
        Chip::Stereo stereo{ chip.stereo() };
        for (int count = chip.countValue(), chip = 0; chip < count; ++chip)
        {
            switch (stereo)
            {
            case Chip::Stereo::ACB:
                SwapChannels(chip, 1, 2);
                break;
            case Chip::Stereo::BAC:
                SwapChannels(chip, 0, 1);
                break;
            case Chip::Stereo::BCA:
                SwapChannels(chip, 0, 1);
                SwapChannels(chip, 1, 2);
                break;
            case Chip::Stereo::CAB:
                SwapChannels(chip, 1, 2);
                SwapChannels(chip, 0, 1);
                break;
            case Chip::Stereo::CBA:
                SwapChannels(chip, 0, 2);
                break;
            }
        }
        return m_frame;
    }
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
