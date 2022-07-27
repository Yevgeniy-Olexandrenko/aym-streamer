#include "DecodeSQT.h"

#pragma warning( disable : 4311)
#pragma warning( disable : 4302)

namespace
{
	const uint16_t SQT_Table[] =
	{ 
		0x0d5d, 0x0c9c, 0x0be7, 0x0b3c, 0x0a9b, 0x0a02, 0x0973, 0x08eb,
		0x086b, 0x07f2, 0x0780, 0x0714, 0x06ae, 0x064e, 0x05f4, 0x059e,
		0x054f, 0x0501, 0x04b9, 0x0475, 0x0435, 0x03f9, 0x03c0, 0x038a,
		0x0357, 0x0327, 0x02fa, 0x02cf, 0x02a7, 0x0281, 0x025d, 0x023b,
		0x021b, 0x01fc, 0x01e0, 0x01c5, 0x01ac, 0x0194, 0x017d, 0x0168,
		0x0153, 0x0140, 0x012e, 0x011d, 0x010d, 0x00fe, 0x00f0, 0x00e2,
		0x00d6, 0x00ca, 0x00be, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f,
		0x0087, 0x007f, 0x0078, 0x0071, 0x006b, 0x0065, 0x005f, 0x005a,
		0x0055, 0x0050, 0x004c, 0x0047, 0x0043, 0x0040, 0x003c, 0x0039,
		0x0035, 0x0032, 0x0030, 0x002d, 0x002a, 0x0028, 0x0026, 0x0024,
		0x0022, 0x0020, 0x001e, 0x001c, 0x001b, 0x0019, 0x0018, 0x0016,
		0x0015, 0x0014, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f, 0x000e 
	};
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

bool DecodeSQT::Open(Stream& stream)
{
    bool isDetected = false;
    if (CheckFileExt(stream, "sqt"))
    {
        std::ifstream fileStream;
        fileStream.open(stream.file, std::fstream::binary);

        if (fileStream)
        {
            fileStream.seekg(0, fileStream.end);
            uint32_t fileSize = (uint32_t)fileStream.tellg();

            if (fileSize >= sizeof(Header) + 5)
            {
                Header header;
                fileStream.seekg(0, fileStream.beg);
                fileStream.read((char*)(&header), sizeof(header));

                bool isHeaderOK = true;
                isHeaderOK &= (header.samplesPointer >= 10);
                isHeaderOK &= (header.ornamentsPointer > header.samplesPointer);
                isHeaderOK &= (header.patternsPointer >= header.ornamentsPointer);
                isHeaderOK &= (header.positionsPointer > header.patternsPointer);
                isHeaderOK &= (header.loopPointer >= header.positionsPointer);
                isHeaderOK &= (header.loopPointer - (header.samplesPointer - 10U) < fileSize);
               
                if (isHeaderOK)
                {
                    m_data = new uint8_t[fileSize];
                    fileStream.seekg(0, fileStream.beg);
                    fileStream.read((char*)m_data, fileSize);

                    // workaround to check if initialization was successful
                    m_positionsPointer = 0;
                    Init();

                    if (m_positionsPointer)
                    {
                        isDetected = true;

                        // seek for the number of last position
                        uint16_t positionPointer = m_positionsPointer;
                        while (m_data[positionPointer]) positionPointer += 7;
                        m_lastPosition = (positionPointer - m_positionsPointer) / 7;

                        stream.info.type("SQ-Tracker module");
                        stream.chip.clock(Chip::Clock::F1773400);
                        stream.play.frameRate(50);
                    }
                    else delete[] m_data;
                }
            }
            fileStream.close();
        }
    }
    return isDetected;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void DecodeSQT::Init()
{
    if (PreInit())
    {
        Header* header = (Header*)m_data;

        memset(&m_chA, 0, sizeof(Channel));
        memset(&m_chB, 0, sizeof(Channel));
        memset(&m_chC, 0, sizeof(Channel));

        m_delayCounter = m_delay = 1;
        m_linesCounter = 1;
        m_positionsPointer = header->positionsPointer;

        memset(&m_regs, 0, sizeof(m_regs));
    }
}

void DecodeSQT::Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition)
{
    Header* header = (Header*)m_data;
    currPosition = (m_positionsPointer  - header->positionsPointer) / 7 - 1;
    loopPosition = (header->loopPointer - header->positionsPointer) / 7;
    lastPosition = m_lastPosition;
}

bool DecodeSQT::Play()
{
    bool isNewLoop = false;
    Header* header = (Header*)m_data;
    uint8_t mixer  = 0;

    if (--m_delayCounter == 0)
    {
        m_delayCounter = m_delay;
        if (--m_linesCounter == 0)
        {
            isNewLoop |= NextPosition(m_chC);
            isNewLoop |= NextPosition(m_chB);
            isNewLoop |= NextPosition(m_chA);

            m_linesCounter = m_data[m_chC.addressInPattern - 1];
            m_delayCounter = m_delay = m_data[m_positionsPointer++];
        }
        PatternInterpreter(m_chC);
        PatternInterpreter(m_chB);
        PatternInterpreter(m_chA);
    }

    GetRegisters(m_chC, mixer);
    GetRegisters(m_chB, mixer);
    GetRegisters(m_chA, mixer);
    mixer = (-(mixer + 1)) & 0x3f;

    if (!m_chA.mixNoise) mixer |= 0x08;
    if (!m_chA.mixTon)   mixer |= 0x01;

    if (!m_chB.mixNoise) mixer |= 0x10;
    if (!m_chB.mixTon)   mixer |= 0x02;

    if (!m_chC.mixNoise) mixer |= 0x20;
    if (!m_chC.mixTon)   mixer |= 0x04;

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

bool DecodeSQT::PreInit()
{
    Header* header = (Header*)m_data;

    uint32_t j2;
    uint16_t* pwrd;

    int i = header->samplesPointer - 10;
    if (i < 0) return false;

    int i2 = header->positionsPointer - i;
    if (i2 < 0) return false;

    int i1 = 0;
    while (m_data[i2] != 0)
    {
        if (i2 > 65536 - 8) return false;

        if (i1 < (m_data[i2] & 0x7f)) i1 = m_data[i2] & 0x7f;
        i2 += 2;
        if (i1 < (m_data[i2] & 0x7f)) i1 = m_data[i2] & 0x7f;
        i2 += 2;
        if (i1 < (m_data[i2] & 0x7f)) i1 = m_data[i2] & 0x7f;
        i2 += 3;
    }

    j2 = (uint32_t)(&m_data[65535]);
    pwrd = (uint16_t*)(&header->samplesPointer);

    i1 = (header->patternsPointer - i + i1 * 2) / 2;
    if (i1 < 1) return false;

    for (i2 = 1; i2 <= i1; i2++)
    {
        if ((uint32_t)(pwrd) >= j2) return false;
        if (*pwrd < i) return false;

        *pwrd -= i;
        pwrd++;
    }
    return true;
}

bool DecodeSQT::NextPosition(Channel& chan)
{
    bool isNewLoop = false;
    Header* header = (Header*)m_data;

    if (m_data[m_positionsPointer] == 0)
    {
        m_positionsPointer = header->loopPointer;
        isNewLoop = true;
    }
    if ((int8_t)(m_data[m_positionsPointer]) < 0)
        chan.b4ix0 = true;
    else
        chan.b4ix0 = false;

    chan.addressInPattern = *(uint16_t*)(&m_data[(uint8_t)(m_data[m_positionsPointer] * 2) + header->patternsPointer]) + 1;
    m_positionsPointer++;

    chan.volume = (m_data[m_positionsPointer] & 0x0F);
    if ((m_data[m_positionsPointer] >> 4) < 9)
        chan.transposit = m_data[m_positionsPointer] >> 4;
    else
        chan.transposit = -((m_data[m_positionsPointer] >> 4) - 9) - 1;

    m_positionsPointer++;
    chan.ix21 = 0;

    return isNewLoop;
}

void DecodeSQT::PatternInterpreter(Channel& chan)
{
    uint16_t ptr = 0;
    if (chan.ix21 != 0)
    {
        chan.ix21--;
        if (chan.b7ix0)
            Call_LC191(chan, ptr);
        return;
    }
    ptr = chan.addressInPattern;
    chan.b6ix0 = true;
    chan.b7ix0 = false;
    while (true)
    {
        uint8_t val = m_data[ptr];
        if (val <= 0x5f)
        {
            chan.note = m_data[ptr];
            chan.ix27 = ptr;
            ptr++;
            Call_LC283(chan, ptr);
            if (chan.b6ix0)
                chan.addressInPattern = ptr;
            break;
        }
        else if (val >= 0x60 && val <= 0x6e)
        {
            Call_LC1D1(chan, ptr, m_data[ptr] - 0x60);
            break;
        }
        else if (val >= 0x6f && val <= 0x7f)
        {
            chan.mixNoise = false;
            chan.mixTon = false;
            chan.enabled = false;
            if (val != 0x6f)
                Call_LC1D1(chan, ptr, m_data[ptr] - 0x6f);
            else
                chan.addressInPattern = ptr + 1;
            break;
        }
        else if (val >= 0x80 && val <= 0xbf)
        {
            chan.addressInPattern = ptr + 1;
            if (val <= 0x9f)
            {
                if ((val & 16) == 0)
                    chan.note += val & 15;
                else
                    chan.note -= val & 15;
            }
            else
            {
                chan.ix21 = val & 15;
                if ((val & 16) == 0)
                    break;
                if (chan.ix21 != 0)
                    chan.b7ix0 = true;
            }
            Call_LC191(chan, ptr);
            break;
        }
        else if (val >= 0xc0)
        {
            chan.addressInPattern = ptr + 1;
            chan.ix27 = ptr;
            Call_LC2A8(chan, val & 31);
            break;
        }
    }
}

void DecodeSQT::GetRegisters(Channel& chan, uint8_t& mixer)
{
    mixer <<= 1;
    if (chan.enabled)
    {
        uint8_t b0 = m_data[chan.pointInSample];
        chan.amplitude = b0 & 15;
        if (chan.amplitude != 0)
        {
            chan.amplitude -= chan.volume;
            if ((int8_t)(chan.amplitude) < 0)
                chan.amplitude = 0;
        }
        else if (chan.envelopeEnabled)
            chan.amplitude = 16;

        uint8_t b1 = m_data[chan.pointInSample + 1];
        if ((b1 & 32) != 0)
        {
            mixer |= 8;
            uint8_t noise = (b0 & 0xf0) >> 3;
            if ((int8_t)(b1) < 0) noise++;
            m_regs[0][N_Period] = noise;
        }
        if ((b1 & 64) != 0)
        {
            mixer |= 1;
        }

        uint8_t note = chan.note;
        if (chan.ornamentEnabled)
        {
            note += m_data[chan.pointInOrnament];
            chan.ornamentTikCounter--;
            if (chan.ornamentTikCounter == 0)
            {
                if (m_data[chan.ornamentPointer] != 32)
                {
                    chan.ornamentTikCounter = m_data[chan.ornamentPointer + 1];
                    chan.pointInOrnament = chan.ornamentPointer + 2 + m_data[chan.ornamentPointer];
                }
                else
                {
                    chan.ornamentTikCounter = m_data[chan.samplePointer + 1];
                    chan.pointInOrnament = chan.ornamentPointer + 2 + m_data[chan.samplePointer];
                }
            }
            else
                chan.pointInOrnament++;
        }
        note += chan.transposit;
        if (note > 95) note = 95;

        if ((b1 & 16) == 0)
            chan.ton = SQT_Table[note] - (((uint16_t)(b1 & 15) << 8) + m_data[chan.pointInSample + 2]);
        else
            chan.ton = SQT_Table[note] + (((uint16_t)(b1 & 15) << 8) + m_data[chan.pointInSample + 2]);
        chan.sampleTikCounter--;
        if (chan.sampleTikCounter == 0)
        {
            chan.sampleTikCounter = m_data[chan.samplePointer + 1];
            if (m_data[chan.samplePointer] == 32)
            {
                chan.enabled = false;
                chan.ornamentEnabled = false;
            }
            chan.pointInSample = chan.samplePointer + 2 + m_data[chan.samplePointer] * 3;
        }
        else
            chan.pointInSample += 3;
        if (chan.gliss)
        {
            chan.ton += chan.currentTonSliding;
            chan.currentTonSliding += chan.tonSlideStep;
        }
        chan.ton = chan.ton & 0xfff;
    }
    else
        chan.amplitude = 0;
}

void DecodeSQT::Call_LC191(Channel& chan, uint16_t& ptr)
{
    ptr = chan.ix27;
    chan.b6ix0 = false;
    if (m_data[ptr] <= 0x7f)
    {
        ptr++;
        Call_LC283(chan, ptr);
    }
    else if (m_data[ptr] >= 0x80)
    {
        Call_LC2A8(chan, m_data[ptr] & 31);
    }
}

void DecodeSQT::Call_LC283(Channel& chan, uint16_t& ptr)
{
    uint8_t val = m_data[ptr];
    if (val <= 0x7f)
    {
        Call_LC1D1(chan, ptr, val);
    }
    else if (val >= 0x80)
    {
        if (((val >> 1) & 31) != 0)
            Call_LC2A8(chan, (val >> 1) & 31);

        if ((val & 64) != 0)
        {
            int Temp = m_data[ptr + 1] >> 4;
            if ((val & 1) != 0)
                Temp = Temp | 16;

            if (Temp != 0)
                Call_LC2D9(chan, Temp);

            ptr++;
            if ((m_data[ptr] & 15) != 0)
                Call_LC1D1(chan, ptr, m_data[ptr] & 15);
        }
    }
    ptr++;
}

void DecodeSQT::Call_LC1D1(Channel& chan, uint16_t& ptr, uint8_t a)
{
    ptr++;
    if (chan.b6ix0)
    {
        chan.addressInPattern = ptr + 1;
        chan.b6ix0 = false;
    }
    switch (a - 1)
    {
    case 0:
        if (chan.b4ix0)
            chan.volume = m_data[ptr] & 15;
        break;
    case 1:
        if (chan.b4ix0)
            chan.volume = (chan.volume + m_data[ptr]) & 15;
        break;
    case 2:
        if (chan.b4ix0)
        {
            m_chA.volume = m_data[ptr];
            m_chB.volume = m_data[ptr];
            m_chC.volume = m_data[ptr];
        }
        break;
    case 3:
        if (chan.b4ix0)
        {
            m_chA.volume = (m_chA.volume + m_data[ptr]) & 15;
            m_chB.volume = (m_chB.volume + m_data[ptr]) & 15;
            m_chC.volume = (m_chC.volume + m_data[ptr]) & 15;
        }
        break;
    case 4:
        if (chan.b4ix0)
        {
            m_delayCounter = m_data[ptr] & 31;
            if (m_delayCounter == 0)
                m_delayCounter = 32;
            m_delay = m_delayCounter;
        }
        break;
    case 5:
        if (chan.b4ix0)
        {
            m_delayCounter = (m_delayCounter + m_data[ptr]) & 31;
            if (m_delayCounter == 0)
                m_delayCounter = 32;
            m_delay = m_delayCounter;
        }
        break;
    case 6:
        chan.currentTonSliding = 0;
        chan.gliss = true;
        chan.tonSlideStep = -m_data[ptr];
        break;
    case 7:
        chan.currentTonSliding = 0;
        chan.gliss = true;
        chan.tonSlideStep = m_data[ptr];
        break;
    default:
        chan.envelopeEnabled = true;
        m_regs[0][E_Shape] = (a - 1) & 15;
        m_regs[0][E_Fine] = m_data[ptr];
        break;
    }
}

void DecodeSQT::Call_LC2A8(Channel& chan, uint8_t a)
{
    Header* header = (Header*)m_data;
    chan.envelopeEnabled = false;
    chan.ornamentEnabled = false;
    chan.gliss = false;
    chan.enabled = true;
    chan.samplePointer = *(uint16_t*)(&m_data[a * 2 + header->samplesPointer]);
    chan.pointInSample = chan.samplePointer + 2;
    chan.sampleTikCounter = 32;
    chan.mixNoise = true;
    chan.mixTon = true;
}

void DecodeSQT::Call_LC2D9(Channel& chan, uint8_t a)
{
    Header* header = (Header*)m_data;
    chan.ornamentPointer = *(uint16_t*)(&m_data[a * 2 + header->ornamentsPointer]);
    chan.pointInOrnament = chan.ornamentPointer + 2;
    chan.ornamentTikCounter = 32;
    chan.ornamentEnabled = true;
}
