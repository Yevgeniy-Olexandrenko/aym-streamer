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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

bool EncodeAYM::Open(const Stream& stream)
{
    if (CheckFileExt(stream, "aym"))
    {
        m_output.open(stream.file, std::fstream::binary);
        if (m_output)
        {
            m_isTS = (stream.chip.count() == 2);

            m_output << "AYYM";
            m_bitStream.Open(m_output);

            // TODO

            return true;
        }
    }
    return false;
}

void EncodeAYM::Encode(const Frame& frame)
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

////////////////////////////////////////////////////////////////////////////////

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

void EncodeAYM::WriteChipDelta(const Frame& frame, int chip, bool isLast)
{
    uint8_t loMask = 0;
    uint8_t hiMask = 0;

    if (frame.IsChanged(Mixer))          loMask |= (1 << 0);
    if (frame.IsChangedPeriod(A_Period)) loMask |= (1 << 1);
    if (frame.IsChanged(A_Volume))       loMask |= (1 << 2);
    if (frame.IsChangedPeriod(B_Period)) loMask |= (1 << 3);
    if (frame.IsChanged(B_Volume))       loMask |= (1 << 4);
    if (frame.IsChangedPeriod(C_Period)) loMask |= (1 << 5);
    if (frame.IsChanged(C_Volume))       loMask |= (1 << 6);
    if (frame.IsChangedPeriod(N_Period)) hiMask |= (1 << 0);
    if (frame.IsChangedPeriod(E_Period)) hiMask |= (1 << 1);
    if (frame.IsChanged(E_Shape))        hiMask |= (1 << 2);

    if (!isLast) hiMask |= (1 << 3);
    if (hiMask ) loMask |= (1 << 7);

    m_bitStream.Write(loMask, 8);
    if (loMask & (1 << 7)) m_bitStream.Write(hiMask, 4);

    if (loMask & (1 << 0)) WriteRDelta(frame, chip, Mixer);
    if (loMask & (1 << 1)) WritePDelta(frame, chip, A_Period);
    if (loMask & (1 << 2)) WriteRDelta(frame, chip, A_Volume);
    if (loMask & (1 << 3)) WritePDelta(frame, chip, B_Period);
    if (loMask & (1 << 4)) WriteRDelta(frame, chip, B_Volume);
    if (loMask & (1 << 5)) WritePDelta(frame, chip, C_Period);
    if (loMask & (1 << 6)) WriteRDelta(frame, chip, C_Volume);
    if (hiMask & (1 << 0)) WritePDelta(frame, chip, N_Period);
    if (hiMask & (1 << 1)) WritePDelta(frame, chip, E_Period);
    if (hiMask & (1 << 2)) WriteRDelta(frame, chip, E_Shape);
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

void EncodeAYM::WriteRDelta(const Frame& frame, int chip, Register r)
{
    WriteDelta({ m_frame.Read(chip, r), frame.Read(chip, r) });
}

void EncodeAYM::WritePDelta(const Frame& frame, int chip, PeriodRegister p)
{
    WriteDelta({ m_frame.ReadPeriod(chip, p), frame.ReadPeriod(chip, p) });
}
