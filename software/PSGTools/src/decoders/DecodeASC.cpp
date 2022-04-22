#include "DecodeASC.h"
#include "stream/Stream.h"

bool DecodeASC::Open(Stream& stream)
{
    bool isDetected = false;
    std::ifstream fileStream;
    fileStream.open(stream.file, std::fstream::binary);

    if (fileStream)
    {
        fileStream.seekg(0, fileStream.end);
        uint32_t fileSize = (uint32_t)fileStream.tellg();

        if (fileSize >= 9)
        {
            Header header;
            fileStream.seekg(0, fileStream.beg);
            fileStream.read((char*)(&header), std::min(sizeof(header), fileSize));

            bool isHeaderOK = true;
            isHeaderOK &= (header.patternsPointers < fileSize);
            isHeaderOK &= (header.samplesPointers < fileSize);
            isHeaderOK &= (header.ornamentsPointers < fileSize);

            if (isHeaderOK)
            {
                m_data = new uint8_t[fileSize];
                fileStream.seekg(0, fileStream.beg);
                fileStream.read((char*)m_data, fileSize);

                if (fileStream)
                {
                    auto titleId = (uint8_t*)(&header.positions[header.numberOfPositions]);
                    if (!memcmp(titleId, "ASM COMPILATION OF ", 19))
                    {
                        auto artistId = (titleId + 19 + 20);
                        if (!memcmp(artistId, " BY ", 4))
                        {
                            stream.info.title(ReadString(titleId + 19, 20));
                            stream.info.artist(ReadString(artistId + 4, 20));
                        }
                        else
                        {
                            stream.info.title(ReadString(titleId + 19, 20 + 4 + 20));
                        }                        
                    }

                    stream.info.type("ASC Sound Master module");
                    stream.playback.frameRate(50);

                    Init();
                    m_loop = m_tick = 0;
                    isDetected = true;
                }
            }
        }
    }
    return isDetected;
}

namespace
{
	const uint16_t ASCNoteTable[] =
	{ 
		0x0edc, 0x0e07, 0x0d3e, 0x0c80, 0x0bcc, 0x0b22, 0x0a82, 0x09ec,
		0x095c, 0x08d6, 0x0858, 0x07e0, 0x076e, 0x0704, 0x069f, 0x0640,
		0x05e6, 0x0591, 0x0541, 0x04f6, 0x04ae, 0x046b, 0x042c, 0x03f0,
		0x03b7, 0x0382, 0x034f, 0x0320, 0x02f3, 0x02c8, 0x02a1, 0x027b,
		0x0257, 0x0236, 0x0216, 0x01f8, 0x01dc, 0x01c1, 0x01a8, 0x0190,
		0x0179, 0x0164, 0x0150, 0x013d, 0x012c, 0x011b, 0x010b, 0x00fc,
		0x00ee, 0x00e0, 0x00d4, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f,
		0x0096, 0x008d, 0x0085, 0x007e, 0x0077, 0x0070, 0x006a, 0x0064,
		0x005e, 0x0059, 0x0054, 0x0050, 0x004b, 0x0047, 0x0043, 0x003f,
		0x003c, 0x0038, 0x0035, 0x0032, 0x002f, 0x002d, 0x002a, 0x0028,
		0x0026, 0x0024, 0x0022, 0x0020, 0x001e, 0x001c 
	};
}

void DecodeASC::Init()
{
    Header* header = (Header*)m_data;
    
    memset(&m_chA, 0, sizeof(Channel));
    memset(&m_chB, 0, sizeof(Channel));
    memset(&m_chC, 0, sizeof(Channel));

    m_currentPosition = 0;
    m_delayCounter = 1;
    m_delay = header->delay;
    
    uint16_t ascPatPt = header->patternsPointers;
    m_chA.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[0] + 0]) + ascPatPt;
    m_chB.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[0] + 2]) + ascPatPt;
    m_chC.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[0] + 4]) + ascPatPt;

    memset(&m_regs, 0, sizeof(m_regs));
}

void DecodeASC::Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition)
{
    Header* header = (Header*)m_data;
    currPosition = m_currentPosition;
    loopPosition = header->loopingPosition;
    lastPosition = header->numberOfPositions - 1;
}

bool DecodeASC::Play()
{
    bool isNewLoop = false;
    Header* header = (Header*)m_data;
    uint8_t mixer  = 0;

    if (--m_delayCounter <= 0)
    {
        if (--m_chA.noteSkipCounter < 0)
        {
            if (m_data[m_chA.addressInPattern] == 255)
            {
                if (++m_currentPosition >= header->numberOfPositions)
                {
                    m_currentPosition = header->loopingPosition;
                    isNewLoop = true;
                }

                uint16_t ascPatPt = header->patternsPointers;
                m_chA.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[m_currentPosition] + 0]) + ascPatPt;
                m_chB.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[m_currentPosition] + 2]) + ascPatPt;
                m_chC.addressInPattern = (*(uint16_t*)&m_data[ascPatPt + 6 * header->positions[m_currentPosition] + 4]) + ascPatPt;

                m_chA.initialNoise = 0;
                m_chB.initialNoise = 0;
                m_chC.initialNoise = 0;
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

void DecodeASC::PatternInterpreter(Channel& chan)
{
    Header* header = (Header*)m_data;

    int16_t deltaTon;
    bool initOfOrnamentDisabled = false;
    bool initOfSampleDisabled = false;

    chan.tonSlidingCounter = 0;
    chan.amplitudeDelayCounter = 0;

    while (true)
    {
        uint8_t val = m_data[chan.addressInPattern];
        if (val <= 0x55)
        {
            chan.note = val;
            chan.addressInPattern++;
            chan.currentNoise = chan.initialNoise;
            if ((int8_t)(chan.tonSlidingCounter) <= 0)
                chan.currentTonSliding = 0;

            if (!initOfSampleDisabled)
            {
                chan.additionToAmplitude = 0;
                chan.tonDeviation = 0;
                chan.pointInSample = chan.initialPointInSample;
                chan.soundEnabled = true;
                chan.sampleFinished = false;
                chan.breakSampleLoop = false;
            }
            if (!initOfOrnamentDisabled)
            {
                chan.pointInOrnament = chan.initialPointInOrnament;
                chan.additionToNote = 0;
            }
            if (chan.envelopeEnabled)
            {
                m_regs[Env_PeriodL] = m_data[chan.addressInPattern++];
            }
            break;
        }
        else if ((val >= 0x56) && (val <= 0x5d))
        {
            chan.addressInPattern++;
            break;
        }
        else if (val == 0x5e)
        {
            chan.breakSampleLoop = true;
            chan.addressInPattern++;
            break;
        }
        else if (val == 0x5f)
        {
            chan.soundEnabled = false;
            chan.addressInPattern++;
            break;
        }
        else if ((val >= 0x60) && (val <= 0x9f))
        {
            chan.numberOfNotesToSkip = val - 0x60;
        }
        else if ((val >= 0xa0) && (val <= 0xbf))
        {
            chan.initialPointInSample = (*(uint16_t*)&m_data[(val - 0xa0) * 2 + header->samplesPointers]) + header->samplesPointers;
        }
        else if ((val >= 0xc0) && (val <= 0xdf))
        {
            chan.initialPointInOrnament = (*(uint16_t*)&m_data[(val - 0xc0) * 2 + header->ornamentsPointers]) + header->ornamentsPointers;
        }
        else if (val == 0xe0)
        {
            chan.volume = 15;
            chan.envelopeEnabled = true;
        }
        else if ((val >= 0xe1) && (val <= 0xef))
        {
            chan.volume = val - 0xe0;
            chan.envelopeEnabled = false;
        }
        else if (val == 0xf0)
        {
            chan.initialNoise = m_data[++chan.addressInPattern];
        }
        else if (val == 0xf1)
        {
            initOfSampleDisabled = true;
        }
        else if (val == 0xf2)
        {
            initOfOrnamentDisabled = true;
        }
        else if (val == 0xf3)
        {
            initOfSampleDisabled = true;
            initOfOrnamentDisabled = true;
        }
        else if (val == 0xf4)
        {
            m_delay = m_data[++chan.addressInPattern];
        }
        else if (val == 0xf5)
        {
            chan.substructionForTonSliding = -(int8_t)(m_data[++chan.addressInPattern]) * 16;
            chan.tonSlidingCounter = 255;
        }
        else if (val == 0xf6)
        {
            chan.substructionForTonSliding = (int8_t)(m_data[++chan.addressInPattern]) * 16;
            chan.tonSlidingCounter = 255;
        }
        else if (val == 0xf7)
        {
            chan.addressInPattern++;
            initOfSampleDisabled = true;
            if (m_data[chan.addressInPattern + 1] < 0x56)
                deltaTon = ASCNoteTable[chan.note] + (chan.currentTonSliding / 16) - ASCNoteTable[m_data[chan.addressInPattern + 1]];
            else
                deltaTon = chan.currentTonSliding / 16;
            deltaTon = deltaTon << 4;
            chan.substructionForTonSliding = -deltaTon / (int8_t)(m_data[chan.addressInPattern]);
            chan.currentTonSliding = deltaTon - deltaTon % (int8_t)(m_data[chan.addressInPattern]);
            chan.tonSlidingCounter = (int8_t)(m_data[chan.addressInPattern]);
        }
        else if (val == 0xf8)
        {
            m_regs[Env_Shape] = 8;
        }
        else if (val == 0xf9)
        {
            chan.addressInPattern++;
            if (m_data[chan.addressInPattern + 1] < 0x56)
            {
                deltaTon = ASCNoteTable[chan.note] - ASCNoteTable[m_data[chan.addressInPattern + 1]];
            }
            else
                deltaTon = chan.currentTonSliding / 16;
            deltaTon = deltaTon << 4;
            chan.substructionForTonSliding = -deltaTon / (int8_t)(m_data[chan.addressInPattern]);
            chan.currentTonSliding = deltaTon - deltaTon % (int8_t)(m_data[chan.addressInPattern]);
            chan.tonSlidingCounter = (int8_t)(m_data[chan.addressInPattern]);

        }
        else if (val == 0xfa)
        {
            m_regs[Env_Shape] = 10;
        }
        else if (val == 0xfb)
        {
            chan.addressInPattern++;
            if ((m_data[chan.addressInPattern] & 32) == 0)
            {
                chan.amplitudeDelay = m_data[chan.addressInPattern] << 3;
                chan.amplitudeDelayCounter = chan.amplitudeDelay;
            }
            else
            {
                chan.amplitudeDelay = ((m_data[chan.addressInPattern] << 3) ^ 0xf8) + 9;
                chan.amplitudeDelayCounter = chan.amplitudeDelay;
            }
        }
        else if (val == 0xfc)
        {
            m_regs[Env_Shape] = 12;
        }
        else if (val == 0xfe)
        {
            m_regs[Env_Shape] = 14;
        }
        chan.addressInPattern++;
    }
    chan.noteSkipCounter = chan.numberOfNotesToSkip;
}

void DecodeASC::GetRegisters(Channel& chan, uint8_t& mixer)
{
    bool sampleSaysOKforEnvelope;
    if (chan.sampleFinished || !chan.soundEnabled)
        chan.amplitude = 0;
    else
    {
        if (chan.amplitudeDelayCounter != 0)
        {
            if (chan.amplitudeDelayCounter >= 16)
            {
                chan.amplitudeDelayCounter -= 8;
                if (chan.additionToAmplitude < -15)
                    chan.additionToAmplitude++;
                else if (chan.additionToAmplitude > 15)
                    chan.additionToAmplitude--;
            }
            else
            {
                if ((chan.amplitudeDelayCounter & 1) != 0)
                {
                    if (chan.additionToAmplitude > -15)
                        chan.additionToAmplitude--;
                }
                else if (chan.additionToAmplitude < 15)
                    chan.additionToAmplitude++;
                chan.amplitudeDelayCounter = chan.amplitudeDelay;
            }
        }

        if ((m_data[chan.pointInSample] & 128) != 0)
            chan.loopPointInSample = chan.pointInSample;

        if ((m_data[chan.pointInSample] & 96) == 32)
            chan.sampleFinished = true;

        chan.tonDeviation += (int8_t)(m_data[chan.pointInSample + 1]);

        mixer |= (m_data[chan.pointInSample + 2] & 9) << 3;

        if ((m_data[chan.pointInSample + 2] & 6) == 2)
            sampleSaysOKforEnvelope = true;
        else
            sampleSaysOKforEnvelope = false;

        if ((m_data[chan.pointInSample + 2] & 6) == 4)
        {
            if (chan.additionToAmplitude > -15)
                chan.additionToAmplitude--;
        }

        if ((m_data[chan.pointInSample + 2] & 6) == 6)
        {
            if (chan.additionToAmplitude < 15)
                chan.additionToAmplitude++;
        }

        chan.amplitude = chan.additionToAmplitude + (m_data[chan.pointInSample + 2] >> 4);
        if ((int8_t)(chan.amplitude) < 0)
            chan.amplitude = 0;
        else if (chan.amplitude > 15)
            chan.amplitude = 15;
        chan.amplitude = (chan.amplitude * (chan.volume + 1)) >> 4;

        if (sampleSaysOKforEnvelope && ((mixer & 64) != 0))
        {
            uint8_t data = m_regs[Env_PeriodL];
            data += ((int8_t)(m_data[chan.pointInSample] << 3) / 8);
            m_regs[Env_PeriodL] = data;
        }
        else
            chan.currentNoise += (int8_t)(m_data[chan.pointInSample] << 3) / 8;

        chan.pointInSample += 3;
        if ((m_data[chan.pointInSample - 3] & 64) != 0)
        {
            if (!chan.breakSampleLoop)
                chan.pointInSample = chan.loopPointInSample;
            else if ((m_data[chan.pointInSample - 3] & 32) != 0)
                chan.sampleFinished = true;
        }

        if ((m_data[chan.pointInOrnament] & 128) != 0)
            chan.loopPointInOrnament = chan.pointInOrnament;

        chan.additionToNote += m_data[chan.pointInOrnament + 1];
        chan.currentNoise += (-(int8_t)(m_data[chan.pointInOrnament] & 0x10)) | m_data[chan.pointInOrnament];
        chan.pointInOrnament += 2;

        if ((m_data[chan.pointInOrnament - 2] & 64) != 0)
            chan.pointInOrnament = chan.loopPointInOrnament;

        if ((mixer & 64) == 0)
        {
            uint8_t data = ((uint8_t)(chan.currentTonSliding >> 8) + chan.currentNoise) & 0x1f;
            m_regs[Noise_Period] = data;
        }

        int8_t note = chan.note + chan.additionToNote;
        if (note < 0) note = 0;
        else if (note > 0x55) note = 0x55;

        chan.ton = (ASCNoteTable[note] + chan.tonDeviation + (chan.currentTonSliding / 16)) & 0xfff;
        if (chan.tonSlidingCounter != 0)
        {
            if ((int8_t)(chan.tonSlidingCounter) > 0)
                chan.tonSlidingCounter--;
            chan.currentTonSliding += chan.substructionForTonSliding;
        }
        if (chan.envelopeEnabled && sampleSaysOKforEnvelope)
            chan.amplitude |= 0x10;
    }
    mixer = mixer >> 1;
}
