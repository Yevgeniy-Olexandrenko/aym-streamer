#include "Output.h"

#include <cassert>
#include "stream/Stream.h"
#include "processing/AY8930EnvelopeFix.h"
#include "processing/ChannelsLayoutChange.h"
#include "processing/ChannelsOutputDisable.h"
#include "processing/ChipClockRateConvert.h"

Output::Output()
    : m_isOpened(false)
{
}

Output::~Output()
{
}

bool Output::Open()
{
    m_isOpened = OpenDevice();
    return m_isOpened;
}

bool Output::Init(const Stream& stream)
{
    if (m_isOpened)
    {
        m_schip = stream.schip;
        m_dchip = stream.dchip;

        if (m_isOpened &= ConfigureChip(m_schip, m_dchip))
        {
            // check if the output chip setup is correct
            assert(m_dchip.clockKnown());
            assert(m_dchip.outputKnown());
            if (m_dchip.output() == Chip::Output::Stereo)
            {
                assert(m_dchip.stereoKnown());
            }

            // restrict stereo modes available for exp mode
            if (stream.IsExpandedModeUsed() && m_dchip.output() == Chip::Output::Stereo)
            {
                if (m_dchip.stereo() != Chip::Stereo::ABC && m_dchip.stereo() != Chip::Stereo::ACB)
                {
                    m_dchip.stereo(Chip::Stereo::ABC);
                }
            }

            // init post-processing
            m_procChain.clear();
            m_procChain.emplace(new AY8930EnvelopeFix(m_dchip));
            m_procChain.emplace(new ChannelsLayoutChange(m_dchip));
            m_procChain.emplace(new ChannelsOutputDisable(m_dchip));
            m_procChain.emplace(new ChipClockRateConvert(m_schip, m_dchip));
            Reset();
        }
    }
    return m_isOpened;
}

bool Output::Write(const Frame& frame)
{
    if (m_isOpened)
    {
        // processing before output
        const Frame& pframe = static_cast<Processing&>(*this)(frame);

        // output to chip(s)
        Data data(32);
        for (int chip = 0; chip < m_dchip.count(); ++chip)
        {
            data.clear();
            if (m_dchip.hasExpMode(chip) && pframe[chip].IsExpMode())
            {
                bool switchBanks = false;
                for (Register reg = BankB_Fst; reg < BankB_Lst; ++reg)
                {
                    if (pframe[chip].IsChanged(reg))
                    {
                        if (!switchBanks)
                        {
                            data.emplace_back(Mode_Bank, pframe[chip].GetData(Mode_Bank) | 0x10);
                            switchBanks = true;
                        }
                        data.emplace_back(reg & 0x0F, pframe[chip].GetData(reg));
                    }
                }
                if (switchBanks)
                    data.emplace_back(Mode_Bank, pframe[chip].GetData(Mode_Bank));
            }

            for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
            {
                if (pframe[chip].IsChanged(reg))
                    data.emplace_back(reg & 0x0F, pframe[chip].GetData(reg));
            }

            if (!(m_isOpened &= WriteToChip(chip, data))) break;
        }
    }
    return m_isOpened;
}

void Output::Close()
{
    CloseDevice();
    m_isOpened = false;
}

const Frame& Output::GetFrame() const
{
    return m_frame;
}

std::string Output::toString() const
{
    return (GetDeviceName() + " -> " + m_dchip.toString());
}

void Output::Reset()
{
    for (const auto& procStep : m_procChain)
    {
        procStep->Reset();
    }
    Processing::Reset();
}

const Frame& Output::operator()(const Frame& frame)
{
    const Frame* pframe = &frame;
    for (const auto& procStep : m_procChain)
    {
        pframe = &(*procStep)(*pframe);
    }
    Processing::Update(*pframe);
    return m_frame;
}
