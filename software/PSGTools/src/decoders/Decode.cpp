#include "Decode.h"
#include "stream/Stream.h"

bool ModuleDecoder::Decode(Frame& frame)
{
    m_regs[Env_Shape] = 0xFF;
    bool isNewLoop = Play();

    if (m_loop == 0)
    {
        uint8_t currPosition, lastPosition, loopPosition;
        Loop(currPosition, lastPosition, loopPosition);

        // detect true loop frame (ommit loop to first or last position)
        if (loopPosition > 0 && loopPosition < lastPosition && currPosition == loopPosition)
        {
            m_loop = m_tick;
        }
    }
    m_tick++;

    if (!isNewLoop)
    {
        for (uint8_t r = 0; r < 16; ++r)
        {
            uint8_t data = m_regs[r];
            if (r == Env_Shape)
            {
                if (data != 0xFF)
                    frame[r].first.override(data);
            }
            else
            {
                frame[r].first.update(data);
            }
        }
        return true;
    }

    // stop decoding on new loop
    return false;
}

void ModuleDecoder::Close(Stream& stream)
{
    if (m_loop) stream.loop.frameId(m_loop);
    delete[] m_data;
}

void ModuleDecoder::Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition)
{
    currPosition = lastPosition = loopPosition = 0;
}

std::string ModuleDecoder::ReadString(uint8_t* ptr, uint8_t size) const
{
    char buf[256];
    memcpy(buf, ptr, size);
    buf[size] = 0;

    int i = size;
    while (i && buf[i - 1] == ' ') buf[--i] = 0;
    return std::string(buf);
}
