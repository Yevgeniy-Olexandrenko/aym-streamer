
#include "DecodePSG.h"
#include "../module/Module.h"
#include "../module/Frame.h"

struct PSG_Header
{
    uint8_t m_psg[3];
    uint8_t m_1Ah;
    uint8_t m_ver;
    uint8_t m_fps;
    uint8_t m_bin[10];
};

bool DecodePSG::Open(Module& module)
{
    const std::string filePath = module.GetFilePath();
    m_fileStream.open(filePath, std::fstream::in | std::fstream::binary);

    if (m_fileStream)
    {
        PSG_Header hdr;
        m_fileStream.read((char*)(&hdr), sizeof(hdr));

        if (m_fileStream && hdr.m_1Ah == 0x1A
            && hdr.m_psg[0] == 'P'
            && hdr.m_psg[1] == 'S'
            && hdr.m_psg[2] == 'G')
        {
            module.SetType("PSG stream");
            module.SetFrameRate(hdr.m_fps ? hdr.m_fps : 50);

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
            frame[byte1].OverrideData(byte2);
        }
    }
    return false;
}

void DecodePSG::Close(Module& module)
{
    module.SetLoopUnavailable();
    m_fileStream.close();
}
