#include "DecodeVGM.h"
#include "module/Module.h"

bool DecodeVGM::Open(Module& module)
{
    bool isDetected = false;
    std::ifstream fileStream;
    fileStream.open(module.file, std::fstream::binary);

    if (fileStream)
    {
        fileStream.seekg(0, fileStream.end);
        uint32_t fileSize = (uint32_t)fileStream.tellg();

        if (fileSize >= sizeof(Header))
        {
            Header header;
            fileStream.seekg(0, fileStream.beg);
            fileStream.read((char*)(&header), sizeof(header));

            if (header.ident == 0x206D6756 && header.eofOffset == fileSize - 4 && header.ay8910Clock)
            {
                m_rawData = new uint8_t[fileSize];
                fileStream.seekg(0, fileStream.beg);
                fileStream.read((char*)m_rawData, fileSize);

                if (fileStream)
                {
                    uint32_t vgmDataOffset = 0x40;
                    if (header.version >= 0x00000150 && header.vgmDataOffset)
                    {
                        vgmDataOffset = header.vgmDataOffset + 0x34;
                    }
                    m_dataPtr = m_rawData + vgmDataOffset;

                    bool divider = (header.ay8910Flags & 0x10);
                    bool ym_chip = (header.ay8910Type  & 0xF0);
                    module.chip.model(ym_chip ? Chip::Model::YM : Chip::Model::AY);
                    module.chip.freqValue(header.ay8910Clock / (divider ? 2 : 1));

                    if (header.rate)
                        module.playback.frameRate(header.rate);
                    else
                        module.playback.frameRate(DetectFrameRate());
                    m_samplesPerFrame = 44100 / module.playback.frameRate();
                    m_waitForSamples  = 0;

                    if (header.loopSamples)
                    {
                        m_loop = (header.totalSamples - header.loopSamples) / m_samplesPerFrame;
                    }
                    
                    module.info.type("VGM stream");
                    isDetected = true;
                }
            }
        }
        fileStream.close();
    }
    return isDetected;
}

bool DecodeVGM::Decode(Frame& frame)
{
    while (m_waitForSamples < m_samplesPerFrame)
    {
        // AY8910, write value dd to register aa
        if (*m_dataPtr == 0xA0)
        {
            uint8_t aa = m_dataPtr[1];
            uint8_t dd = m_dataPtr[2];
            frame[aa].first.override(dd);
            m_dataPtr += 2;
        }

        // Wait n samples, n can range from 0 to 65535 (approx 1.49 seconds).
        // Longer pauses than this are represented by multiple wait commands.
        else if (*m_dataPtr == 0x61)
        {
            m_waitForSamples += *(uint16_t*)(m_dataPtr + 1);
            m_dataPtr += 2;
        }

        // Wait 735 samples (60th of a second), a shortcut for 0x61 0xdf 0x02
        else if (*m_dataPtr == 0x62)
        {
            m_samplesPerFrame = 735;
            m_waitForSamples += m_samplesPerFrame;
        }

        // Wait 882 samples (50th of a second), a shortcut for 0x61 0x72 0x03
        else if (*m_dataPtr == 0x63)
        {
            m_samplesPerFrame = 882;
            m_waitForSamples += m_samplesPerFrame;
        }

        // End of sound data
        else if (*m_dataPtr == 0x66)
        {
            return false;
        }

        // Wait n+1 samples, n can range from 0 to 15.
        else if ((*m_dataPtr & 0xF0) == 0x70)
        {
            m_waitForSamples += (1 + *m_dataPtr & 0x0F);
        }

        // Unknown command, stop decoding
        else
        {
            return false;
        }
        m_dataPtr++;
    }
    m_waitForSamples -= m_samplesPerFrame;
    return true;
}

void DecodeVGM::Close(Module& module)
{
    if (m_loop) module.loop.frameId(m_loop);
    delete m_rawData;
}

int DecodeVGM::DetectFrameRate()
{
    uint8_t* dataPtrSaved = m_dataPtr;
    int frameRate = 0;

    Frame frame;
    m_samplesPerFrame = INT32_MAX;
    m_waitForSamples  = 0;

    while (Decode(frame))
    {
        if (m_samplesPerFrame != INT32_MAX)
        {
            frameRate = 44100 / m_samplesPerFrame;
            break;
        }
    }

    if (!frameRate)
    {
        // TODO: detect in some another way,
        // set some default value for now
        frameRate = 50;
    }

    m_dataPtr = dataPtrSaved;
    return frameRate;
}
