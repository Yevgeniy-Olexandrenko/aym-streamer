#include "Output.h"
#include "stream/Frame.h"

Output::Output()
    : m_isOpened(false)
{
}

std::string Output::toString() const
{
    return (GetOutputDeviceName() + " -> " + m_chip.toString());
}

bool Output::Init(const Stream& stream)
{
    m_fixAY8930Envelope.Reset();
    m_convertExpToComp.Reset();
    m_swapChannels.Reset();
    m_disableChannels.Reset();
    return m_isOpened;
}

bool Output::Write(const Frame& frame)
{
    const Frame* processed = &frame;

    // post processing before output
    processed = &m_fixAY8930Envelope (m_chip, *processed);
    processed = &m_convertExpToComp  (m_chip, *processed);
    processed = &m_swapChannels      (m_chip, *processed);
    processed = &m_disableChannels   (m_chip, *processed);

    // output to chip(s)
    for (int chip = 0; chip < m_chip.countValue(); ++chip)
    {
        WriteToChip(chip, *processed);
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
