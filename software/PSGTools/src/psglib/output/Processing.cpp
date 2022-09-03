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
    if (chip.first.model() == Chip::Model::AY8930 || chip.second.model() == Chip::Model::AY8930)
    {
        Update(frame);
        for (int count = chip.count(), chip = 0; chip < count; ++chip)
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
                    m_frame.Update(chip, A_Duty + chan, 0x8);
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
    if (chip.first.model() != Chip::Model::AY8930 || (chip.second.modelKnown() && chip.second.model() != Chip::Model::AY8930))
    {
        m_frame.ResetChanges();
        for (int count = chip.count(), chip = 0; chip < count; ++chip)
        {
            if (frame.IsExpMode(chip))
            {
                m_frame.UpdatePeriod(chip, A_Period, frame.ReadPeriod(chip, A_Period));
                m_frame.UpdatePeriod(chip, B_Period, frame.ReadPeriod(chip, B_Period));
                m_frame.UpdatePeriod(chip, C_Period, frame.ReadPeriod(chip, C_Period));
                m_frame.UpdatePeriod(chip, N_Period, frame.ReadPeriod(chip, N_Period));
                m_frame.Update(chip, Mixer, frame.Read(chip, Mixer));

                // convert volume + envelope flag registers
                uint8_t a_volume = frame.Read(chip, A_Volume);
                uint8_t b_volume = frame.Read(chip, B_Volume);
                uint8_t c_volume = frame.Read(chip, C_Volume);
                m_frame.Update(chip, A_Volume, a_volume >> 1);
                m_frame.Update(chip, B_Volume, b_volume >> 1);
                m_frame.Update(chip, C_Volume, c_volume >> 1);

                // choose envelope depending on channels priority
#if 1
                auto e_period = frame.ReadPeriod(chip, EC_Period);
                auto e_shape = frame.Read(chip, EC_Shape);
                m_frame.UpdatePeriod(chip, E_Period, e_period);
                m_frame.Update(chip, E_Shape, e_shape);
                if (e_shape != k_unchangedShape)
                    e_shape &= 0x0F;
#else
                auto e_period = frame.ReadPeriod(chip, EA_Period);
                auto e_shape = frame.Read(chip, EA_Shape);
                if ((a_volume & 0x20) == 0)
                {
                    if ((b_volume & 0x20) != 0)
                    {
                        e_period = frame.ReadPeriod(chip, EB_Period);
                        e_shape = frame.Read(chip, EB_Shape);
                    }
                    else if ((c_volume & 0x20) != 0)
                    {
                        e_period = frame.ReadPeriod(chip, EC_Period);
                        e_shape = frame.Read(chip, EC_Shape);
                        if (e_shape != k_unchangedShape)
                            e_shape &= 0x0F;
                    }
                }
                if (e_shape != k_unchangedShape) 
                    e_shape &= 0x0F;
                m_frame.UpdatePeriod(chip, E_Period, e_period);
                m_frame.Update(chip, E_Shape, e_shape);
#endif
            }
        }
        return m_frame;
    }
#endif
    return frame;
}

const Frame& ConvertToNewClock::operator()(const Chip& chip, const Frame& frame)
{
#ifdef Enable_ConvertToNewClock
    {
        Update(frame);

        uint16_t a_period = m_frame.ReadPeriod(A_Period);
        uint16_t b_period = m_frame.ReadPeriod(B_Period);
        uint16_t c_period = m_frame.ReadPeriod(C_Period);

        m_frame.UpdatePeriod(A_Period, a_period << 1);
        m_frame.UpdatePeriod(B_Period, b_period << 1);
        m_frame.UpdatePeriod(C_Period, c_period << 1);

        return m_frame;
    }
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
        for (int count = chip.count(), chip = 0; chip < count; ++chip)
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
