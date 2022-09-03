#include "DecodePSG.h"

namespace
{
    const uint32_t PSGSignature = 0x1A475350;
}

bool DecodePSG::Open(Stream& stream)
{
    m_fileStream.open(stream.file, std::fstream::binary);
    if (m_fileStream)
    {
        Header header;
        m_fileStream.read((char*)(&header), sizeof(header));

        if (m_fileStream && header.m_sig == PSGSignature)
        {
            stream.info.type("PSG stream");
            stream.play.frameRate(header.m_fps ? header.m_fps : 50);
            return true;
        }
    }
	return false;
}

bool DecodePSG::Decode(Frame& frame)
{
    if (m_skipFrames)
    {
        m_skipFrames--;
        return true;
    }

    uint8_t data0, data1;
    while (m_fileStream.get((char&)data0))
    {
        // beginning of the next frame
        if (data0 == 0xFF) return true;

        // next frames are duplicates
        if (data0 == 0xFE) 
        {
            if (m_fileStream.get((char&)data0))
            {
                m_skipFrames = 4 * data0 - 1;
            }
            return true;
        }

        // end of stream
        if (data0 == 0xFD) break;

        if (m_fileStream.get((char&)data1))
        {
            frame[0].Update(data0, data1);
        }
    }
    return frame.HasChanges();
}

void DecodePSG::Close(Stream& stream)
{
    m_fileStream.close();
}
