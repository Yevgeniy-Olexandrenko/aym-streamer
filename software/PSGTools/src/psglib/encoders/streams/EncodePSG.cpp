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

            m_isTS = (stream.chip.count() == Chip::Count::TurboSound);
            m_skip = 0;

            return true;
        }
    }
    return false;
}

void EncodePSG::Encode(FrameId id, const Frame& frame)
{
    m_skip++;
    if (frame.HasChanges())
    {
        WriteFrameBeginOrSkip();
        for (uint8_t chip = 0; chip < (m_isTS ? 2 : 1); ++chip)
        {
            for (uint8_t reg = 0; reg < 14; ++reg)
            {
                if (frame.IsChanged(chip, reg))
                {
                    uint8_t data0 = (chip << 7 | reg);
                    uint8_t data1 = frame.Read(reg);
                    m_output << data0 << data1;
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
        if (int blocks = m_skip / 4) m_output << uint8_t(0xFE) << uint8_t(blocks);
        if (int frames = m_skip % 4) m_output << std::string(frames, uint8_t(0xFF));
        m_skip = 0;
    }
}
