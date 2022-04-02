#include <fstream>
#include "DecodePT3.h"
#include "module/Module.h"

namespace
{
    const int PT3NoteTable_PT_33_34r[] = {
        0x0C21,0x0B73,0x0ACE,0x0A33,0x09A0,0x0916,0x0893,0x0818,0x07A4,0x0736,0x06CE,0x066D,
        0x0610,0x05B9,0x0567,0x0519,0x04D0,0x048B,0x0449,0x040C,0x03D2,0x039B,0x0367,0x0336,
        0x0308,0x02DC,0x02B3,0x028C,0x0268,0x0245,0x0224,0x0206,0x01E9,0x01CD,0x01B3,0x019B,
        0x0184,0x016E,0x0159,0x0146,0x0134,0x0122,0x0112,0x0103,0x00F4,0x00E6,0x00D9,0x00CD,
        0x00C2,0x00B7,0x00AC,0x00A3,0x009A,0x0091,0x0089,0x0081,0x007A,0x0073,0x006C,0x0066,
        0x0061,0x005B,0x0056,0x0051,0x004D,0x0048,0x0044,0x0040,0x003D,0x0039,0x0036,0x0033,
        0x0030,0x002D,0x002B,0x0028,0x0026,0x0024,0x0022,0x0020,0x001E,0x001C,0x001B,0x0019,
        0x0018,0x0016,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000E,0x000D,0x000C };

    const int PT3NoteTable_PT_34_35[] = {
        0x0C22,0x0B73,0x0ACF,0x0A33,0x09A1,0x0917,0x0894,0x0819,0x07A4,0x0737,0x06CF,0x066D,
        0x0611,0x05BA,0x0567,0x051A,0x04D0,0x048B,0x044A,0x040C,0x03D2,0x039B,0x0367,0x0337,
        0x0308,0x02DD,0x02B4,0x028D,0x0268,0x0246,0x0225,0x0206,0x01E9,0x01CE,0x01B4,0x019B,
        0x0184,0x016E,0x015A,0x0146,0x0134,0x0123,0x0112,0x0103,0x00F5,0x00E7,0x00DA,0x00CE,
        0x00C2,0x00B7,0x00AD,0x00A3,0x009A,0x0091,0x0089,0x0082,0x007A,0x0073,0x006D,0x0067,
        0x0061,0x005C,0x0056,0x0052,0x004D,0x0049,0x0045,0x0041,0x003D,0x003A,0x0036,0x0033,
        0x0031,0x002E,0x002B,0x0029,0x0027,0x0024,0x0022,0x0020,0x001F,0x001D,0x001B,0x001A,
        0x0018,0x0017,0x0016,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000E,0x000D,0x000C };

    const int PT3NoteTable_ST[] = {
        0x0EF8,0x0E10,0x0D60,0x0C80,0x0BD8,0x0B28,0x0A88,0x09F0,0x0960,0x08E0,0x0858,0x07E0,
        0x077C,0x0708,0x06B0,0x0640,0x05EC,0x0594,0x0544,0x04F8,0x04B0,0x0470,0x042C,0x03FD,
        0x03BE,0x0384,0x0358,0x0320,0x02F6,0x02CA,0x02A2,0x027C,0x0258,0x0238,0x0216,0x01F8,
        0x01DF,0x01C2,0x01AC,0x0190,0x017B,0x0165,0x0151,0x013E,0x012C,0x011C,0x010A,0x00FC,
        0x00EF,0x00E1,0x00D6,0x00C8,0x00BD,0x00B2,0x00A8,0x009F,0x0096,0x008E,0x0085,0x007E,
        0x0077,0x0070,0x006B,0x0064,0x005E,0x0059,0x0054,0x004F,0x004B,0x0047,0x0042,0x003F,
        0x003B,0x0038,0x0035,0x0032,0x002F,0x002C,0x002A,0x0027,0x0025,0x0023,0x0021,0x001F,
        0x001D,0x001C,0x001A,0x0019,0x0017,0x0016,0x0015,0x0013,0x0012,0x0011,0x0010,0x000F };

    const int PT3NoteTable_ASM_34r[] = {
        0x0D3E,0x0C80,0x0BCC,0x0B22,0x0A82,0x09EC,0x095C,0x08D6,0x0858,0x07E0,0x076E,0x0704,
        0x069F,0x0640,0x05E6,0x0591,0x0541,0x04F6,0x04AE,0x046B,0x042C,0x03F0,0x03B7,0x0382,
        0x034F,0x0320,0x02F3,0x02C8,0x02A1,0x027B,0x0257,0x0236,0x0216,0x01F8,0x01DC,0x01C1,
        0x01A8,0x0190,0x0179,0x0164,0x0150,0x013D,0x012C,0x011B,0x010B,0x00FC,0x00EE,0x00E0,
        0x00D4,0x00C8,0x00BD,0x00B2,0x00A8,0x009F,0x0096,0x008D,0x0085,0x007E,0x0077,0x0070,
        0x006A,0x0064,0x005E,0x0059,0x0054,0x0050,0x004B,0x0047,0x0043,0x003F,0x003C,0x0038,
        0x0035,0x0032,0x002F,0x002D,0x002A,0x0028,0x0026,0x0024,0x0022,0x0020,0x001E,0x001D,
        0x001B,0x001A,0x0019,0x0018,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000E };

    const int PT3NoteTable_ASM_34_35[] = {
        0x0D10,0x0C55,0x0BA4,0x0AFC,0x0A5F,0x09CA,0x093D,0x08B8,0x083B,0x07C5,0x0755,0x06EC,
        0x0688,0x062A,0x05D2,0x057E,0x052F,0x04E5,0x049E,0x045C,0x041D,0x03E2,0x03AB,0x0376,
        0x0344,0x0315,0x02E9,0x02BF,0x0298,0x0272,0x024F,0x022E,0x020F,0x01F1,0x01D5,0x01BB,
        0x01A2,0x018B,0x0174,0x0160,0x014C,0x0139,0x0128,0x0117,0x0107,0x00F9,0x00EB,0x00DD,
        0x00D1,0x00C5,0x00BA,0x00B0,0x00A6,0x009D,0x0094,0x008C,0x0084,0x007C,0x0075,0x006F,
        0x0069,0x0063,0x005D,0x0058,0x0053,0x004E,0x004A,0x0046,0x0042,0x003E,0x003B,0x0037,
        0x0034,0x0031,0x002F,0x002C,0x0029,0x0027,0x0025,0x0023,0x0021,0x001F,0x001D,0x001C,
        0x001A,0x0019,0x0017,0x0016,0x0015,0x0014,0x0012,0x0011,0x0010,0x000F,0x000E,0x000D };

    const int PT3NoteTable_REAL_34r[] = {
        0x0CDA,0x0C22,0x0B73,0x0ACF,0x0A33,0x09A1,0x0917,0x0894,0x0819,0x07A4,0x0737,0x06CF,
        0x066D,0x0611,0x05BA,0x0567,0x051A,0x04D0,0x048B,0x044A,0x040C,0x03D2,0x039B,0x0367,
        0x0337,0x0308,0x02DD,0x02B4,0x028D,0x0268,0x0246,0x0225,0x0206,0x01E9,0x01CE,0x01B4,
        0x019B,0x0184,0x016E,0x015A,0x0146,0x0134,0x0123,0x0113,0x0103,0x00F5,0x00E7,0x00DA,
        0x00CE,0x00C2,0x00B7,0x00AD,0x00A3,0x009A,0x0091,0x0089,0x0082,0x007A,0x0073,0x006D,
        0x0067,0x0061,0x005C,0x0056,0x0052,0x004D,0x0049,0x0045,0x0041,0x003D,0x003A,0x0036,
        0x0033,0x0031,0x002E,0x002B,0x0029,0x0027,0x0024,0x0022,0x0020,0x001F,0x001D,0x001B,
        0x001A,0x0018,0x0017,0x0016,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000E,0x000D };

    const int PT3NoteTable_REAL_34_35[] = {
        0x0CDA,0x0C22,0x0B73,0x0ACF,0x0A33,0x09A1,0x0917,0x0894,0x0819,0x07A4,0x0737,0x06CF,
        0x066D,0x0611,0x05BA,0x0567,0x051A,0x04D0,0x048B,0x044A,0x040C,0x03D2,0x039B,0x0367,
        0x0337,0x0308,0x02DD,0x02B4,0x028D,0x0268,0x0246,0x0225,0x0206,0x01E9,0x01CE,0x01B4,
        0x019B,0x0184,0x016E,0x015A,0x0146,0x0134,0x0123,0x0112,0x0103,0x00F5,0x00E7,0x00DA,
        0x00CE,0x00C2,0x00B7,0x00AD,0x00A3,0x009A,0x0091,0x0089,0x0082,0x007A,0x0073,0x006D,
        0x0067,0x0061,0x005C,0x0056,0x0052,0x004D,0x0049,0x0045,0x0041,0x003D,0x003A,0x0036,
        0x0033,0x0031,0x002E,0x002B,0x0029,0x0027,0x0024,0x0022,0x0020,0x001F,0x001D,0x001B,
        0x001A,0x0018,0x0017,0x0016,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000E,0x000D };

    const uint8_t PT3VolumeTable_33_34[16][16] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x02},
        {0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03},
        {0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x02,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x04},
        {0x00,0x00,0x00,0x01,0x01,0x01,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x04,0x05,0x05},
        {0x00,0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06},
        {0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07},
        {0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x05,0x05,0x06,0x06,0x07,0x07,0x08},
        {0x00,0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x05,0x05,0x06,0x06,0x07,0x08,0x08,0x09},
        {0x00,0x00,0x01,0x02,0x02,0x03,0x04,0x04,0x05,0x06,0x06,0x07,0x08,0x08,0x09,0x0A},
        {0x00,0x00,0x01,0x02,0x03,0x03,0x04,0x05,0x06,0x06,0x07,0x08,0x09,0x09,0x0A,0x0B},
        {0x00,0x00,0x01,0x02,0x03,0x04,0x04,0x05,0x06,0x07,0x08,0x08,0x09,0x0A,0x0B,0x0C},
        {0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D},
        {0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F} };

    const uint8_t PT3VolumeTable_35[16][16] = {
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01},
        {0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02},
        {0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03},
        {0x00,0x00,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x04,0x04},
        {0x00,0x00,0x01,0x01,0x01,0x02,0x02,0x02,0x03,0x03,0x03,0x04,0x04,0x04,0x05,0x05},
        {0x00,0x00,0x01,0x01,0x02,0x02,0x02,0x03,0x03,0x04,0x04,0x04,0x05,0x05,0x06,0x06},
        {0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07},
        {0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,0x08},
        {0x00,0x01,0x01,0x02,0x02,0x03,0x04,0x04,0x05,0x05,0x06,0x07,0x07,0x08,0x08,0x09},
        {0x00,0x01,0x01,0x02,0x03,0x03,0x04,0x05,0x05,0x06,0x07,0x07,0x08,0x09,0x09,0x0A},
        {0x00,0x01,0x01,0x02,0x03,0x04,0x04,0x05,0x06,0x07,0x07,0x08,0x09,0x0A,0x0A,0x0B},
        {0x00,0x01,0x02,0x02,0x03,0x04,0x05,0x06,0x06,0x07,0x08,0x09,0x0A,0x0A,0x0B,0x0C},
        {0x00,0x01,0x02,0x03,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0A,0x0B,0x0C,0x0D},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E},
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F} };

    const std::string SignaturePT = "ProTracker 3.";
    const std::string SignatureVT = "Vortex Tracker II";
}

bool DecodePT3::Open(Module& module)
{
    bool isDetected = false;
    std::ifstream fileStream;
    fileStream.open(module.file, std::fstream::binary);

    if (fileStream)
    {
        uint8_t signature[30];
        fileStream.read((char*)signature, 30);

        bool isPT = !memcmp(signature, SignaturePT.data(), SignaturePT.size());
        bool isVT = !memcmp(signature, SignatureVT.data(), SignatureVT.size());

        if (isPT || isVT)
        {
            isDetected = true;

            fileStream.seekg(0, fileStream.end);
            m_size = (uint32_t)fileStream.tellg();

            fileStream.seekg(0, fileStream.beg);
            m_data = new uint8_t[m_size];

            fileStream.read((char*)m_data, m_size);

            Init();
            m_tick = 0;
            m_loop = 0;

            auto GetTextProperty = [&](int offset, int size)
            {
                char buf[33];
                memcpy(buf, m_data + offset, size);

                int i = size;
                buf[i] = 0;
                while (i && buf[i - 1] == ' ') buf[--i] = 0;

                return std::string(buf);
            };

            module.info.type(GetTextProperty(0x00, isVT ? 21 : 14) + " module");
            module.info.title(GetTextProperty(0x1E, 32));
            module.info.artist(GetTextProperty(0x42, 32));
            module.playback.frameRate(50);

            if (m_chip[0].header->tonTableId == 1)
            {
                module.chip.frequency(Chip::Frequency::F1773400);
            }
            else if (m_chip[0].header->tonTableId == 2 && m_ver > 3)
            {
                module.chip.frequency(Chip::Frequency::F1750000);
            }

            if (m_ts) module.chip.count(Chip::Count::TurboSound);
        }
        fileStream.close();
    }
	return isDetected;
}

bool DecodePT3::Decode(Frame& frame)
{
    // stop decoding on new loop
    if (Step()) return false;

    for (uint8_t r = 0; r < 16; ++r)
    {
        uint8_t data = m_regs[0][r];
        if (r == Env_Shape)
        {
            if (data != 0xFF) 
                frame[r].first.override(data);
        }
        else
        {
            frame[r].first.update(data);
        }

        if (m_ts)
        {
            data = m_regs[1][r];
            if (r == Env_Shape)
            {
                if (data != 0xFF) 
                    frame[r].second.override(data);
            }
            else
            {
                frame[r].second.update(data);
            }
        }
    }
    return true;
}

void DecodePT3::Close(Module& module)
{
    if (m_loop > 0) 
        module.loop.frameId(m_loop);
    
    delete[] m_data;
}

////////////////////////////////////////////////////////////////////////////////

bool DecodePT3::Init()
{
    // InitForAllTypes

    memset(&m_chip, 0, sizeof(m_chip));
    memset(&m_regs, 0, sizeof(m_regs));

    // PrepareItem / CaseTrModules

    m_chip[0].header = (Header*)m_data;
    m_chip[1].header = (Header*)m_data;
    m_chip[0].data = m_data;
    m_chip[1].data = m_data;

    uint8_t v = m_chip[0].header->musicName[13];
    m_ver = ('0' <= v && v <= '9') ? v - '0' : 6;
    m_chip[0].ts = m_chip[1].ts = 0x20;
    int TS = m_chip[0].header->musicName[98];
    m_ts = (TS != 0x20);
    if (m_ts) {
        m_chip[1].ts = TS;
    }
    else if (m_size > 400 && !memcmp(m_data + m_size - 4, "02TS", 4)) { // try load Vortex II '02TS'
        uint16_t sz1 = m_data[m_size - 12] + 0x100 * m_data[m_size - 11];
        uint16_t sz2 = m_data[m_size - 6] + 0x100 * m_data[m_size - 5];
        if (sz1 + sz2 < m_size && sz1 > 200 && sz2 > 200) {
            m_ts = true;
            m_chip[1].data = m_data + sz1;
            m_chip[1].header = (Header*)m_chip[1].data;
        }
    }

    // InitTrackerModule code

    for (int cnum = 0; cnum < 2; cnum++) {
        m_chip[cnum].glob.delay = m_chip[cnum].header->delay;
        m_chip[cnum].glob.delayCounter = 1;
        int b = m_chip[cnum].ts;
        uint8_t i = m_chip[cnum].header->positionList[0];
        if (b != 0x20) i = (uint8_t)(3 * b - 3 - i);
        for (int chan = 0; chan < 3; chan++) {
            m_chip[cnum].chan[chan].addressInPattern =
                m_chip[cnum].data[m_chip[cnum].header->patternsPointer + 2 * (i + chan)] +
                m_chip[cnum].data[m_chip[cnum].header->patternsPointer + 2 * (i + chan) + 1] * 0x100;

            m_chip[cnum].chan[chan].samplePointer = m_chip[cnum].header->samplesPointers[2] + m_chip[cnum].header->samplesPointers[3] * 0x100;
            m_chip[cnum].chan[chan].ornamentPointer = m_chip[cnum].header->ornamentsPointers[0] + m_chip[cnum].header->ornamentsPointers[1] * 0x100;
            m_chip[cnum].chan[chan].loopOrnamentPosition = m_chip[cnum].data[m_chip[cnum].chan[chan].ornamentPointer++];
            m_chip[cnum].chan[chan].ornamentLength = m_chip[cnum].data[m_chip[cnum].chan[chan].ornamentPointer++];
            m_chip[cnum].chan[chan].loopSamplePosition = m_chip[cnum].data[m_chip[cnum].chan[chan].samplePointer++];
            m_chip[cnum].chan[chan].sampleLength = m_chip[cnum].data[m_chip[cnum].chan[chan].samplePointer++];
            m_chip[cnum].chan[chan].volume = 15;
            m_chip[cnum].chan[chan].noteSkipCounter = 1;
        }
    }
    return true;
}

bool DecodePT3::Step()
{
    bool isNewLoop = GetRegisters(0);
    if (m_ts) GetRegisters(1);

    if (m_loop == 0)
    {
        uint8_t currPosition = m_chip[0].glob.currentPosition;
        uint8_t loopPosition = m_chip[0].header->loopPosition;
        uint8_t lastPosition = m_chip[0].header->numberOfPositions - 1;

        // detect true loop frame (ommit loop to first or last position)
        if (loopPosition > 0 && loopPosition < lastPosition && currPosition == loopPosition)
        {
            m_loop = m_tick;
        }
    }
    
    m_tick++;
    return isNewLoop;
}

int DecodePT3::GetNoteFreq(int chip, int note)
{
    switch (m_chip[chip].header->tonTableId) 
    {
    case  0: return (m_ver <= 3) ? PT3NoteTable_PT_33_34r[note] : PT3NoteTable_PT_34_35[note];
    case  1: return PT3NoteTable_ST[note];
    case  2: return (m_ver <= 3) ? PT3NoteTable_ASM_34r[note] : PT3NoteTable_ASM_34_35[note];
    default: return (m_ver <= 3) ? PT3NoteTable_REAL_34r[note] : PT3NoteTable_REAL_34_35[note];
    }
}

bool DecodePT3::GetRegisters(int chip)
{
    bool isNewLoop = false;
    m_regs[chip][13] = 0xFF;

    if (!--m_chip[chip].glob.delayCounter) 
    {
        for (int ch = 0; ch < 3; ch++) 
        {
            if (!--m_chip[chip].chan[ch].noteSkipCounter) 
            {
                if (ch == 0 && m_chip[chip].data[m_chip[chip].chan[ch].addressInPattern] == 0) 
                {
                    if (++m_chip[chip].glob.currentPosition == m_chip[chip].header->numberOfPositions)
                    {
                        m_chip[chip].glob.currentPosition = m_chip[chip].header->loopPosition;
                        isNewLoop = true;
                    }

                    uint8_t i = m_chip[chip].header->positionList[m_chip[chip].glob.currentPosition];
                    int b = m_chip[chip].ts;
                    if (b != 0x20) i = (uint8_t)(3 * b - 3 - i);

                    for (int chan = 0; chan < 3; chan++) 
                    {
                        m_chip[chip].chan[chan].addressInPattern =
                            m_chip[chip].data[m_chip[chip].header->patternsPointer + 2 * (i + chan)] +
                            m_chip[chip].data[m_chip[chip].header->patternsPointer + 2 * (i + chan) + 1] * 0x100;
                    }
                    m_chip[chip].glob.noiseBase = 0;
                }
                PatternInterpreter(chip, m_chip[chip].chan[ch]);
            }
        }
        m_chip[chip].glob.delayCounter = m_chip[chip].glob.delay;
    }
    AddToEnv = 0;
    TempMixer = 0;

    for (int ch = 0; ch < 3; ch++)
    {
        ChangeRegisters(chip, m_chip[chip].chan[ch]);
    }

    m_regs[chip][7] = TempMixer;
    m_regs[chip][6] = (m_chip[chip].glob.noiseBase + m_chip[chip].glob.addToNoise) & 0x1F;

    for (int ch = 0; ch < 3; ch++) 
    {
        m_regs[chip][2 * ch + 0] = m_chip[chip].chan[ch].ton & 0xFF;
        m_regs[chip][2 * ch + 1] = (m_chip[chip].chan[ch].ton >> 8) & 0x0F;
        m_regs[chip][ch + 8] = m_chip[chip].chan[ch].amplitude;
    }

    uint16_t env = m_chip[chip].glob.envBaseHi * 0x100 + m_chip[chip].glob.envBaseLo + AddToEnv + m_chip[chip].glob.curEnvSlide;
    m_regs[chip][11] = env & 0xFF;
    m_regs[chip][12] = (env >> 8) & 0xFF;

    if (m_chip[chip].glob.curEnvDelay > 0) 
    {
        if (!--m_chip[chip].glob.curEnvDelay) 
        {
            m_chip[chip].glob.curEnvDelay = m_chip[chip].glob.envDelay;
            m_chip[chip].glob.curEnvSlide += m_chip[chip].glob.envSlideAdd;
        }
    }

    return isNewLoop;
}

void DecodePT3::PatternInterpreter(int cnum, Channel& chan)
{
    int PrNote = chan.note;
    int PrSliding = chan.currentTonSliding;
    uint8_t counter = 0;
    uint8_t f1 = 0, f2 = 0, f3 = 0, f4 = 0, f5 = 0, f8 = 0, f9 = 0;
    for (;;) {
        uint8_t cc = m_chip[cnum].data[chan.addressInPattern];
        if (0xF0 <= cc && cc <= 0xFF) {
            uint8_t c1 = cc - 0xF0;
            chan.ornamentPointer = m_chip[cnum].header->ornamentsPointers[2 * c1] + 0x100 * m_chip[cnum].header->ornamentsPointers[2 * c1 + 1];
            chan.loopOrnamentPosition = m_chip[cnum].data[chan.ornamentPointer++];
            chan.ornamentLength = m_chip[cnum].data[chan.ornamentPointer++];
            chan.addressInPattern++;
            uint8_t c2 = m_chip[cnum].data[chan.addressInPattern] / 2;
            chan.samplePointer = m_chip[cnum].header->samplesPointers[2 * c2] + 0x100 * m_chip[cnum].header->samplesPointers[2 * c2 + 1];
            chan.loopSamplePosition = m_chip[cnum].data[chan.samplePointer++];
            chan.sampleLength = m_chip[cnum].data[chan.samplePointer++];
            chan.envelopeEnabled = false;
            chan.positionInOrnament = 0;
        }
        else if (0xD1 <= cc && cc <= 0xEF) {
            uint8_t c2 = cc - 0xD0;
            chan.samplePointer = m_chip[cnum].header->samplesPointers[2 * c2] + 0x100 * m_chip[cnum].header->samplesPointers[2 * c2 + 1];
            chan.loopSamplePosition = m_chip[cnum].data[chan.samplePointer++];
            chan.sampleLength = m_chip[cnum].data[chan.samplePointer++];
        }
        else if (cc == 0xD0) {
            chan.addressInPattern++;
            break;
        }
        else if (0xC1 <= cc && cc <= 0xCF) {
            chan.volume = cc - 0xC0;
        }
        else if (cc == 0xC0) {
            chan.positionInSample = 0;
            chan.currentAmplitudeSliding = 0;
            chan.currentNoiseSliding = 0;
            chan.currentEnvelopeSliding = 0;
            chan.positionInOrnament = 0;
            chan.tonSlideCount = 0;
            chan.currentTonSliding = 0;
            chan.tonAccumulator = 0;
            chan.currentOnOff = 0;
            chan.enabled = false;
            chan.addressInPattern++;
            break;
        }
        else if (0xB2 <= cc && cc <= 0xBF) {
            chan.envelopeEnabled = true;
            m_regs[cnum][13] = cc - 0xB1;
            m_chip[cnum].glob.envBaseHi = m_chip[cnum].data[++chan.addressInPattern];
            m_chip[cnum].glob.envBaseLo = m_chip[cnum].data[++chan.addressInPattern];
            chan.positionInOrnament = 0;
            m_chip[cnum].glob.curEnvSlide = 0;
            m_chip[cnum].glob.curEnvDelay = 0;
        }
        else if (cc == 0xB1) {
            chan.numberOfNotesToSkip = m_chip[cnum].data[++chan.addressInPattern];
        }
        else if (cc == 0xB0) {
            chan.envelopeEnabled = false;
            chan.positionInOrnament = 0;
        }
        else if (0x50 <= cc && cc <= 0xAF) {
            chan.note = cc - 0x50;
            chan.positionInSample = 0;
            chan.currentAmplitudeSliding = 0;
            chan.currentNoiseSliding = 0;
            chan.currentEnvelopeSliding = 0;
            chan.positionInOrnament = 0;
            chan.tonSlideCount = 0;
            chan.currentTonSliding = 0;
            chan.tonAccumulator = 0;
            chan.currentOnOff = 0;
            chan.enabled = true;
            chan.addressInPattern++;
            break;
        }
        else if (0x40 <= cc && cc <= 0x4F) {
            uint8_t c1 = cc - 0x40;
            chan.ornamentPointer = m_chip[cnum].header->ornamentsPointers[2 * c1] + 0x100 * m_chip[cnum].header->ornamentsPointers[2 * c1 + 1];
            chan.loopOrnamentPosition = m_chip[cnum].data[chan.ornamentPointer++];
            chan.ornamentLength = m_chip[cnum].data[chan.ornamentPointer++];
            chan.positionInOrnament = 0;
        }
        else if (0x20 <= cc && cc <= 0x3F) {
            m_chip[cnum].glob.noiseBase = cc - 0x20;
        }
        else if (0x10 <= cc && cc <= 0x1F) {
            chan.envelopeEnabled = (cc != 0x10);
            if (chan.envelopeEnabled) {
                m_regs[cnum][13] = cc - 0x10;
                m_chip[cnum].glob.envBaseHi = m_chip[cnum].data[++chan.addressInPattern];
                m_chip[cnum].glob.envBaseLo = m_chip[cnum].data[++chan.addressInPattern];
                m_chip[cnum].glob.curEnvSlide = 0;
                m_chip[cnum].glob.curEnvDelay = 0;
            }
            uint8_t c2 = m_chip[cnum].data[++chan.addressInPattern] / 2;
            chan.samplePointer = m_chip[cnum].header->samplesPointers[2 * c2] + 0x100 * m_chip[cnum].header->samplesPointers[2 * c2 + 1];
            chan.loopSamplePosition = m_chip[cnum].data[chan.samplePointer++];
            chan.sampleLength = m_chip[cnum].data[chan.samplePointer++];
            chan.positionInOrnament = 0;
        }
        else if (cc == 0x09) {
            f9 = ++counter;
        }
        else if (cc == 0x08) {
            f8 = ++counter;
        }
        else if (cc == 0x05) {
            f5 = ++counter;
        }
        else if (cc == 0x04) {
            f4 = ++counter;
        }
        else if (cc == 0x03) {
            f3 = ++counter;
        }
        else if (cc == 0x02) {
            f2 = ++counter;
        }
        else if (cc == 0x01) {
            f1 = ++counter;
        }

        chan.addressInPattern++;
    }

    while (counter > 0) {
        if (counter == f1) {
            chan.tonSlideDelay = m_chip[cnum].data[chan.addressInPattern++];
            chan.tonSlideCount = chan.tonSlideDelay;
            chan.tonSlideStep = (int16_t)(m_chip[cnum].data[chan.addressInPattern] + 0x100 * m_chip[cnum].data[chan.addressInPattern + 1]);
            chan.addressInPattern += 2;
            chan.simpleGliss = true;
            chan.currentOnOff = 0;
            if (chan.tonSlideCount == 0 && m_ver >= 7)
                chan.tonSlideCount++;
        }
        else if (counter == f2) {
            chan.simpleGliss = false;
            chan.currentOnOff = 0;
            chan.tonSlideDelay = m_chip[cnum].data[chan.addressInPattern];
            chan.tonSlideCount = chan.tonSlideDelay;
            chan.addressInPattern += 3;
            uint16_t step = m_chip[cnum].data[chan.addressInPattern] + 0x100 * m_chip[cnum].data[chan.addressInPattern + 1];
            chan.addressInPattern += 2;
            int16_t signed_step = step;
            chan.tonSlideStep = (signed_step < 0) ? -signed_step : signed_step;
            chan.tonDelta = GetNoteFreq(cnum, chan.note) - GetNoteFreq(cnum, PrNote);
            chan.slideToNote = chan.note;
            chan.note = PrNote;
            if (m_ver >= 6) chan.currentTonSliding = PrSliding;
            if (chan.tonDelta - chan.currentTonSliding < 0)
                chan.tonSlideStep = -chan.tonSlideStep;
        }
        else if (counter == f3) {
            chan.positionInSample = m_chip[cnum].data[chan.addressInPattern++];
        }
        else if (counter == f4) {
            chan.positionInOrnament = m_chip[cnum].data[chan.addressInPattern++];
        }
        else if (counter == f5) {
            chan.onOffDelay = m_chip[cnum].data[chan.addressInPattern++];
            chan.offOnDelay = m_chip[cnum].data[chan.addressInPattern++];
            chan.currentOnOff = chan.onOffDelay;
            chan.tonSlideCount = 0;
            chan.currentTonSliding = 0;
        }
        else if (counter == f8) {
            m_chip[cnum].glob.envDelay = m_chip[cnum].data[chan.addressInPattern++];
            m_chip[cnum].glob.curEnvDelay = m_chip[cnum].glob.envDelay;
            m_chip[cnum].glob.envSlideAdd = m_chip[cnum].data[chan.addressInPattern] + 0x100 * m_chip[cnum].data[chan.addressInPattern + 1];
            chan.addressInPattern += 2;
        }
        else if (counter == f9) {
            uint8_t b = m_chip[cnum].data[chan.addressInPattern++];
            m_chip[cnum].glob.delay = b;
            if (m_ts && m_chip[1].ts != 0x20) {
                m_chip[0].glob.delay = b;
                m_chip[0].glob.delayCounter = b;
                m_chip[1].glob.delay = b;
            }
        }
        counter--;
    }
    chan.noteSkipCounter = chan.numberOfNotesToSkip;
}

void DecodePT3::ChangeRegisters(int cnum, Channel& chan)
{
    if (chan.enabled) {
        uint16_t c1 = chan.samplePointer + chan.positionInSample * 4;
        uint8_t b0 = m_chip[cnum].data[c1], b1 = m_chip[cnum].data[c1 + 1];
        chan.ton = m_chip[cnum].data[c1 + 2] + 0x100 * m_chip[cnum].data[c1 + 3];
        chan.ton += chan.tonAccumulator;
        if (b1 & 0x40) chan.tonAccumulator = chan.ton;
        uint8_t j = chan.note + m_chip[cnum].data[chan.ornamentPointer + chan.positionInOrnament];
        if ((int8_t)j < 0) j = 0;
        else if (j > 95) j = 95;
        int w = GetNoteFreq(cnum, j);

        chan.ton = (chan.ton + chan.currentTonSliding + w) & 0xFFF;
        if (chan.tonSlideCount > 0) {
            if (!--chan.tonSlideCount) {
                chan.currentTonSliding += chan.tonSlideStep;
                chan.tonSlideCount = chan.tonSlideDelay;
                if (!chan.simpleGliss) {
                    if ((chan.tonSlideStep < 0 && chan.currentTonSliding <= chan.tonDelta) ||
                        (chan.tonSlideStep >= 0 && chan.currentTonSliding >= chan.tonDelta))
                    {
                        chan.note = chan.slideToNote;
                        chan.tonSlideCount = 0;
                        chan.currentTonSliding = 0;
                    }
                }
            }
        }
        chan.amplitude = b1 & 0x0F;
        if (b0 & 0x80) {
            if (b0 & 0x40) {
                if (chan.currentAmplitudeSliding < 15) {
                    chan.currentAmplitudeSliding++;
                }
            }
            else if (chan.currentAmplitudeSliding > -15) {
                chan.currentAmplitudeSliding--;
            }
        }
        chan.amplitude += chan.currentAmplitudeSliding;
        if ((int8_t)chan.amplitude < 0) chan.amplitude = 0;
        else if (chan.amplitude > 15) chan.amplitude = 15;
        if (m_ver <= 4) chan.amplitude = PT3VolumeTable_33_34[chan.volume][chan.amplitude];
        else chan.amplitude = PT3VolumeTable_35[chan.volume][chan.amplitude];

        if (!(b0 & 1) && chan.envelopeEnabled) chan.amplitude |= 0x10;
        if (b1 & 0x80) {
            uint8_t j = (b0 & 0x20)
                ? ((b0 >> 1) | 0xF0) + chan.currentEnvelopeSliding
                : ((b0 >> 1) & 0x0F) + chan.currentEnvelopeSliding;
            if (b1 & 0x20) chan.currentEnvelopeSliding = j;
            AddToEnv += j;
        }
        else {
            m_chip[cnum].glob.addToNoise = (b0 >> 1) + chan.currentNoiseSliding;
            if (b1 & 0x20) chan.currentNoiseSliding = m_chip[cnum].glob.addToNoise;
        }
        TempMixer |= (b1 >> 1) & 0x48;
        if (++chan.positionInSample >= chan.sampleLength)
            chan.positionInSample = chan.loopSamplePosition;
        if (++chan.positionInOrnament >= chan.ornamentLength)
            chan.positionInOrnament = chan.loopOrnamentPosition;
    }
    else {
        chan.amplitude = 0;
    }
    TempMixer >>= 1;
    if (chan.currentOnOff > 0) {
        if (!--chan.currentOnOff) {
            chan.enabled = !chan.enabled;
            chan.currentOnOff = chan.enabled ? chan.onOffDelay : chan.offOnDelay;
        }
    }
}
