#include <fstream>
#include "DecodeASC.h"
#include "module/Module.h"

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

bool DecodeASC::Init()
{
    Header* header = (Header*)m_data;
    
    memset(&ASC_A, 0, sizeof(Channel));
    memset(&ASC_B, 0, sizeof(Channel));
    memset(&ASC_C, 0, sizeof(Channel));

    CurrentPosition = 0;
    DelayCounter = 1;
    Delay = header->ASC1_Delay;
    
    uint16_t ascPatPt = header->ASC1_PatternsPointers;
    ASC_A.Address_In_Pattern = (*(uint16_t*)&m_data[ascPatPt + 6 * m_data[9] + 0]) + ascPatPt;
    ASC_B.Address_In_Pattern = (*(uint16_t*)&m_data[ascPatPt + 6 * m_data[9] + 2]) + ascPatPt;
    ASC_C.Address_In_Pattern = (*(uint16_t*)&m_data[ascPatPt + 6 * m_data[9] + 4]) + ascPatPt;

    memset(&m_regs, 0, sizeof(m_regs));
    return true;
}

void DecodeASC::PatternInterpreter(Channel& chan)
{
    Header* header = (Header*)m_data;

    int16_t delta_ton;
    bool Initialization_Of_Ornament_Disabled = false;
    bool Initialization_Of_Sample_Disabled = false;

    chan.Ton_Sliding_Counter = 0;
    chan.Amplitude_Delay_Counter = 0;

    while (true)
    {
        uint8_t val = m_data[chan.Address_In_Pattern];
        if (val <= 0x55)
        {
            chan.Note = val;
            chan.Address_In_Pattern++;
            chan.Current_Noise = chan.Initial_Noise;
            if ((int8_t)(chan.Ton_Sliding_Counter) <= 0)
                chan.Current_Ton_Sliding = 0;

            if (!Initialization_Of_Sample_Disabled)
            {
                chan.Addition_To_Amplitude = 0;
                chan.Ton_Deviation = 0;
                chan.Point_In_Sample = chan.Initial_Point_In_Sample;
                chan.Sound_Enabled = true;
                chan.Sample_Finished = false;
                chan.Break_Sample_Loop = false;
            }
            if (!Initialization_Of_Ornament_Disabled)
            {
                chan.Point_In_Ornament = chan.Initial_Point_In_Ornament;
                chan.Addition_To_Note = 0;
            }
            if (chan.Envelope_Enabled)
            {
                m_regs[Env_PeriodL] = m_data[chan.Address_In_Pattern];
                chan.Address_In_Pattern++;
            }
            break;
        }
        else if ((val >= 0x56) && (val <= 0x5d))
        {
            chan.Address_In_Pattern++;
            break;
        }
        else if (val == 0x5e)
        {
            chan.Break_Sample_Loop = true;
            chan.Address_In_Pattern++;
            break;
        }
        else if (val == 0x5f)
        {
            chan.Sound_Enabled = false;
            chan.Address_In_Pattern++;
            break;
        }
        else if ((val >= 0x60) && (val <= 0x9f))
        {
            chan.Number_Of_Notes_To_Skip = val - 0x60;
        }
        else if ((val >= 0xa0) && (val <= 0xbf))
        {
            chan.Initial_Point_In_Sample = (*(uint16_t *)&m_data[(val - 0xa0) * 2 + header->ASC1_SamplesPointers]) + header->ASC1_SamplesPointers;
        }
        else if ((val >= 0xc0) && (val <= 0xdf))
        {
            chan.Initial_Point_In_Ornament = (*(uint16_t *)&m_data[(val - 0xc0) * 2 + header->ASC1_OrnamentsPointers]) + header->ASC1_OrnamentsPointers;
        }
        else if (val == 0xe0)
        {
            chan.Volume = 15;
            chan.Envelope_Enabled = true;
        }
        else if ((val >= 0xe1) && (val <= 0xef))
        {
            chan.Volume = val - 0xe0;
            chan.Envelope_Enabled = false;
        }
        else if (val == 0xf0)
        {
            chan.Address_In_Pattern++;
            chan.Initial_Noise = m_data[chan.Address_In_Pattern];
        }
        else if (val == 0xf1)
        {
            Initialization_Of_Sample_Disabled = true;
        }
        else if (val == 0xf2)
        {
            Initialization_Of_Ornament_Disabled = true;
        }
        else if (val == 0xf3)
        {
            Initialization_Of_Sample_Disabled = true;
            Initialization_Of_Ornament_Disabled = true;
        }
        else if (val == 0xf4)
        {
            chan.Address_In_Pattern++;
            Delay = m_data[chan.Address_In_Pattern];
        }
        else if (val == 0xf5)
        {
            chan.Address_In_Pattern++;
            chan.Substruction_for_Ton_Sliding = -(int8_t)(m_data[chan.Address_In_Pattern]) * 16;
            chan.Ton_Sliding_Counter = 255;
        }
        else if (val == 0xf6)
        {
            chan.Address_In_Pattern++;
            chan.Substruction_for_Ton_Sliding = (int8_t)(m_data[chan.Address_In_Pattern]) * 16;
            chan.Ton_Sliding_Counter = 255;
        }
        else if (val == 0xf7)
        {
            chan.Address_In_Pattern++;
            Initialization_Of_Sample_Disabled = true;
            if (m_data[chan.Address_In_Pattern + 1] < 0x56)
                delta_ton = ASCNoteTable[chan.Note] + (chan.Current_Ton_Sliding / 16) - ASCNoteTable[m_data[chan.Address_In_Pattern + 1]];
            else
                delta_ton = chan.Current_Ton_Sliding / 16;
            delta_ton = delta_ton << 4;
            chan.Substruction_for_Ton_Sliding = -delta_ton / (int8_t)(m_data[chan.Address_In_Pattern]);
            chan.Current_Ton_Sliding = delta_ton - delta_ton % (int8_t)(m_data[chan.Address_In_Pattern]);
            chan.Ton_Sliding_Counter = (int8_t)(m_data[chan.Address_In_Pattern]);
        }
        else if (val == 0xf8)
        {
            m_regs[Env_Shape] = 8;
        }
        else if (val == 0xf9)
        {
            chan.Address_In_Pattern++;
            if (m_data[chan.Address_In_Pattern + 1] < 0x56)
            {
                delta_ton = ASCNoteTable[chan.Note] - ASCNoteTable[m_data[chan.Address_In_Pattern + 1]];
            }
            else
                delta_ton = chan.Current_Ton_Sliding / 16;
            delta_ton = delta_ton << 4;
            chan.Substruction_for_Ton_Sliding = -delta_ton / (int8_t)(m_data[chan.Address_In_Pattern]);
            chan.Current_Ton_Sliding = delta_ton - delta_ton % (int8_t)(m_data[chan.Address_In_Pattern]);
            chan.Ton_Sliding_Counter = (int8_t)(m_data[chan.Address_In_Pattern]);

        }
        else if (val == 0xfa)
        {
            m_regs[Env_Shape] = 10;
        }
        else if (val == 0xfb)
        {
            chan.Address_In_Pattern++;
            if ((m_data[chan.Address_In_Pattern] & 32) == 0)
            {
                chan.Amplitude_Delay = m_data[chan.Address_In_Pattern] << 3;
                chan.Amplitude_Delay_Counter = chan.Amplitude_Delay;
            }
            else
            {
                chan.Amplitude_Delay = ((m_data[chan.Address_In_Pattern] << 3) ^ 0xf8) + 9;
                chan.Amplitude_Delay_Counter = chan.Amplitude_Delay;
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
        chan.Address_In_Pattern++;
    }
    chan.Note_Skip_Counter = chan.Number_Of_Notes_To_Skip;
}

void DecodeASC::GetRegisters(Channel& chan, uint8_t& mixer)
{
    bool Sample_Says_OK_for_Envelope;
    if (chan.Sample_Finished || !chan.Sound_Enabled)
        chan.Amplitude = 0;
    else
    {
        if (chan.Amplitude_Delay_Counter != 0)
        {
            if (chan.Amplitude_Delay_Counter >= 16)
            {
                chan.Amplitude_Delay_Counter -= 8;
                if (chan.Addition_To_Amplitude < -15)
                    chan.Addition_To_Amplitude++;
                else if (chan.Addition_To_Amplitude > 15)
                    chan.Addition_To_Amplitude--;
            }
            else
            {
                if ((chan.Amplitude_Delay_Counter & 1) != 0)
                {
                    if (chan.Addition_To_Amplitude > -15)
                        chan.Addition_To_Amplitude--;
                }
                else if (chan.Addition_To_Amplitude < 15)
                    chan.Addition_To_Amplitude++;
                chan.Amplitude_Delay_Counter = chan.Amplitude_Delay;
            }
        }
        if ((m_data[chan.Point_In_Sample] & 128) != 0)
            chan.Loop_Point_In_Sample = chan.Point_In_Sample;
        if ((m_data[chan.Point_In_Sample] & 96) == 32)
            chan.Sample_Finished = true;
        chan.Ton_Deviation += (int8_t)(m_data[chan.Point_In_Sample + 1]);
        mixer |= (m_data[chan.Point_In_Sample + 2] & 9) << 3;
        if ((m_data[chan.Point_In_Sample + 2] & 6) == 2)
            Sample_Says_OK_for_Envelope = true;
        else
            Sample_Says_OK_for_Envelope = false;
        if ((m_data[chan.Point_In_Sample + 2] & 6) == 4)
        {
            if (chan.Addition_To_Amplitude > -15)
                chan.Addition_To_Amplitude--;
        }
        if ((m_data[chan.Point_In_Sample + 2] & 6) == 6)
        {
            if (chan.Addition_To_Amplitude < 15)
                chan.Addition_To_Amplitude++;
        }
        chan.Amplitude = chan.Addition_To_Amplitude + (m_data[chan.Point_In_Sample + 2] >> 4);
        if ((int8_t)(chan.Amplitude) < 0)
            chan.Amplitude = 0;
        else if (chan.Amplitude > 15)
            chan.Amplitude = 15;
        chan.Amplitude = (chan.Amplitude * (chan.Volume + 1)) >> 4;
        if (Sample_Says_OK_for_Envelope && ((mixer & 64) != 0))
        {
            uint8_t data = m_regs[Env_PeriodL];
            data += ((int8_t)(m_data[chan.Point_In_Sample] << 3) / 8);
            m_regs[Env_PeriodL] = data;
        }
        else
            chan.Current_Noise += (int8_t)(m_data[chan.Point_In_Sample] << 3) / 8;
        chan.Point_In_Sample += 3;
        if ((m_data[chan.Point_In_Sample - 3] & 64) != 0)
        {
            if (!chan.Break_Sample_Loop)
                chan.Point_In_Sample = chan.Loop_Point_In_Sample;
            else if ((m_data[chan.Point_In_Sample - 3] & 32) != 0)
                chan.Sample_Finished = true;
        }
        if ((m_data[chan.Point_In_Ornament] & 128) != 0)
            chan.Loop_Point_In_Ornament = chan.Point_In_Ornament;
        chan.Addition_To_Note += m_data[chan.Point_In_Ornament + 1];
        chan.Current_Noise += (-(int8_t)(m_data[chan.Point_In_Ornament] & 0x10)) | m_data[chan.Point_In_Ornament];
        chan.Point_In_Ornament += 2;
        if ((m_data[chan.Point_In_Ornament - 2] & 64) != 0)
            chan.Point_In_Ornament = chan.Loop_Point_In_Ornament;
        if ((mixer & 64) == 0)
        {
            uint8_t data = ((uint8_t)(chan.Current_Ton_Sliding >> 8) + chan.Current_Noise) & 0x1f;
            m_regs[Noise_Period] = data;
        }

        int8_t note = chan.Note + chan.Addition_To_Note;
        if (note < 0) note = 0;
        else if (note > 0x55) note = 0x55;

        chan.Ton = (ASCNoteTable[note] + chan.Ton_Deviation + (chan.Current_Ton_Sliding / 16)) & 0xfff;
        if (chan.Ton_Sliding_Counter != 0)
        {
            if ((int8_t)(chan.Ton_Sliding_Counter) > 0)
                chan.Ton_Sliding_Counter--;
            chan.Current_Ton_Sliding += chan.Substruction_for_Ton_Sliding;
        }
        if (chan.Envelope_Enabled && Sample_Says_OK_for_Envelope)
            chan.Amplitude |= 0x10;
    }
    mixer = mixer >> 1;
}

bool DecodeASC::Play()
{
    Header* header = (Header*)m_data;

    if (--DelayCounter <= 0)
    {
        if (--ASC_A.Note_Skip_Counter < 0)
        {
            if (m_data[ASC_A.Address_In_Pattern] == 255)
            {
                if (++CurrentPosition >= header->ASC1_Number_Of_Positions)
                {
                    CurrentPosition = header->ASC1_LoopingPosition;
                }

                uint16_t ascPatPt = header->ASC1_PatternsPointers;
                ASC_A.Address_In_Pattern = (*(uint16_t *)&m_data[ascPatPt + 6 * m_data[CurrentPosition + 9] + 0]) + ascPatPt;
                ASC_B.Address_In_Pattern = (*(uint16_t *)&m_data[ascPatPt + 6 * m_data[CurrentPosition + 9] + 2]) + ascPatPt;
                ASC_C.Address_In_Pattern = (*(uint16_t *)&m_data[ascPatPt + 6 * m_data[CurrentPosition + 9] + 4]) + ascPatPt;

                ASC_A.Initial_Noise = 0;
                ASC_B.Initial_Noise = 0;
                ASC_C.Initial_Noise = 0;
            }
            PatternInterpreter(ASC_A);
        }
        if (--ASC_B.Note_Skip_Counter < 0)
        {
            PatternInterpreter(ASC_B);
        }
        if (--ASC_C.Note_Skip_Counter < 0)
        {
            PatternInterpreter(ASC_C);
        }
        DelayCounter = Delay;
    }

    uint8_t mixer = 0;
    GetRegisters(ASC_A, mixer);
    GetRegisters(ASC_B, mixer);
    GetRegisters(ASC_C, mixer);

    m_regs[Mixer_Flags] = mixer;
    m_regs[TonA_PeriodL] = ASC_A.Ton & 0xff;
    m_regs[TonA_PeriodH] = (ASC_A.Ton >> 8) & 0xf;
    m_regs[TonB_PeriodL] = ASC_B.Ton & 0xff;
    m_regs[TonB_PeriodH] = (ASC_B.Ton >> 8) & 0xf;
    m_regs[TonC_PeriodL] = ASC_C.Ton & 0xff;
    m_regs[TonC_PeriodH] = (ASC_C.Ton >> 8) & 0xf;
    m_regs[VolA_EnvFlg] = ASC_A.Amplitude;
    m_regs[VolB_EnvFlg] = ASC_B.Amplitude;
    m_regs[VolC_EnvFlg] = ASC_C.Amplitude;
}