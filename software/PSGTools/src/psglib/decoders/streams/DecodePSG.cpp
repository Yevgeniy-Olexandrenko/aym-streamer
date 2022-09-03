#include "DecodePSG.h"

namespace
{
    const uint32_t PSGSignature = 0x1A475350;
}

bool DecodePSG::Open(Stream& stream)
{
    m_input.open(stream.file, std::fstream::binary);
    if (m_input)
    {
        Header header;
        m_input.read((char*)(&header), sizeof(header));

        if (m_input && header.m_sig == PSGSignature)
        {
            stream.info.type("PSG stream");
            stream.play.frameRate(header.m_fps ? header.m_fps : 50);

            m_skip = 0;
            return true;
        }
    }
	return false;
}

bool DecodePSG::Decode(Frame& frame)
{
    if (m_skip) m_skip--;
    else
    {
        // skip frame begin marker 0xFF or
        // pending argument of 0xFE command
        if (!m_input.ignore(1)) return false;

        // loop through register values
        // until next frame marker
        for(uint8_t v1, v2;;)
        {
            if (m_input.peek() == EOF ) break;
            if (m_input.peek() == 0xFF) break; 
            if (m_input.peek() == 0xFE)
            {
                // skip 0xFE command byte
                m_input.ignore(1);

                // peek next byte if available
                if (m_input.peek() == EOF) return false;
                v2 = uint8_t(m_input.peek());

                // setup number of frames to skip
                m_skip = (v2 * 4 - 1);
                return true;
            }

            // read register and value
            m_input.get((char&)v1);
            if (!m_input.get((char&)v2)) return false;

            // update frame state
            int chip = (v1 >> 7);
            Register reg = (v1 & 0x7F);
            frame[chip].Update(reg, v2);
        }
    }
    return true;
}

void DecodePSG::Close(Stream& stream)
{
    m_input.close();

    if (stream.IsExpandedModeUsed(0))
        stream.chip.first.model(Chip::Model::AY8930);

    if (stream.IsExpandedModeUsed(1))
        stream.chip.second.model(Chip::Model::AY8930);

    if (stream.IsSecondChipUsed())
        stream.chip.second.model(stream.chip.first.model());
}
