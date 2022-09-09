#include "Output.h"

#include <cassert>
#include "stream/Stream.h"
#include "processing/AY8930EnvelopeFix.h"
#include "processing/ChannelsLayoutChange.h"
#include "processing/ChannelsOutputDisable.h"
#include "processing/ChipClockRateConvert.h"

#define DEBUG_OUT 0

#if DEBUG_OUT
#include <fstream>
std::ofstream debug_out;
#endif

Output::Output()
    : m_isOpened(false)
{
#if DEBUG_OUT
    debug_out.open(__FILE__".txt");
#endif
}

Output::~Output()
{
#if DEBUG_OUT
    debug_out.close();
#endif
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
        if (m_isOpened &= InitDstChip(stream.chip, m_chip))
        {
            // check if the output chip setup is correct
            assert(m_chip.clockKnown());
            assert(m_chip.outputKnown());
            if (m_chip.output() == Chip::Output::Stereo)
            {
                assert(m_chip.stereoKnown());
            }

            // restrict stereo modes available for exp mode
            if (stream.IsExpandedModeUsed() && m_chip.output() == Chip::Output::Stereo)
            {
                if (m_chip.stereo() != Chip::Stereo::ABC && m_chip.stereo() != Chip::Stereo::ACB)
                {
                    m_chip.stereo(Chip::Stereo::ABC);
                }
            }

            // init post-processing
            m_processingChain.push_back(std::make_unique<AY8930EnvelopeFix>(m_chip));
            m_processingChain.push_back(std::make_unique<ChannelsLayoutChange>(m_chip));
            m_processingChain.push_back(std::make_unique<ChannelsOutputDisable>(m_chip));
            m_processingChain.push_back(std::make_unique<ChipClockRateConvert>(stream.chip, m_chip));
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
        for (int chip = 0; chip < m_chip.count(); ++chip)
        {
            data.clear();
            if (m_chip.hasExpMode(chip) && pframe[chip].IsExpMode())
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
                {
                    data.emplace_back(Mode_Bank, pframe[chip].GetData(Mode_Bank));
                }
            }

            for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
            {
                if (pframe[chip].IsChanged(reg))
                {
                    data.emplace_back(reg & 0x0F, pframe[chip].GetData(reg));
                }
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
    return (GetDeviceName() + " -> " + m_chip.toString());
}

void Output::Reset()
{
    for (auto& processing : m_processingChain)
    {
        processing->Reset();
    }
    Processing::Reset();
}

const Frame& Output::operator()(const Frame& frame)
{
    const Frame* processed = &frame;
    for (auto& processing : m_processingChain)
    {
        processed = &(*processing)(*processed);
    }
    Update(*processed);

#if DEBUG_OUT
    Frame& oldFrame = const_cast<Frame&>(frame);
    Frame& newFrame = const_cast<Frame&>(m_frame);
    debug_out << oldFrame << newFrame << "\n";
#endif

    return m_frame;
}
