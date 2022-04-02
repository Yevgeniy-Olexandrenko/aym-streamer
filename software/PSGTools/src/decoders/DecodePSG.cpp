#include "DecodePSG.h"
#include "module/Module.h"

bool DecodePSG::Open(Module& module)
{
    m_fileStream.open(module.file, std::fstream::binary);
    if (m_fileStream)
    {
        PSGHeader hdr;
        m_fileStream.read((char*)(&hdr), sizeof(hdr));

        if (m_fileStream && hdr.m_1Ah == 0x1A
            && hdr.m_psg[0] == 'P'
            && hdr.m_psg[1] == 'S'
            && hdr.m_psg[2] == 'G')
        {
            module.info.type("PSG stream");
            module.playback.frameRate(hdr.m_fps ? hdr.m_fps : 50);

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

void DecodePSG::Close(Module& module)
{
    m_fileStream.close();
}
