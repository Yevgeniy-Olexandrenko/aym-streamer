#include "EncodePSG.h"

bool EncodePSG::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "psg"))
    {
        m_output.open(stream.file, std::fstream::binary);
        if (m_output)
        {
            m_output << "PSG";
            m_output << uint8_t(0x1A);
            m_output << uint8_t(0x0A);
            m_output << uint8_t(stream.play.frameRate());
            m_output << std::string(10, 0x00);

            m_skip = 0;
            return true;
        }
    }
    return false;
}

void EncodePSG::Encode(const Frame& frame)
{
    const auto OutputRegister = [&](int chip, Register reg)
    {
        if (frame[chip].IsChanged(reg))
        {
            uint8_t v1 = (chip << 7 | reg);
            uint8_t v2 = frame[chip].Read(reg);
            m_output << v1 << v2;
        }
    };

    m_skip++;
    if (frame.HasChanges())
    {
        WriteFrameBeginOrSkip();

        // write full dump of frame (two chips
        // each in expanded mode if available)
        for (int chip = 0; chip < 2; ++chip)
        {
            if (frame[chip].IsExpMode())
            {
                OutputRegister(chip, Mode_Bank);
                for (Register reg = BankA_Fst; reg <= BankB_Lst; ++reg)
                {
                    if ((reg & 0x0F) == Mode_Bank) continue;
                    OutputRegister(chip, reg);
                }
            }
            else
            {
                for (Register reg = BankA_Fst; reg <= BankA_Lst; ++reg)
                {
                    OutputRegister(chip, reg);
                }
            }
        }
    }
}

void EncodePSG::Close(const Stream& stream)
{
    WriteFrameBeginOrSkip();
    m_output.close();
}

void EncodePSG::WriteFrameBeginOrSkip()
{
    if (m_skip)
    {
        // TODO: what if blocks >= 256 (1024 frames) ???
        if (int blocks = m_skip / 4) m_output << uint8_t(0xFE) << uint8_t(blocks);
        if (int frames = m_skip % 4) m_output << std::string(frames, uint8_t(0xFF));
        m_skip = 0;
    }
}
