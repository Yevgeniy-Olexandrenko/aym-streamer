#include <fstream>
#include "DecodeSTC.h"
#include "module/Module.h"

namespace
{
	const unsigned short ST_Table[96] =
	{ 
		0xef8, 0xe10, 0xd60, 0xc80, 0xbd8, 0xb28, 0xa88, 0x9f0,
		0x960, 0x8e0, 0x858, 0x7e0, 0x77c, 0x708, 0x6b0, 0x640,
		0x5ec, 0x594, 0x544, 0x4f8, 0x4b0, 0x470, 0x42c, 0x3f0,
		0x3be, 0x384, 0x358, 0x320, 0x2f6, 0x2ca, 0x2a2, 0x27c,
		0x258, 0x238, 0x216, 0x1f8, 0x1df, 0x1c2, 0x1ac, 0x190,
		0x17b, 0x165, 0x151, 0x13e, 0x12c, 0x11c, 0x10b, 0x0fc,
		0x0ef, 0x0e1, 0x0d6, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f,
		0x096, 0x08e, 0x085, 0x07e, 0x077, 0x070, 0x06b, 0x064,
		0x05e, 0x059, 0x054, 0x04f, 0x04b, 0x047, 0x042, 0x03f,
		0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02c, 0x02a, 0x027,
		0x025, 0x023, 0x021, 0x01f, 0x01d, 0x01c, 0x01a, 0x019,
		0x017, 0x016, 0x015, 0x013, 0x012, 0x011, 0x010, 0x00f
	};
}

bool DecodeSTC::STC_Init()
{
    STC_File* header = (STC_File*)m_data;

    memset(&STC_A, 0, sizeof(STC_Channel_Parameters));
    memset(&STC_B, 0, sizeof(STC_Channel_Parameters));
    memset(&STC_C, 0, sizeof(STC_Channel_Parameters));

    uint16_t ST_PositionsPointer = (header->ST_PositionsPointer0 | header->ST_PositionsPointer1 << 8);
    uint16_t ST_OrnamentsPointer = (header->ST_OrnamentsPointer0 | header->ST_OrnamentsPointer1 << 8);
    uint16_t ST_PatternsPointer = (header->ST_PatternsPointer0 | header->ST_PatternsPointer1 << 8);

    STC.CurrentPosition = 0;
    STC.Transposition = m_data[ST_PositionsPointer + 2];
    STC.DelayCounter = 1;

    unsigned long i = 0;
    while (m_data[ST_PatternsPointer + 7 * i] != m_data[ST_PositionsPointer + 1]) i++;
    
    ST_PatternsPointer += 7 * i;
    STC_A.Address_In_Pattern = m_data[ST_PatternsPointer + 1] | m_data[ST_PatternsPointer + 2] << 8;
    STC_B.Address_In_Pattern = m_data[ST_PatternsPointer + 3] | m_data[ST_PatternsPointer + 4] << 8;
    STC_C.Address_In_Pattern = m_data[ST_PatternsPointer + 5] | m_data[ST_PatternsPointer + 6] << 8;

    for (STC_Channel_Parameters* chan : { &STC_C, &STC_C, &STC_C })
    {
        chan->Sample_Tik_Counter = -1;
        chan->OrnamentPointer = ST_OrnamentsPointer + 1;
    }

    memset(&m_regs, 0, sizeof(m_regs));
    return true;
}

bool DecodeSTC::Step()
{
    //
}

void DecodeSTC::STC_PatternInterpreter(STC_Channel_Parameters& chan)
{
    STC_File* header = (STC_File*)m_data;

    uint16_t ST_PositionsPointer = (header->ST_PositionsPointer0 | header->ST_PositionsPointer1 << 8);
    uint16_t ST_OrnamentsPointer = (header->ST_OrnamentsPointer0 | header->ST_OrnamentsPointer1 << 8);
    uint16_t ST_PatternsPointer = (header->ST_PatternsPointer0 | header->ST_PatternsPointer1 << 8);


    unsigned short k;
    while (true)
    {
        unsigned char val = m_data[chan.Address_In_Pattern];
        if (val <= 0x5f)
        {
            chan.Note = val;
            chan.Sample_Tik_Counter = 32;
            chan.Position_In_Sample = 0;
            chan.Address_In_Pattern++;
            break;
        }
        else if (val >= 0x60 && val <= 0x6f)
        {
            k = 0;
            while (m_data[0x1b + 0x63 * k] != (val - 0x60))
                k++;
            chan.SamplePointer = 0x1c + 0x63 * k;
        }
        else if (val >= 0x70 && val <= 0x7f)
        {
            k = 0;
            while (m_data[ST_OrnamentsPointer + 0x21 * k] != (val - 0x70))
                k++;
            chan.OrnamentPointer = ST_OrnamentsPointer + 0x21 * k + 1;
            chan.Envelope_Enabled = false;
        }
        else if (val == 0x80)
        {
            chan.Sample_Tik_Counter = -1;
            chan.Address_In_Pattern++;
            break;
        }
        else if (val == 0x81)
        {
            chan.Address_In_Pattern++;
            break;
        }
        else if (val == 0x82)
        {
            k = 0;
            while (m_data[ST_OrnamentsPointer + 0x21 * k] != 0)
                k++;
            chan.OrnamentPointer = ST_OrnamentsPointer + 0x21 * k + 1;
            chan.Envelope_Enabled = false;
        }
        else if (val >= 0x83 && val <= 0x8e)
        {
            m_regs[Env_Shape] = val - 0x80;
            chan.Address_In_Pattern++;

            m_regs[Env_PeriodL] = m_data[chan.Address_In_Pattern];
            chan.Envelope_Enabled = true;

            k = 0;
            while (m_data[ST_OrnamentsPointer + 0x21 * k] != 0) k++;
            chan.OrnamentPointer = ST_OrnamentsPointer + 0x21 * k + 1;
        }
        else
        {
            chan.Number_Of_Notes_To_Skip = val - 0xa1;
        }
        chan.Address_In_Pattern++;
    }
    chan.Note_Skip_Counter = chan.Number_Of_Notes_To_Skip;
}

void DecodeSTC::STC_GetRegisters(STC_Channel_Parameters& chan, unsigned char& TempMixer)
{
    STC_File* header = (STC_File*)m_data;

    unsigned short i;
    unsigned char j;

    if (chan.Sample_Tik_Counter >= 0)
    {
        chan.Sample_Tik_Counter--;
        chan.Position_In_Sample = (chan.Position_In_Sample + 1) & 0x1f;
        if (chan.Sample_Tik_Counter == 0)
        {
            if (m_data[chan.SamplePointer + 0x60] != 0)
            {
                chan.Position_In_Sample = m_data[chan.SamplePointer + 0x60] & 0x1f;
                chan.Sample_Tik_Counter = m_data[chan.SamplePointer + 0x61] + 1;
            }
            else
                chan.Sample_Tik_Counter = -1;
        }
    }
    if (chan.Sample_Tik_Counter >= 0)
    {
        i = ((chan.Position_In_Sample - 1) & 0x1f) * 3 + chan.SamplePointer;
        if ((m_data[i + 1] & 0x80) != 0)
            TempMixer = TempMixer | 64;
        else
            m_regs[Noise_Period] = m_data[i + 1] & 0x1f;

        if ((m_data[i + 1] & 0x40) != 0)
            TempMixer = TempMixer | 8;
        chan.Amplitude = m_data[i] & 15;
        j = chan.Note + m_data[chan.OrnamentPointer + ((chan.Position_In_Sample - 1) & 0x1f)] + STC.Transposition;
        if (j > 95)
            j = 95;
        if ((m_data[i + 1] & 0x20) != 0)
            chan.Ton = (ST_Table[j] + m_data[i + 2] + (((unsigned short)(m_data[i] & 0xf0)) << 4)) & 0xFFF;
        else
            chan.Ton = (ST_Table[j] - m_data[i + 2] - (((unsigned short)(m_data[i] & 0xf0)) << 4)) & 0xFFF;
        if (chan.Envelope_Enabled)
            chan.Amplitude = chan.Amplitude | 16;
    }
    else
        chan.Amplitude = 0;

    TempMixer = TempMixer >> 1;
}

bool DecodeSTC::STC_Play()
{
    STC_File* header = (STC_File*)m_data;

    uint16_t ST_PositionsPointer = (header->ST_PositionsPointer0 | header->ST_PositionsPointer1 << 8);
    uint16_t ST_OrnamentsPointer = (header->ST_OrnamentsPointer0 | header->ST_OrnamentsPointer1 << 8);
    uint16_t ST_PatternsPointer = (header->ST_PatternsPointer0 | header->ST_PatternsPointer1 << 8);

    bool isNewLoop = false;
    m_regs[Env_Shape] = 0xFF; // ???

    unsigned char TempMixer;
    unsigned short i;
    
    STC.DelayCounter--;
    if (STC.DelayCounter == 0)
    {
        STC.DelayCounter = header->ST_Delay;
        STC_A.Note_Skip_Counter--;
        if (STC_A.Note_Skip_Counter < 0)
        {
            if (m_data[STC_A.Address_In_Pattern] == 255)
            {
                if (STC.CurrentPosition == m_data[ST_PositionsPointer])
                {
                    STC.CurrentPosition = 0;
                    isNewLoop = true;
                }
                else
                    STC.CurrentPosition++;

                STC.Transposition = m_data[ST_PositionsPointer + 2 + STC.CurrentPosition * 2];

                i = 0;
                while (m_data[ST_PatternsPointer + 7 * i] != m_data[ST_PositionsPointer + 1 + STC.CurrentPosition * 2]) i++;

                ST_PatternsPointer += 7 * i;
                STC_A.Address_In_Pattern = m_data[ST_PatternsPointer + 1] | m_data[ST_PatternsPointer + 2] << 8;
                STC_B.Address_In_Pattern = m_data[ST_PatternsPointer + 3] | m_data[ST_PatternsPointer + 4] << 8;
                STC_C.Address_In_Pattern = m_data[ST_PatternsPointer + 5] | m_data[ST_PatternsPointer + 6] << 8;
            }
            STC_PatternInterpreter(STC_A);
        }

        STC_B.Note_Skip_Counter--;
        if (STC_B.Note_Skip_Counter < 0)
            STC_PatternInterpreter(STC_B);

        STC_C.Note_Skip_Counter--;
        if (STC_C.Note_Skip_Counter < 0)
            STC_PatternInterpreter(STC_C);
    }

    TempMixer = 0;
    STC_GetRegisters(STC_A, TempMixer);
    STC_GetRegisters(STC_B, TempMixer);
    STC_GetRegisters(STC_C, TempMixer);

    m_regs[Mixer_Flags] = TempMixer;
    m_regs[TonA_PeriodL] = STC_A.Ton & 0xff;
    m_regs[TonA_PeriodH] = (STC_A.Ton >> 8) & 0xf;
    m_regs[TonB_PeriodL] = STC_B.Ton & 0xff;
    m_regs[TonB_PeriodH] = (STC_B.Ton >> 8) & 0xf;
    m_regs[TonC_PeriodL] = STC_C.Ton & 0xff;
    m_regs[TonC_PeriodH] = (STC_C.Ton >> 8) & 0xf;
    m_regs[VolA_EnvFlg] = STC_A.Amplitude;
    m_regs[VolB_EnvFlg] = STC_B.Amplitude;
    m_regs[VolC_EnvFlg] = STC_C.Amplitude;
    return isNewLoop;
}
