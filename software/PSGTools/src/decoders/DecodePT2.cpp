#include "DecodePT2.h"
#include "module/Module.h"

bool DecodePT2::Open(Module& module)
{
    bool isDetected = false;
    std::ifstream fileStream;
    fileStream.open(module.file, std::fstream::binary);

    if (fileStream)
    {
        fileStream.seekg(0, fileStream.end);
        uint32_t fileSize = (uint32_t)fileStream.tellg();

        if (fileSize > 131)
        {
            uint8_t data[131 + 1];
            fileStream.seekg(0, fileStream.beg);
            fileStream.read((char*)data, sizeof(data));
            Header* header = (Header*)data;

            bool isHeaderOk = true;
            isHeaderOk &= (header->delay >= 3);
            isHeaderOk &= (header->numberOfPositions > 0);
            isHeaderOk &= (header->samplesPointers[0] == 0);
            isHeaderOk &= (header->patternsPointer < fileSize);
            isHeaderOk &= (header->ornamentsPointers[0] - header->samplesPointers[0] - 2 <= fileSize);
            isHeaderOk &= (header->ornamentsPointers[0] - header->samplesPointers[0] >= 0);

            if (isHeaderOk)
            {
                m_data = new uint8_t[fileSize];
                fileStream.seekg(0, fileStream.beg);
                fileStream.read((char*)m_data, fileSize);

                if (fileStream)
                {
                    module.info.title(ReadString(header->musicName, 30));
                    module.info.type("ProTracker 2.x module");
                    module.playback.frameRate(50);

                    Init();
                    m_loop = m_tick = 0;
                    isDetected = true;
                }
            }
        }
        fileStream.close();
    }
	return isDetected;
}

namespace
{
    const uint16_t PT2NoteTable[96] =
    {
        0x0ef8, 0x0e10, 0x0d60, 0x0c80, 0x0bd8, 0x0b28, 0x0a88, 0x09f0,
        0x0960, 0x08e0, 0x0858, 0x07e0, 0x077c, 0x0708, 0x06b0, 0x0640,
        0x05ec, 0x0594, 0x0544, 0x04f8, 0x04b0, 0x0470, 0x042c, 0x03fd,
        0x03be, 0x0384, 0x0358, 0x0320, 0x02f6, 0x02ca, 0x02a2, 0x027c,
        0x0258, 0x0238, 0x0216, 0x01f8, 0x01df, 0x01c2, 0x01ac, 0x0190,
        0x017b, 0x0165, 0x0151, 0x013e, 0x012c, 0x011c, 0x010a, 0x00fc,
        0x00ef, 0x00e1, 0x00d6, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f,
        0x0096, 0x008e, 0x0085, 0x007e, 0x0077, 0x0070, 0x006b, 0x0064,
        0x005e, 0x0059, 0x0054, 0x004f, 0x004b, 0x0047, 0x0042, 0x003f,
        0x003b, 0x0038, 0x0035, 0x0032, 0x002f, 0x002c, 0x002a, 0x0027,
        0x0025, 0x0023, 0x0021, 0x001f, 0x001d, 0x001c, 0x001a, 0x0019,
        0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f,
    };
}

void DecodePT2::Init()
{
    Header* header = (Header*)m_data;

    memset(&m_chA, 0, sizeof(Channel));
    memset(&m_chB, 0, sizeof(Channel));
    memset(&m_chC, 0, sizeof(Channel));

    m_delay = header->delay;
    m_delayCounter = 1;
    m_currentPosition = 0;

    uint16_t patternsPointer = header->patternsPointer;
    patternsPointer += header->positionList[0] * 6;
    m_chA.addressInPattern = m_data[patternsPointer + 0] | m_data[patternsPointer + 1] << 8;
    m_chB.addressInPattern = m_data[patternsPointer + 2] | m_data[patternsPointer + 3] << 8;
    m_chC.addressInPattern = m_data[patternsPointer + 4] | m_data[patternsPointer + 5] << 8;

    for (Channel* chan : { &m_chA, &m_chB, &m_chC })
    {
        chan->ornamentPointer = header->ornamentsPointers[0];
        chan->ornamentLength = m_data[chan->ornamentPointer++];
        chan->loopOrnamentPosition = m_data[chan->ornamentPointer++];
        chan->volume = 15;
    }

    memset(&m_regs, 0, sizeof(m_regs));
}

void DecodePT2::Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition)
{
    Header* header = (Header*)m_data;
    currPosition = m_currentPosition;
    loopPosition = header->loopPosition;
    lastPosition = header->numberOfPositions - 1;
}

bool DecodePT2::Play()
{
    bool isNewLoop = false;
    Header* header = (Header*)m_data;
    uint8_t mixer  = 0;

    if (--m_delayCounter == 0)
    {
        if (--m_chA.noteSkipCounter < 0)
        {
            if (m_data[m_chA.addressInPattern] == 0)
            {
                if (++m_currentPosition == header->numberOfPositions)
                {
                    m_currentPosition = header->loopPosition;
                    isNewLoop = true;
                }

                uint16_t patternsPointer = header->patternsPointer;
                patternsPointer += header->positionList[m_currentPosition] * 6;
                m_chA.addressInPattern = m_data[patternsPointer + 0] | m_data[patternsPointer + 1] << 8;
                m_chB.addressInPattern = m_data[patternsPointer + 2] | m_data[patternsPointer + 3] << 8;
                m_chC.addressInPattern = m_data[patternsPointer + 4] | m_data[patternsPointer + 5] << 8;
            }
            PatternInterpreter(m_chA);
        }

        if (--m_chB.noteSkipCounter < 0) PatternInterpreter(m_chB);
        if (--m_chC.noteSkipCounter < 0) PatternInterpreter(m_chC);
        m_delayCounter = m_delay;
    }

    GetRegisters(m_chA, mixer);
    GetRegisters(m_chB, mixer);
    GetRegisters(m_chC, mixer);

    m_regs[Mixer_Flags] = mixer;
    m_regs[TonA_PeriodL] = m_chA.ton & 0xff;
    m_regs[TonA_PeriodH] = (m_chA.ton >> 8) & 0xf;
    m_regs[TonB_PeriodL] = m_chB.ton & 0xff;
    m_regs[TonB_PeriodH] = (m_chB.ton >> 8) & 0xf;
    m_regs[TonC_PeriodL] = m_chC.ton & 0xff;
    m_regs[TonC_PeriodH] = (m_chC.ton >> 8) & 0xf;
    m_regs[VolA_EnvFlg] = m_chA.amplitude;
    m_regs[VolB_EnvFlg] = m_chB.amplitude;
    m_regs[VolC_EnvFlg] = m_chC.amplitude;
    return isNewLoop;
}

void DecodePT2::PatternInterpreter(Channel& chan)
{
    Header* header = (Header*)m_data;
    bool quit = false;
    bool gliss = false;

    do
    {
        uint8_t val = m_data[chan.addressInPattern];
        if (val >= 0xe1)
        {
            chan.samplePointer = header->samplesPointers[val - 0xe0];
            chan.sampleLength = m_data[chan.samplePointer++];
            chan.loopSamplePosition = m_data[chan.samplePointer++];
        }
        else if (val == 0xe0)
        {
            chan.positionInSample = 0;
            chan.positionInOrnament = 0;
            chan.currentTonSliding = 0;
            chan.glissType = 0;
            chan.enabled = false;
            quit = true;
        }
        else if (val >= 0x80 && val <= 0xdf)
        {
            chan.positionInSample = 0;
            chan.positionInOrnament = 0;
            chan.currentTonSliding = 0;
            if (gliss)
            {
                chan.slideToNote = val - 0x80;
                if (chan.glissType == 1)
                    chan.note = chan.slideToNote;
            }
            else
            {
                chan.note = val - 0x80;
                chan.glissType = 0;
            }
            chan.enabled = true;
            quit = true;
        }
        else if (val == 0x7f)
        {
            chan.envelopeEnabled = false;
        }
        else if (val >= 0x71 && val <= 0x7e)
        {
            chan.envelopeEnabled = true;
            m_regs[Env_Shape] = val - 0x70;
            m_regs[Env_PeriodL] = m_data[++chan.addressInPattern];
            m_regs[Env_PeriodH] = m_data[++chan.addressInPattern];
        }
        else if (val == 0x70)
        {
            quit = true;
        }
        else if (val >= 0x60 && val <= 0x6f)
        {
            chan.ornamentPointer = header->ornamentsPointers[val - 0x60];
            chan.ornamentLength = m_data[chan.ornamentPointer++];
            chan.loopOrnamentPosition = m_data[chan.ornamentPointer++];
            chan.positionInOrnament = 0;
        }
        else if (val >= 0x20 && val <= 0x5f)
        {
            chan.numberOfNotesToSkip = val - 0x20;
        }
        else if (val >= 0x10 && val <= 0x1f)
        {
            chan.volume = val - 0x10;
        }
        else if (val == 0xf)
        {
            m_delay = m_data[++chan.addressInPattern];
        }
        else if (val == 0xe)
        {
            chan.glissade = m_data[++chan.addressInPattern];
            chan.glissType = 1;
            gliss = true;
        }
        else if (val == 0xd)
        {
            chan.glissade = std::abs((int8_t)(m_data[++chan.addressInPattern]));

            // Do not use precalculated Ton_Delta to
            // avoide error with first note of pattern
            chan.addressInPattern += 2; 
            
            chan.glissType = 2;
            gliss = true;
        }
        else if (val == 0xc)
        {
            chan.glissType = 0;
        }
        else
        {
            chan.additionToNoise = m_data[++chan.addressInPattern];
        }
        chan.addressInPattern++;
    } while (!quit);

    if (gliss && (chan.glissType == 2))
    {
        chan.tonDelta = std::abs(PT2NoteTable[chan.slideToNote] - PT2NoteTable[chan.note]);
        if (chan.slideToNote > chan.note)
            chan.glissade = -chan.glissade;
    }
    chan.noteSkipCounter = chan.numberOfNotesToSkip;
}

void DecodePT2::GetRegisters(Channel& chan, uint8_t& mixer)
{
    uint8_t note, b0, b1;
    Header* header = (Header*)m_data;

    if (chan.enabled)
    {
        uint16_t samplePointer = chan.samplePointer + chan.positionInSample * 3;
        b0 = m_data[samplePointer + 0];
        b1 = m_data[samplePointer + 1];
        chan.ton = m_data[samplePointer + 2] + (uint16_t)((b1 & 15) << 8);
        if ((b0 & 4) == 0)
            chan.ton = -chan.ton;

        note = chan.note + m_data[chan.ornamentPointer + chan.positionInOrnament];
        if (note > 95) note = 95;
        chan.ton = (chan.ton + chan.currentTonSliding + PT2NoteTable[note]) & 0xfff;

        if (chan.glissType == 2)
        {
            chan.tonDelta = chan.tonDelta - std::abs(chan.glissade);
            if (chan.tonDelta < 0)
            {
                chan.note = chan.slideToNote;
                chan.glissType = 0;
                chan.currentTonSliding = 0;
            }
        }
        if (chan.glissType != 0)
            chan.currentTonSliding += chan.glissade;

        chan.amplitude = (chan.volume * 17 + (uint8_t)(chan.volume > 7)) * (b1 >> 4) / 256;
        if (chan.envelopeEnabled) chan.amplitude |= 16;

        if ((b0 & 1) != 0)
            mixer |= 64;
        else
            m_regs[Noise_Period] = ((b0 >> 3) + chan.additionToNoise) & 0x1f;

        if ((b0 & 2) != 0)
            mixer |= 8;

        if (++chan.positionInSample == chan.sampleLength)
            chan.positionInSample = chan.loopSamplePosition;

        if (++chan.positionInOrnament == chan.ornamentLength)
            chan.positionInOrnament = chan.loopOrnamentPosition;
    }
    else
    {
        chan.amplitude = 0;
    }
    mixer >>= 1;
}
