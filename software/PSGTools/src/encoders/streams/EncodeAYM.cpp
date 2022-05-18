#include "EncodeAYM.h"
#include <cassert>

bool EncodeAYM::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "aym"))
    {
        m_fileStream.open(stream.file);
        if (m_fileStream)
        {
            m_fileStream << "AYYM";
            m_bitStream.Open(m_fileStream);

            // TODO

            return true;
        }
    }
    return false;
}

void EncodeAYM::Encode(FrameId id, const Frame& frame)
{
    if (frame.HasChanges())
    {
        // write step delta
        if (m_newStep != m_oldStep)
        {
            m_bitStream.Write(0x00, 8);
            WriteDelta(Delta(m_oldStep, m_newStep));

            m_oldStep = m_newStep;
            m_newStep = 1;
        }

        // write mask of changes
        uint8_t mainMask = 0;
        uint8_t extraMask = 0;

        if (frame.IsChanged(Mixer)) mainMask |= 0b10000000;
        if (frame.IsChangedPeriod(A_Period)) mainMask |= 0b01000000;
        if (frame.IsChanged(A_Volume)) mainMask |= 0b00100000;
        if (frame.IsChangedPeriod(B_Period)) mainMask |= 0b00010000;
        if (frame.IsChanged(B_Volume)) mainMask |= 0b00001000;
        if (frame.IsChangedPeriod(C_Period)) mainMask |= 0b00000100;
        if (frame.IsChanged(C_Volume)) mainMask |= 0b00000010;
        if (frame.IsChangedPeriod(N_Period)) extraMask |= 0b1000;
        if (frame.IsChangedPeriod(E_Period)) extraMask |= 0b0100;
        if (frame.IsChanged(E_Shape)) extraMask |= 0b0010;
        if (extraMask) mainMask |= 0b00000001;

        m_bitStream.Write(mainMask, 8);
        if (mainMask & 0b00000001) m_bitStream.Write(extraMask, 4);

        // write changes
        if (mainMask & 0b10000000) WriteDelta(Delta(m_frame.Read(Mixer), frame.Read(Mixer)));
        if (mainMask & 0b01000000) WriteDelta(Delta(m_frame.ReadPeriod(A_Period), frame.ReadPeriod(A_Period)));
        if (mainMask & 0b00100000) WriteDelta(Delta(m_frame.Read(A_Volume), frame.Read(A_Volume)));
        if (mainMask & 0b00010000) WriteDelta(Delta(m_frame.ReadPeriod(B_Period), frame.ReadPeriod(B_Period)));
        if (mainMask & 0b00001000) WriteDelta(Delta(m_frame.Read(B_Volume), frame.Read(B_Volume)));
        if (mainMask & 0b00000100) WriteDelta(Delta(m_frame.ReadPeriod(C_Period), frame.ReadPeriod(C_Period)));
        if (mainMask & 0b00000010) WriteDelta(Delta(m_frame.Read(C_Volume), frame.Read(C_Volume)));
        if (extraMask & 0b1000) WriteDelta(Delta(m_frame.ReadPeriod(N_Period), frame.ReadPeriod(N_Period)));
        if (extraMask & 0b0100) WriteDelta(Delta(m_frame.ReadPeriod(E_Period), frame.ReadPeriod(E_Period)));
        if (extraMask & 0b0010) WriteDelta(Delta(m_frame.Read(E_Shape), frame.Read(E_Shape)));
    }
    else
    {
        m_newStep++;
    }

    m_frame = frame;
}

void EncodeAYM::Close(const Stream& stream)
{
    // write step delta
    if (m_newStep != m_oldStep)
    {
        m_bitStream.Write(0x00, 8);
        WriteDelta(Delta(m_oldStep, m_newStep));
    }

    m_bitStream.Close();
    m_fileStream.close();
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeAYM::WriteDelta(const Delta& delta)
{
#if 1
    int8_t index = m_deltaList.GetIndex(delta);
    if (index < 0)
    {
        switch (delta.GetValue())
        {
        default:
            m_bitStream.Write(delta.GetBits() / 4 - 1, 3);
            m_bitStream.Write(delta.GetValue(), delta.GetBits());
            break;

        case 0:
            m_bitStream.Write(0b100, 3); break;
        case +1:
            m_bitStream.Write(0b101, 3); break;
        case -1:
            m_bitStream.Write(0b110, 3); break;
        }
    }
    else
    {
        m_bitStream.Write(0b11100000 | index, 8);
    }
#else
    int8_t index = m_deltaList.GetIndex(delta);
    if (index < 0)
    {
        switch (delta.GetValue())
        {
        default:
            m_bitStream.Write(delta.GetBits() / 4 - 1, 4);
            m_bitStream.Write(delta.GetValue(), delta.GetBits());
            break;

        case 0: 
            m_bitStream.Write(0b0100, 4); break;
        case +1: 
            m_bitStream.Write(0b0101, 4); break;
        case -1: 
            m_bitStream.Write(0b0110, 4); break;
        }
    }
    else
    {
        m_bitStream.Write(0x80 | index, 8);
    }
#endif
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Delta::Delta(uint16_t from, uint16_t to)
    : m_value(to - from)
    , m_bits(16)
{
    if (m_value <= 7i8 && m_value >= (-7i8 - 1)) m_bits = 4;
    else if (m_value <= 127i8 && m_value >= (-127i8 - 1)) m_bits = 8;
    else if (m_value <= 2047i16 && m_value >= (-2047i16 - 1)) m_bits = 12;
}

int16_t Delta::GetValue() const
{
    return m_value;
}

uint8_t Delta::GetBits() const
{
    return m_bits;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

DeltaList::DeltaList()
    : m_list{}
    , m_writeIndex(0)
{
}

int8_t DeltaList::GetIndex(const Delta& delta)
{
    int8_t index = -1;
#if 1
    if (delta.GetBits() > 4)
    {
        auto size = sizeof(m_list) / sizeof(m_list[0]);
        for (int i = 0; i < size; ++i)
        {
            if (m_list[i] == delta.GetValue())
            {
                return i;
            }
        }

        index = m_writeIndex;
        m_list[m_writeIndex] = delta.GetValue();
        if (++m_writeIndex == size) m_writeIndex = 0;
    }
#endif
    return index;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void BitStream::Open(std::ostream& stream)
{
    m_stream = &stream;
    m_buffer = m_count = 0;
}

void BitStream::Write(uint32_t data, uint8_t bits)
{
    if (m_stream)
    {
        uint8_t maxSize = uint8_t(sizeof(data) << 3);
        bits = std::min(bits, maxSize);
        data <<= (maxSize - bits);

        for (int i = 0; i < bits; ++i)
        {
            m_buffer <<= 1;
            m_buffer |= (data >> (maxSize - 1) & 1);
            data <<= 1;
            m_count++;

            if (m_count == 8)
            {
                (*m_stream) << m_buffer;
                m_buffer = m_count = 0;
            }
        }
    }
}

void BitStream::Close()
{
    if (m_stream && m_count)
    {
        m_buffer <<= (8 - m_count);
        (*m_stream) << m_buffer;
        m_buffer = m_count = 0;
    }
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///
