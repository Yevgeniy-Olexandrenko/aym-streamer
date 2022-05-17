#include "EncodeTXT.h"

bool EncodeTXT::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "txt"))
    {
        m_fileStream.open(stream.file);
        if (m_fileStream)
        {
            m_isTS = (stream.chip.count() == Chip::Count::TurboSound);
            m_loop = stream.loop.frameId();
            m_frameRate = stream.playback.frameRate();
            m_displayType = DisplayType::Grouped;

            PrintFrameHeader();
            return true;
        }
    }
    return false;
}

void EncodeTXT::Encode(FrameId id, const Frame& frame)
{
    PrintFrameRegisters(id, frame);
    m_prevFrame = frame;
}

void EncodeTXT::Close(const Stream& stream)
{
    m_fileStream.close();
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeTXT::PrintFrameHeader()
{
    m_fileStream << "frame ";
    PrintChipHeader();
#if 0
    auto save = m_displayType;
    m_displayType = DisplayType::Grouped;
    m_fileStream << ' ';
    PrintChipHeader();
    m_displayType = save;
#else
    if (m_isTS)
    {
        m_fileStream << ' ';
        PrintChipHeader();
    }
#endif
    m_fileStream << std::endl;
}

void EncodeTXT::PrintFrameRegisters(FrameId id, const Frame& frame)
{
    if ((id % m_frameRate) == 0) PrintDelimiter();
    m_fileStream << std::setfill('0') << std::setw(5) << id;
    m_fileStream << (id == m_loop ? '*' : ' ');
    PrintChipRegisters(frame, 0);

#if 0
    auto save = m_displayType;
    m_displayType = DisplayType::Grouped;
    m_fileStream << ' ';
    PrintChipRegisters(frame, 0);
    m_displayType = save;
#else
    if (m_isTS)
    {
        m_fileStream << ' ';
        PrintChipRegisters(frame, 1);
    }
#endif
    m_fileStream << std::endl;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeTXT::PrintChipHeader()
{
    switch (m_displayType)
    {
    case DisplayType::Dump:
        m_fileStream << "|r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 rA rB rC rD|";
        break;

    case DisplayType::Grouped:
        m_fileStream << "|r7|r1r0 r8|r3r2 r9|r5r4 rA|rCrB rD|r6|";
        break;

    case DisplayType::Readable:
        m_fileStream << "|NNN|EEEEE S|TNE TTTT VV|TNE TTTT VV|TNE TTTT VV|";
        break;
    }
}

void EncodeTXT::PrintChipRegisters(const Frame& frame, int chip)
{
    switch (m_displayType)
    {
    case DisplayType::Dump:
        m_fileStream << '|';
        for (uint8_t reg = 0; reg < 14; ++reg)
        {
            PrintChipRegister(frame, chip, reg);
            if (reg < 13) m_fileStream << ' ';
        }
        m_fileStream << '|';
        break;

    case DisplayType::Grouped:
        m_fileStream << '|';
        PrintChipRegister(frame, chip, Mixer);
        m_fileStream << '|';
        PrintChipRegister(frame, chip, A_Coarse);
        PrintChipRegister(frame, chip, A_Fine);
        m_fileStream << ' ';
        PrintChipRegister(frame, chip, A_Volume);
        m_fileStream << '|';
        PrintChipRegister(frame, chip, B_Coarse);
        PrintChipRegister(frame, chip, B_Fine);
        m_fileStream << ' ';
        PrintChipRegister(frame, chip, B_Volume);
        m_fileStream << '|';
        PrintChipRegister(frame, chip, C_Coarse);
        PrintChipRegister(frame, chip, C_Fine);
        m_fileStream << ' ';
        PrintChipRegister(frame, chip, C_Volume);
        m_fileStream << '|';
        PrintChipRegister(frame, chip, E_Coarse);
        PrintChipRegister(frame, chip, E_Fine);
        m_fileStream << ' ';
        PrintChipRegister(frame, chip, E_Shape);
        m_fileStream << '|';
        PrintChipRegister(frame, chip, N_Period);
        m_fileStream << "|";
        break;

    case DisplayType::Readable:
        m_fileStream << '|';
        PrintChipRegisterDelta(frame, chip, N_Period, 5);
        m_fileStream << '|';
        PrintChipRegisterDelta(frame, chip, E_Fine, 16);
        m_fileStream << ' ';
        if (frame.IsChanged(chip, E_Shape))
            PrintNibble(frame.Read(chip, E_Shape));
        else
            m_fileStream << '.';

        m_fileStream << '|';
        PrintChannelMixer(frame, chip, 0);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, A_Fine, 12);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, A_Volume, 4);

        m_fileStream << '|';
        PrintChannelMixer(frame, chip, 1);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, B_Fine, 12);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, B_Volume, 4);

        m_fileStream << '|';
        PrintChannelMixer(frame, chip, 2);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, C_Fine, 12);
        m_fileStream << ' ';
        PrintChipRegisterDelta(frame, chip, C_Volume, 4);

        m_fileStream << '|';
        break;
    }
}

void EncodeTXT::PrintDelimiter()
{
    const size_t SIZE[]{ 43, 39, 49 };
    size_t size = SIZE[size_t(m_displayType)];
    m_fileStream << std::string(6 + (m_isTS ? (size * 2 + 1) : size), '-') << std::endl;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeTXT::PrintNibble(uint8_t nibble)
{
    nibble &= 0x0F;
    m_fileStream << char((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble);
}

void EncodeTXT::PrintChipRegister(const Frame& frame, int chip, int reg)
{
    if (frame.IsChanged(chip, reg))
    {
        uint8_t data = frame.Read(chip, reg);
        PrintNibble(data >> 4);
        PrintNibble(data);
    }
    else
    {
        m_fileStream << "..";
    }
}

void EncodeTXT::PrintChipRegisterDelta(const Frame& frame, int chip, int reg, int bits)
{
    if (frame.IsChanged(chip, reg))
    {
        int mask = (1 << bits) - 1;

        int old_v = m_prevFrame.Read(chip, reg);
        if (bits > 8) old_v |= m_prevFrame.Read(chip, reg + 1) << 8;
        old_v &= mask;
       
        int new_v = frame.Read(chip, reg);
        if (bits > 8) new_v |= frame.Read(chip, reg + 1) << 8;
        new_v &= mask;

        int delta = new_v - old_v;
        if (delta < 0)
        {
            m_fileStream << '-';
            delta = -delta;
        }
        else
        {
            m_fileStream << '+';
        }

        if (bits > 12) PrintNibble(delta >> 12);
        if (bits >  8) PrintNibble(delta >>  8);
        if (bits >  4) PrintNibble(delta >>  4);
        PrintNibble(delta);
    }
    else
    {
        m_fileStream << std::string((bits + 3) / 4 + 1, '.');
    }
}

void EncodeTXT::PrintChannelMixer(const Frame& frame, int chip, int chan)
{
    uint8_t old_mixer = m_prevFrame.Read(chip, Mixer) >> chan;
    uint8_t old_vol_e = m_prevFrame.Read(chip, A_Volume + chan);
    uint8_t new_mixer = frame.Read(chip, Mixer) >> chan;
    uint8_t new_vol_e = frame.Read(chip, A_Volume + chan);

    bool changeT = ((new_mixer ^ old_mixer) & 0x01);
    bool changeN = ((new_mixer ^ old_mixer) & 0x08);
    bool changeE = ((new_vol_e ^ old_vol_e) & 0x10);

    m_fileStream << (changeT ? (new_mixer & 0x01 ? 't' : 'T') : '.');
    m_fileStream << (changeN ? (new_mixer & 0x08 ? 'n' : 'N') : '.');
    m_fileStream << (changeE ? (new_vol_e & 0x10 ? 'E' : 'e') : '.');
}