#include "EncodeAYM.h"
#include <cassert>

EncodeAYM::Delta::Delta(uint16_t from, uint16_t to)
    : m_value(to - from)
    , m_size(16)
{
    if (m_value <= 7i8 && m_value >= (-7i8 - 1)) m_size = 4;
    else if (m_value <= 127i8 && m_value >= (-127i8 - 1)) m_size = 8;
    else if (m_value <= 2047i16 && m_value >= (-2047i16 - 1)) m_size = 12;
}

int16_t EncodeAYM::Delta::value() const
{
    return m_value;
}

uint8_t EncodeAYM::Delta::size() const
{
    return m_size;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

EncodeAYM::DeltaList::DeltaList()
    : m_list{}
    , m_index(0)
{
}

int8_t EncodeAYM::DeltaList::GetIndex(const Delta& delta)
{
    int8_t index = -1;
    if (delta.size() > 4)
    {
        int size = sizeof(m_list) / sizeof(m_list[0]);
        for (int i = 0; i < size; ++i)
        {
            if (m_list[i] == delta.value())
            {
                return i;
            }
        }

        index = m_index;
        m_list[m_index] = delta.value();
        if (++m_index == size) m_index = 0;
    }
    return index;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeAYM::BitStream::Open(std::ostream& stream)
{
    m_stream = &stream;
    m_buffer = m_count = 0;
}

void EncodeAYM::BitStream::Write(uint16_t data, uint8_t size)
{
    if (m_stream)
    {
        auto maxSize = uint8_t(sizeof(data) * 8);
        size = std::min(size, maxSize);
        data <<= (maxSize - size);

        for (int i = 0; i < size; ++i)
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

void EncodeAYM::BitStream::Close()
{
    if (m_stream && m_count)
    {
        m_buffer <<= (8 - m_count);
        (*m_stream) << m_buffer;
        m_buffer = m_count = 0;
    }
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

bool EncodeAYM::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "aym"))
    {
        m_output.open(stream.file, std::fstream::binary);
        if (m_output)
        {
            m_isTS = (stream.chip.count() == Chip::Count::TurboSound);

            m_output << "AYYM";
            m_bitStream.Open(m_output);

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
        WriteStepDelta();
        if (m_isTS)
        {
            WriteChipDelta(frame, 0, false);
            WriteChipDelta(frame, 1, true);
        }
        else
            WriteChipDelta(frame, 0, true);
    }
    else
    {
        m_newStep++;
    }
    m_frame = frame;
}

void EncodeAYM::Close(const Stream& stream)
{
    WriteStepDelta();
    m_bitStream.Close();
    m_output.close();
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void EncodeAYM::WriteDelta(const Delta& delta)
{
    auto index = m_deltaList.GetIndex(delta);
    if (index < 0)
    {
        switch (delta.value())
        {
        default:
            m_bitStream.Write(delta.size() / 4 - 1, 3);
            m_bitStream.Write(delta.value(), delta.size());
            break;

        case  0: m_bitStream.Write(0b100, 3); break;
        case +1: m_bitStream.Write(0b101, 3); break;
        case -1: m_bitStream.Write(0b110, 3); break;
        }
    }
    else
    {
        m_bitStream.Write(0b11100000 | index, 8);
    }
}

void EncodeAYM::WriteChipDelta(const Frame& frame, uint8_t chip, bool isLast)
{
    uint8_t loMask = 0;
    uint8_t hiMask = 0;

    if (frame.IsChanged(Mixer)) loMask |= 0b10000000;
    if (frame.IsChangedPeriod(A_Period)) loMask |= 0b01000000;
    if (frame.IsChanged(A_Volume)) loMask |= 0b00100000;
    if (frame.IsChangedPeriod(B_Period)) loMask |= 0b00010000;
    if (frame.IsChanged(B_Volume)) loMask |= 0b00001000;
    if (frame.IsChangedPeriod(C_Period)) loMask |= 0b00000100;
    if (frame.IsChanged(C_Volume)) loMask |= 0b00000010;
    if (frame.IsChangedPeriod(N_Period)) hiMask |= 0b1000;
    if (frame.IsChangedPeriod(E_Period)) hiMask |= 0b0100;
    if (frame.IsChanged(E_Shape)) hiMask |= 0b0010;
    if (!isLast) hiMask |= 0b0001;
    if (hiMask ) loMask |= 0b00000001;

    m_bitStream.Write(loMask, 8);
    if (loMask & 0b00000001) m_bitStream.Write(hiMask, 4);

    if (loMask & 0b10000000) WriteRegDelta(frame, chip, Mixer);
    if (loMask & 0b01000000) WritePerDelta(frame, chip, A_Period);
    if (loMask & 0b00100000) WriteRegDelta(frame, chip, A_Volume);
    if (loMask & 0b00010000) WritePerDelta(frame, chip, B_Period);
    if (loMask & 0b00001000) WriteRegDelta(frame, chip, B_Volume);
    if (loMask & 0b00000100) WritePerDelta(frame, chip, C_Period);
    if (loMask & 0b00000010) WriteRegDelta(frame, chip, C_Volume);
    if (hiMask & 0b1000) WritePerDelta(frame, chip, N_Period);
    if (hiMask & 0b0100) WritePerDelta(frame, chip, E_Period);
    if (hiMask & 0b0010) WriteRegDelta(frame, chip, E_Shape);
}

void EncodeAYM::WriteStepDelta()
{
    if (m_newStep != m_oldStep)
    {
        m_bitStream.Write(0x00, 8);
        WriteDelta({ m_oldStep, m_newStep });

        m_oldStep = m_newStep;
        m_newStep = 1;
    }
}

void EncodeAYM::WriteRegDelta(const Frame& frame, uint8_t chip, uint8_t reg)
{
    WriteDelta({ m_frame.Read(chip, reg), frame.Read(chip, reg) });
}

void EncodeAYM::WritePerDelta(const Frame& frame, uint8_t chip, uint8_t per)
{
    WriteDelta({ m_frame.ReadPeriod(chip, per), frame.ReadPeriod(chip, per) });
}
