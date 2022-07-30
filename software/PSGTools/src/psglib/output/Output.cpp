#include "Output.h"
#include "stream/Frame.h"

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

bool Output::Init(const Stream& stream)
{
    // m_chip.stereo(Chip::Stereo::BCA);

    // reset post-processing
    static_cast<Processing&>(*this).Reset();
    return m_isOpened;
}

bool Output::Write(const Frame& frame)
{
    // post-processing before output
    const Frame& processed = static_cast<Processing&>(*this)(m_chip, frame);

    // output to chip(s)
    for (int chip = 0; chip < m_chip.countValue(); ++chip)
    {
        WriteToChip(chip, processed);
    }
    return m_isOpened;
}

void Output::WriteToChip(int chip, const Frame& frame)
{
    if (m_isOpened)
    {
        std::vector<uint8_t> data;

        if (m_chip.model() == Chip::Model::AY8930 && frame.IsExpMode(chip))
        {
            bool switchBanks = false;
            for (Register reg = BankB_Fst; reg < BankB_Lst; ++reg)
            {
                if (frame.IsChanged(chip, reg))
                {
                    if (!switchBanks)
                    {
                        data.push_back(Mode_Bank);
                        data.push_back(frame.Read(chip, Mode_Bank) | 0x10);
                        switchBanks = true;
                    }
                    data.push_back(reg & 0x0F);
                    data.push_back(frame.Read(chip, reg));
                }
            }

            if (switchBanks)
            {
                data.push_back(Mode_Bank);
                data.push_back(frame.Read(chip, Mode_Bank));
            }
        }

        for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
        {
            if (frame.IsChanged(chip, reg))
            {
                data.push_back(reg & 0x0F);
                data.push_back(frame.Read(chip, reg));
            }
        }

        data.push_back(0xFF);
        WriteToChip(chip, data);
    }
}

void Output::Reset()
{
    m_fixAY8930Envelope.Reset();
    m_convertExpToComp.Reset();
    m_convertToNewClock.Reset();
    m_swapChannelsOrder.Reset();
    m_disableChannels.Reset();
    Processing::Reset();
}

const Frame& Output::operator()(const Chip& chip, const Frame& frame)
{
    const Frame* processed = &frame;
    processed = &m_fixAY8930Envelope (chip, *processed);
    processed = &m_convertExpToComp  (chip, *processed);
    processed = &m_convertToNewClock(chip,  *processed);
    processed = &m_swapChannelsOrder (chip, *processed);
    processed = &m_disableChannels   (chip, *processed);
    Update(*processed);

#if DEBUG_OUT
    Frame& oldFrame = const_cast<Frame&>(frame);
    Frame& newFrame = const_cast<Frame&>(m_frame);
    debug_out << oldFrame << newFrame << "\n";
#endif

    return m_frame;
}

const Frame& Output::GetFrame() const
{
    return m_frame;
}

std::string Output::toString() const
{
    return (GetOutputDeviceName() + " -> " + m_chip.toString());
}
