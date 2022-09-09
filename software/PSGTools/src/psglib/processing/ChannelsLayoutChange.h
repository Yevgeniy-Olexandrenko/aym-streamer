#pragma once

#include <cassert>
#include "Processing.h"

class ChannelsLayoutChange : public Processing
{
    Chip m_chip;

public:
    ChannelsLayoutChange(const Chip& dstChip)
        : m_chip(dstChip)
    {}

#ifdef Enable_ChannelsLayoutChange
	const Frame& operator()(const Frame& frame) override
    {
        assert(m_chip.outputKnown());
        assert(m_chip.stereoKnown());

        if (m_chip.output() == Chip::Output::Stereo && m_chip.stereo() != Chip::Stereo::ABC)
        {
            auto SwapChannels = [&](int chip, int L, int R)
            {
                Frame::Channel chL = m_frame[chip].ReadChannel(L);
                Frame::Channel chR = m_frame[chip].ReadChannel(R);

                m_frame[chip].UpdateChannel(L, chR);
                m_frame[chip].UpdateChannel(R, chL);
            };

            Update(frame); // TODO: there may be redundant changes
            for (int chip = 0; chip < m_chip.count(); ++chip)
            {
                switch (m_chip.stereo())
                {
                case Chip::Stereo::ACB:
                    SwapChannels(chip, Frame::Channel::B, Frame::Channel::C);
                    break;

                case Chip::Stereo::BAC:
                    SwapChannels(chip, Frame::Channel::A, Frame::Channel::B);
                    break;

                case Chip::Stereo::BCA:
                    SwapChannels(chip, Frame::Channel::A, Frame::Channel::B);
                    SwapChannels(chip, Frame::Channel::B, Frame::Channel::C);
                    break;

                case Chip::Stereo::CAB:
                    SwapChannels(chip, Frame::Channel::B, Frame::Channel::C);
                    SwapChannels(chip, Frame::Channel::A, Frame::Channel::B);
                    break;

                case Chip::Stereo::CBA:
                    SwapChannels(chip, Frame::Channel::A, Frame::Channel::C);
                    break;
                }
            }
            return m_frame;
        }
        return frame;
    }
#endif
};
