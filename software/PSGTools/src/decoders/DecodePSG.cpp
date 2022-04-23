#include "DecodePSG.h"

bool DecodePSG::Open(Stream& stream)
{
    m_fileStream.open(stream.file, std::fstream::binary);
    if (m_fileStream)
    {
        Header header;
        m_fileStream.read((char*)(&header), sizeof(header));

        if (m_fileStream && header.m_1Ah == 0x1A
            && header.m_psg[0] == 'P'
            && header.m_psg[1] == 'S'
            && header.m_psg[2] == 'G')
        {
            stream.info.type("PSG stream");
            stream.playback.frameRate(header.m_fps ? header.m_fps : 50);

            m_skipFrames = 0;
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

    uint8_t byte1, byte2;
    while (m_fileStream.get((char&)byte1))
    {
        // end of frame
        if (byte1 == 0xFF) return true;

        // delay (duplicated frames)
        if (byte1 == 0xFE) 
        {
            if (m_fileStream.get((char&)byte1))
            {
                m_skipFrames = 4 * byte1 - 1;
            }
            return true;
        }

        // end of stream
        if (byte1 == 0xFD) break;

        if (m_fileStream.get((char&)byte2))
        {
            frame[byte1].first.override(byte2);
        }
    }
    return false;
}

void DecodePSG::Close(Stream& stream)
{
    m_fileStream.close();
}
