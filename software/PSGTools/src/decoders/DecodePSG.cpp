#include "DecodePSG.h"
#include "../module/Module.h"
#include "../module/Frame.h"

struct PSG_Header
{
    char _psg[3];
    char _1a;
    char ver;
    char freq;
    char data[10];
};

bool DecodePSG::InitModule(std::ifstream& file, Module& module)
{
    PSG_Header hdr;
    file.read((char*)(&hdr), sizeof(hdr));

    if (file && hdr._psg[0] == 'P' && hdr._psg[1] == 'S' && hdr._psg[2] == 'G' && hdr._1a == 0x1A)
    {
        // TODO

        m_skipFrames = 0;
        return true;
    }
	return false;
}

bool DecodePSG::DecodeFrame(std::ifstream& file, Frame& frame)
{
    if (m_skipFrames)
    {
        m_skipFrames--;
        return true;
    }

    uint8_t byte1, byte2;
    while (file.get((char&)byte1))
    {
        if (byte1 == 0xFF)
        {
            // end of frame
            return true;
        }

        if (byte1 == 0xFE)
        {
            // delay (duplicated frames)
            if (file.get((char&)byte1))
            {
                m_skipFrames = 4 * byte1 - 1;
            }
            return true;
        }

        if (byte1 == 0xFD)
        {
            // end of stream
            break;
        }

        if (file.get((char&)byte2))
        {
            frame[byte1] = byte2;
        }
    }

	return false;
}
