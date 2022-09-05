#include "BitStream.h"

bool BitStream::Open(std::ostream& stream)
{
    if (Close())
    {
        m_ostream = &stream;
        return true;
    }
    return false;
}

bool BitStream::Open(std::istream& stream)
{
    if (Close())
    {
        m_istream = &stream;
        return true;
    }
    return false;
}

std::ostream* BitStream::GetOStream()
{
    return m_ostream;
}

std::istream* BitStream::GetIStream()
{
    return m_istream;
}

bool BitStream::Write(uint32_t data, unsigned size)
{
    if (m_ostream)
    {
        auto maxSize = sizeof(data) * 8;
        size = std::min(size, maxSize);
        data <<= (maxSize - size);

        bool ok = true;
        for (; size && ok; --size)
        {
            m_buffer <<= 1;
            m_buffer |= (data >> (maxSize - 1) & 1);
            data <<= 1;
            m_count++;

            if (m_count == 8)
            {
                ok = bool(m_ostream->put(m_buffer));
                m_buffer = m_count = 0;
            }
        }
#if 0
        Align();
#endif
        return ok;
    }
    return false;
}

bool BitStream::Read(uint32_t& data, unsigned size)
{
    if (m_istream)
    {
        // TODO
    }
    return false;
}

bool BitStream::Align()
{
    bool ok = true;
    if (m_ostream && m_count)
    {
        m_buffer <<= (8 - m_count);
        ok = bool(m_ostream->put(m_buffer));
    }
    m_buffer = m_count = 0;
    return ok;
}

bool BitStream::Close()
{
    bool ok = Align();
    m_ostream = nullptr;
    m_istream = nullptr;
    return ok;
}
