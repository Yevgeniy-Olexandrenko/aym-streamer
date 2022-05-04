#include "Decode.h"

bool Decoder::CheckFileExt(const Stream& stream, const std::string& ext) const
{
    auto check_ext = "." + ext;
    auto extension = stream.file.extension().string();
    std::for_each(extension.begin(), extension.end(), [](char& c) { c = ::tolower(c); });
    return (extension == check_ext);
}

std::string Decoder::ReadString(uint8_t* ptr, uint8_t size) const
{
    char buf[256];
    memcpy(buf, ptr, size);
    buf[size] = 0;

    int i = size;
    while (i && buf[i - 1] == ' ') buf[--i] = 0;
    return std::string(buf);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

bool ModuleDecoder::Decode(Frame& frame)
{
    m_regs[0][Env_Shape] = 0xFF;
    m_regs[1][Env_Shape] = 0xFF;
    bool isNewLoop = Play();

    if (m_loop == 0)
    {
        uint8_t currPosition, lastPosition, loopPosition;
        Loop(currPosition, lastPosition, loopPosition);

        // detect true loop frame (ommit loop to first or last position)
        if (loopPosition > 0 && loopPosition < lastPosition && currPosition == loopPosition)
        {
            m_loop = m_frame;
        }
    }
    m_frame++;

    if (!isNewLoop)
    {
        for (uint8_t reg = 0; reg < 16; ++reg)
            frame.Update(0, reg, m_regs[0][reg]);

        if (m_isTS)
        {
            for (uint8_t reg = 0; reg < 16; ++reg)
                frame.Update(1, reg, m_regs[1][reg]);
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
