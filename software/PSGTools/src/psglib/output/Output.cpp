#include "Output.h"
#include "stream/Frame.h"

bool Output::Write(const Frame& frame)
{
    WriteToChip(0, frame);
    if (m_chip.count() == Chip::Count::TwoChips)
    {
        WriteToChip(1, frame);
    }
    return m_isOpened;
}

void Output::WriteToChip(int chip, const Frame& frame)
{
    if (m_isOpened)
    {
        std::vector<uint8_t> data;

        if (frame.IsExpMode(chip))
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
                    data.push_back(reg);
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
                data.push_back(reg);
                data.push_back(frame.Read(chip, reg));
            }
        }

        data.push_back(0xFF);
        WriteToChip(chip, data);
    }
}
