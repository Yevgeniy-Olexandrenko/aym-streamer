#include "DecodeSTP.h"

namespace
{
    const uint16_t STPNoteTable[] =
    {
        0x0ef8, 0x0e10, 0x0d60, 0x0c80, 0x0bd8, 0x0b28, 0x0a88, 0x09f0,
        0x0960, 0x08e0, 0x0858, 0x07e0, 0x077c, 0x0708, 0x06b0, 0x0640,
        0x05ec, 0x0594, 0x0544, 0x04f8, 0x04b0, 0x0470, 0x042c, 0x03f0,
        0x03be, 0x0384, 0x0358, 0x0320, 0x02f6, 0x02ca, 0x02a2, 0x027c,
        0x0258, 0x0238, 0x0216, 0x01f8, 0x01df, 0x01c2, 0x01ac, 0x0190,
        0x017b, 0x0165, 0x0151, 0x013e, 0x012c, 0x011c, 0x010b, 0x00fc,
        0x00ef, 0x00e1, 0x00d6, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f,
        0x0096, 0x008e, 0x0085, 0x007e, 0x0077, 0x0070, 0x006b, 0x0064,
        0x005e, 0x0059, 0x0054, 0x004f, 0x004b, 0x0047, 0x0042, 0x003f,
        0x003b, 0x0038, 0x0035, 0x0032, 0x002f, 0x002c, 0x002a, 0x0027,
        0x0025, 0x0023, 0x0021, 0x001f, 0x001d, 0x001c, 0x001a, 0x0019,
        0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f
    };

    const std::string KSASignature = "KSA SOFTWARE COMPILATION OF ";
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

bool DecodeSTP::Open(Stream& stream)
{
    bool isDetected = false;
    if (CheckFileExt(stream, "stp"))
    {
        std::ifstream fileStream;
        fileStream.open(stream.file, std::fstream::binary);

        if (fileStream)
        {
            fileStream.seekg(0, fileStream.end);
            uint32_t fileSize = (uint32_t)fileStream.tellg();

            if (fileSize >= sizeof(Header))
            {
                Header header;
                fileStream.seekg(0, fileStream.beg);
                fileStream.read((char*)(&header), sizeof(header));

                bool isHeaderOK = true;
                isHeaderOK &= (header.positionsPointer < fileSize&& header.patternsPointer < fileSize);
                isHeaderOK &= (header.ornamentsPointer < fileSize&& header.samplesPointer < fileSize);
                isHeaderOK &= (int(header.samplesPointer - header.ornamentsPointer) == 0x20);
                isHeaderOK &= (int(header.ornamentsPointer - header.patternsPointer) > 0);
                isHeaderOK &= (int(header.ornamentsPointer - header.patternsPointer) % 6 == 0);

                if (isHeaderOK)
                {
                    m_data = new uint8_t[fileSize];
                    fileStream.seekg(0, fileStream.beg);
                    fileStream.read((char*)m_data, fileSize);

                    Init();
                    isDetected = true;

                    if (!memcmp(&m_data[10], KSASignature.data(), KSASignature.size()))
                    {
                        stream.info.title(ReadString(&m_data[38], 25));
                    }

                    stream.info.type("Sound Tracker Pro module");
                    stream.playback.frameRate(50);
                }
            }
            fileStream.close();
        }
    }
    return isDetected;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void DecodeSTP::Init()
{
    Header* header = (Header*)m_data;
    
    memset(&m_chA, 0, sizeof(Channel));
    memset(&m_chB, 0, sizeof(Channel));
    memset(&m_chC, 0, sizeof(Channel));

    m_delayCounter = 1;
    m_transposition = m_data[header->positionsPointer + 3];
    m_currentPosition = 0;

    uint16_t patternPointer = header->patternsPointer + m_data[header->positionsPointer + 2];
    m_chA.addressInPattern = *(uint16_t*)(&m_data[header->patternsPointer + m_data[header->positionsPointer + 2] + 0]);
    m_chB.addressInPattern = *(uint16_t*)(&m_data[header->patternsPointer + m_data[header->positionsPointer + 2] + 2]);
    m_chC.addressInPattern = *(uint16_t*)(&m_data[header->patternsPointer + m_data[header->positionsPointer + 2] + 4]);

    for (Channel* chan : { &m_chA, &m_chB, &m_chC })
    {
        chan->samplePointer = *(uint16_t*)(&m_data[header->samplesPointer]);
        chan->loopSamplePosition = m_data[chan->samplePointer++];
        chan->sampleLength = m_data[chan->samplePointer++];

        chan->ornamentPointer = *(uint16_t*)(&m_data[header->ornamentsPointer]);
        chan->loopOrnamentPosition = m_data[chan->ornamentPointer++];
        chan->ornamentLength = m_data[chan->ornamentPointer++];
    }

    memset(&m_regs, 0, sizeof(m_regs));
}

void DecodeSTP::Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition)
{
    Header* header = (Header*)m_data;
    currPosition = m_currentPosition;
    loopPosition = m_data[header->positionsPointer + 1];
    lastPosition = m_data[header->positionsPointer + 0] - 1;
}

bool DecodeSTP::Play()
{
    bool isNewLoop = false;
    Header* header = (Header*)m_data;
    uint8_t mixer = 0;

    if (--m_delayCounter == 0)
    {
        if (--m_chA.noteSkipCounter < 0)
        {
            if (m_data[m_chA.addressInPattern] == 0)
            {
                if (++m_currentPosition == m_data[header->positionsPointer + 0])
                {
                    m_currentPosition = m_data[header->positionsPointer + 1];
                    isNewLoop = true;
                }

                uint16_t patternPointer = header->patternsPointer + m_data[header->positionsPointer + 2 + m_currentPosition * 2];
                m_chA.addressInPattern = *(uint16_t*)(&m_data[patternPointer + 0]);
                m_chB.addressInPattern = *(uint16_t*)(&m_data[patternPointer + 2]);
                m_chC.addressInPattern = *(uint16_t*)(&m_data[patternPointer + 4]);

                m_transposition = m_data[header->positionsPointer + 3 + m_currentPosition * 2];
            }
            PatternInterpreter(m_chA);
        }

        if (--m_chB.noteSkipCounter < 0) PatternInterpreter(m_chB);
        if (--m_chC.noteSkipCounter < 0) PatternInterpreter(m_chC);

        m_delayCounter = header->delay;
    }

    GetRegisters(m_chA, mixer);
    GetRegisters(m_chB, mixer);
    GetRegisters(m_chC, mixer);

    m_regs[0][Mixer] = mixer;
    m_regs[0][A_Fine] = m_chA.ton & 0xff;
    m_regs[0][A_Coarse] = (m_chA.ton >> 8) & 0xf;
    m_regs[0][B_Fine] = m_chB.ton & 0xff;
    m_regs[0][B_Coarse] = (m_chB.ton >> 8) & 0xf;
    m_regs[0][C_Fine] = m_chC.ton & 0xff;
    m_regs[0][C_Coarse] = (m_chC.ton >> 8) & 0xf;
    m_regs[0][A_Volume] = m_chA.amplitude;
    m_regs[0][B_Volume] = m_chB.amplitude;
    m_regs[0][C_Volume] = m_chC.amplitude;
    return isNewLoop;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void DecodeSTP::PatternInterpreter(Channel& chan)
{
    Header* header = (Header*)m_data;
    bool quit = false;

    do
    {
        uint8_t val = m_data[chan.addressInPattern];
        if (val >= 1 && val <= 0x60)
        {
            chan.note = val - 1;
            chan.positionInSample = 0;
            chan.positionInOrnament = 0;
            chan.currentTonSliding = 0;
            chan.enabled = true;
            quit = true;
        }
        else if (val >= 0x61 && val <= 0x6f)
        {
            chan.samplePointer = *(uint16_t*)(&m_data[header->samplesPointer + (val - 0x61) * 2]);
            chan.loopSamplePosition = m_data[chan.samplePointer++];
            chan.sampleLength = m_data[chan.samplePointer++];
        }
        else if (val >= 0x70 && val <= 0x7f)
        {
            chan.ornamentPointer = *(uint16_t*)(&m_data[header->ornamentsPointer + (val - 0x70) * 2]);
            chan.loopOrnamentPosition = m_data[chan.ornamentPointer++];
            chan.ornamentLength = m_data[chan.ornamentPointer++];
            chan.envelopeEnabled = false;
            chan.glissade = 0;
        }
        else if (val >= 0x80 && val <= 0xbf)
        {
            chan.numberOfNotesToSkip = val - 0x80;
        }
        else if (val >= 0xc0 && val <= 0xcf)
        {
            if (val != 0xc0)
            {
                m_regs[0][E_Shape] = val - 0xc0;
                m_regs[0][E_Fine] = m_data[++chan.addressInPattern];
            }
            chan.envelopeEnabled = true;
            chan.loopOrnamentPosition = 0;
            chan.glissade = 0;
            chan.ornamentLength = 1;
        }
        else if (val >= 0xd0 && val <= 0xdf)
        {
            chan.enabled = false;
            quit = true;
        }
        else if (val >= 0xe0 && val <= 0xef)
        {
            quit = true;
        }
        else if (val == 0xf0)
        {
            chan.glissade = m_data[++chan.addressInPattern];
        }
        else if (val >= 0xf1 && val <= 0xff)
        {
            chan.volume = val - 0xf1;
        }
        chan.addressInPattern++;
    } while (!quit);
    chan.noteSkipCounter = chan.numberOfNotesToSkip;
}

void DecodeSTP::GetRegisters(Channel& chan, uint8_t& mixer)
{
    Header* header = (Header*)m_data;
    
    if (chan.enabled)
    {
        uint8_t note;
        chan.currentTonSliding += chan.glissade;
        if (chan.envelopeEnabled)
            note = chan.note + m_transposition;
        else
            note = chan.note + m_transposition + m_data[chan.ornamentPointer + chan.positionInOrnament];
        if (note > 95) note = 95;

        uint8_t b0 = m_data[chan.samplePointer + chan.positionInSample * 4 + 0];
        uint8_t b1 = m_data[chan.samplePointer + chan.positionInSample * 4 + 1];

        chan.ton = (STPNoteTable[note] + chan.currentTonSliding + (*(uint16_t*)(&m_data[chan.samplePointer + chan.positionInSample * 4 + 2]))) & 0xfff;
        chan.amplitude = (b0 & 15) - chan.volume;

        if ((int8_t)(chan.amplitude) < 0)
            chan.amplitude = 0;

        if (((b1 & 1) != 0) && chan.envelopeEnabled)
            chan.amplitude = chan.amplitude | 16;

        mixer |= ((b0 >> 1) & 0x48);
        if ((int8_t)(b0) >= 0)
        {
            m_regs[0][N_Period] = ((b1 >> 1) & 31);
        }

        if (++chan.positionInOrnament >= chan.ornamentLength)
            chan.positionInOrnament = chan.loopOrnamentPosition;

        if (++chan.positionInSample >= chan.sampleLength)
        {
            chan.positionInSample = chan.loopSamplePosition;
            if ((int8_t)(chan.loopSamplePosition) < 0)
                chan.enabled = false;
        }
    }
    else
    {
        mixer |= 0x48;
        chan.amplitude = 0;
    }
    mixer >>= 1;
}
