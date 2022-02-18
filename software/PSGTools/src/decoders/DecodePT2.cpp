#include <fstream>
#include "DecodePT2.h"
#include "module/Module.h"

bool DecodePT2::Open(Module& module)
{
	return false;
}

bool DecodePT2::Decode(Frame& frame)
{
	return false;
}

void DecodePT2::Close(Module& module)
{
    //
}

////////////////////////////////////////////////////////////////////////////////

const uint16_t PT2_Table[96] =
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


enum
{
    AY_CHNL_A_FINE = 0,
    AY_CHNL_A_COARSE,
    AY_CHNL_B_FINE,
    AY_CHNL_B_COARSE,
    AY_CHNL_C_FINE,
    AY_CHNL_C_COARSE,
    AY_NOISE_PERIOD,
    AY_MIXER,
    AY_CHNL_A_VOL,
    AY_CHNL_B_VOL,
    AY_CHNL_C_VOL,
    AY_ENV_FINE,
    AY_ENV_COARSE,
    AY_ENV_SHAPE,
    AY_GPIO_A,
    AY_GPIO_B
};

uint16_t ay_sys_getword(uint8_t* p)
{
    return *p | ((*(p + 1)) << 8);
}

#define PT2_A ((PT2_SongInfo *)info.data)->PT2_A
#define PT2_B ((PT2_SongInfo *)info.data)->PT2_B
#define PT2_C ((PT2_SongInfo *)info.data)->PT2_C
#define PT2 ((PT2_SongInfo *)info.data)->PT2

#define PT2_SamplesPointers(x) (header->PT2_SamplesPointers0 [(x) * 2] | (header->PT2_SamplesPointers0 [(x) * 2 + 1] << 8))
#define PT2_OrnamentsPointers(x) (header->PT2_OrnamentsPointers0 [(x) * 2] | (header->PT2_OrnamentsPointers0 [(x) * 2 + 1] << 8))
#define PT2_PositionList(x) (header->PT2_PositionList [(x)])
#define PT2_PatternsPointer (header->PT2_PatternsPointer0 | (header->PT2_PatternsPointer1 << 8))
#define PT2_Delay (header->PT2_Delay)
#define PT2_NumberOfPositions (header->PT2_NumberOfPositions)
#define PT2_LoopPosition (header->PT2_LoopPosition)

bool DecodePT2::Init(AYSongInfo& info)
{
    uint8_t* module = info.module;
    PT2_File* header = (PT2_File*)module;
    if (info.data)
    {
        delete (PT2_SongInfo*)info.data;
        info.data = 0;
    }
    info.data = (void*)new PT2_SongInfo;
    if (!info.data)
        return false;

    memset(&PT2_A, 0, sizeof(PT2_Channel_Parameters));
    memset(&PT2_B, 0, sizeof(PT2_Channel_Parameters));
    memset(&PT2_C, 0, sizeof(PT2_Channel_Parameters));
    PT2.DelayCounter = 1;
    PT2.Delay = PT2_Delay;
    PT2.CurrentPosition = 0;

    PT2_A.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(0) * 6]);
    PT2_B.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(0) * 6 + 2]);
    PT2_C.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(0) * 6 + 4]);

    PT2_A.OrnamentPointer = PT2_OrnamentsPointers(0);
    PT2_A.Ornament_Length = module[PT2_A.OrnamentPointer];
    PT2_A.OrnamentPointer++;
    PT2_A.Loop_Ornament_Position = module[PT2_A.OrnamentPointer];
    PT2_A.OrnamentPointer++;
    PT2_A.Envelope_Enabled = false;
    PT2_A.Position_In_Sample = 0;
    PT2_A.Position_In_Ornament = 0;
    PT2_A.Addition_To_Noise = 0;
    PT2_A.Glissade = 0;
    PT2_A.Current_Ton_Sliding = 0;
    PT2_A.GlissType = 0;
    PT2_A.Enabled = false;
    PT2_A.Number_Of_Notes_To_Skip = 0;
    PT2_A.Note_Skip_Counter = 0;
    PT2_A.Volume = 15;
    PT2_A.Ton = 0;

    PT2_B.OrnamentPointer = PT2_A.OrnamentPointer;
    PT2_B.Loop_Ornament_Position = PT2_A.Loop_Ornament_Position;
    PT2_B.Ornament_Length = PT2_A.Ornament_Length;
    PT2_B.Envelope_Enabled = false;
    PT2_B.Position_In_Sample = 0;
    PT2_B.Position_In_Ornament = 0;
    PT2_B.Addition_To_Noise = 0;
    PT2_B.Glissade = 0;
    PT2_B.Current_Ton_Sliding = 0;
    PT2_B.GlissType = 0;
    PT2_B.Enabled = false;
    PT2_B.Number_Of_Notes_To_Skip = 0;
    PT2_B.Note_Skip_Counter = 0;
    PT2_B.Volume = 15;
    PT2_B.Ton = 0;

    PT2_C.OrnamentPointer = PT2_A.OrnamentPointer;
    PT2_C.Loop_Ornament_Position = PT2_A.Loop_Ornament_Position;
    PT2_C.Ornament_Length = PT2_A.Ornament_Length;
    PT2_C.Envelope_Enabled = false;
    PT2_C.Position_In_Sample = 0;
    PT2_C.Position_In_Ornament = 0;
    PT2_C.Addition_To_Noise = 0;
    PT2_C.Glissade = 0;
    PT2_C.Current_Ton_Sliding = 0;
    PT2_C.GlissType = 0;
    PT2_C.Enabled = false;
    PT2_C.Number_Of_Notes_To_Skip = 0;
    PT2_C.Note_Skip_Counter = 0;
    PT2_C.Volume = 15;
    PT2_C.Ton = 0;

    memset(&regs, 0, sizeof(regs));
	return true;
}

void DecodePT2::PT2_PatternInterpreter(AYSongInfo& info, PT2_Channel_Parameters& chan)
{
    uint8_t* module = info.module;
    PT2_File* header = (PT2_File*)module;
    bool quit, gliss;
    quit = false;
    gliss = false;
    do
    {
        uint8_t val = module[chan.Address_In_Pattern];
        if (val >= 0xe1)
        {
            chan.SamplePointer = PT2_SamplesPointers(val - 0xe0);
            chan.Sample_Length = module[chan.SamplePointer];
            chan.SamplePointer++;
            chan.Loop_Sample_Position = module[chan.SamplePointer];
            chan.SamplePointer++;
        }
        else if (val == 0xe0)
        {
            chan.Position_In_Sample = 0;
            chan.Position_In_Ornament = 0;
            chan.Current_Ton_Sliding = 0;
            chan.GlissType = 0;
            chan.Enabled = false;
            quit = true;
        }
        else if (val >= 0x80 && val <= 0xdf)
        {
            chan.Position_In_Sample = 0;
            chan.Position_In_Ornament = 0;
            chan.Current_Ton_Sliding = 0;
            if (gliss)
            {
                chan.Slide_To_Note = val - 0x80;
                if (chan.GlissType == 1)
                    chan.Note = chan.Slide_To_Note;
            }
            else
            {
                chan.Note = val - 0x80;
                chan.GlissType = 0;
            }
            chan.Enabled = true;
            quit = true;
        }
        else if (val == 0x7f)
            chan.Envelope_Enabled = false;
        else if (val >= 0x71 && val <= 0x7e)
        {
            chan.Envelope_Enabled = true;
            regs[AY_ENV_SHAPE] = val - 0x70;
            chan.Address_In_Pattern++;
            regs[AY_ENV_FINE] = module[chan.Address_In_Pattern];
            chan.Address_In_Pattern++;
            regs[AY_ENV_COARSE] = module[chan.Address_In_Pattern];
        }
        else if (val == 0x70)
            quit = true;
        else if (val >= 0x60 && val <= 0x6f)
        {
            chan.OrnamentPointer = PT2_OrnamentsPointers(val - 0x60);
            chan.Ornament_Length = module[chan.OrnamentPointer];
            chan.OrnamentPointer++;
            chan.Loop_Ornament_Position = module[chan.OrnamentPointer];
            chan.OrnamentPointer++;
            chan.Position_In_Ornament = 0;
        }
        else if (val >= 0x20 && val <= 0x5f)
            chan.Number_Of_Notes_To_Skip = val - 0x20;
        else if (val >= 0x10 && val <= 0x1f)
            chan.Volume = val - 0x10;
        else if (val == 0xf)
        {
            chan.Address_In_Pattern++;
            PT2.Delay = module[chan.Address_In_Pattern];
        }
        else if (val == 0xe)
        {
            chan.Address_In_Pattern++;
            chan.Glissade = module[chan.Address_In_Pattern];
            chan.GlissType = 1;
            gliss = true;
        }
        else if (val == 0xd)
        {
            chan.Address_In_Pattern++;
            chan.Glissade = abs((int8_t)(module[chan.Address_In_Pattern]));
            chan.Address_In_Pattern += 2; //Not use precalculated Ton_Delta
            //to avoide error with first note of pattern
            chan.GlissType = 2;
            gliss = true;
        }
        else if (val == 0xc)
            chan.GlissType = 0;
        else
        {
            chan.Address_In_Pattern++;
            chan.Addition_To_Noise = module[chan.Address_In_Pattern];
        }
        chan.Address_In_Pattern++;
    } while (!quit);
    if (gliss && (chan.GlissType == 2))
    {
        chan.Ton_Delta = abs(PT2_Table[chan.Slide_To_Note] - PT2_Table[chan.Note]);
        if (chan.Slide_To_Note > chan.Note)
            chan.Glissade = -chan.Glissade;
    }
    chan.Note_Skip_Counter = chan.Number_Of_Notes_To_Skip;
}

void DecodePT2::PT2_GetRegisters(AYSongInfo& info, PT2_Channel_Parameters& chan, uint8_t& TempMixer)
{
    uint8_t j, b0, b1;
    uint8_t* module = info.module;
    PT2_File* header = (PT2_File*)module;
    if (chan.Enabled)
    {
        b0 = module[chan.SamplePointer + chan.Position_In_Sample * 3];
        b1 = module[chan.SamplePointer + chan.Position_In_Sample * 3 + 1];
        chan.Ton = module[chan.SamplePointer + chan.Position_In_Sample * 3 + 2] + (uint16_t)((b1 & 15) << 8);
        if ((b0 & 4) == 0)
            chan.Ton = -chan.Ton;
        j = chan.Note + module[chan.OrnamentPointer + chan.Position_In_Ornament];
        if (j > 95)
            j = 95;
        chan.Ton = (chan.Ton + chan.Current_Ton_Sliding + PT2_Table[j]) & 0xfff;
        if (chan.GlissType == 2)
        {
            chan.Ton_Delta = chan.Ton_Delta - abs(chan.Glissade);
            if (chan.Ton_Delta < 0)
            {
                chan.Note = chan.Slide_To_Note;
                chan.GlissType = 0;
                chan.Current_Ton_Sliding = 0;
            }
        }
        if (chan.GlissType != 0)
            chan.Current_Ton_Sliding += chan.Glissade;
        chan.Amplitude = (chan.Volume * 17 + (uint8_t)(chan.Volume > 7)) * (b1 >> 4) / 256;
        if (chan.Envelope_Enabled)
            chan.Amplitude = chan.Amplitude | 16;
        if ((module[chan.SamplePointer + chan.Position_In_Sample * 3] & 1) != 0)
            TempMixer = TempMixer | 64;
        else
            regs[AY_NOISE_PERIOD] = ((b0 >> 3) + chan.Addition_To_Noise) & 31;
        if ((b0 & 2) != 0)
            TempMixer = TempMixer | 8;
        chan.Position_In_Sample++;
        if (chan.Position_In_Sample == chan.Sample_Length)
            chan.Position_In_Sample = chan.Loop_Sample_Position;
        chan.Position_In_Ornament++;
        if (chan.Position_In_Ornament == chan.Ornament_Length)
            chan.Position_In_Ornament = chan.Loop_Ornament_Position;
    }
    else
        chan.Amplitude = 0;
    TempMixer = TempMixer >> 1;
}

void DecodePT2::PT2_Play(AYSongInfo& info)
{
    uint8_t TempMixer;
    uint8_t* module = info.module;
    PT2_File* header = (PT2_File*)module;
    PT2.DelayCounter--;
    if (PT2.DelayCounter == 0)
    {
        PT2_A.Note_Skip_Counter--;
        if (PT2_A.Note_Skip_Counter < 0)
        {
            if (module[PT2_A.Address_In_Pattern] == 0)
            {
                PT2.CurrentPosition++;
                if (PT2.CurrentPosition == PT2_NumberOfPositions)
                    PT2.CurrentPosition = PT2_LoopPosition;
                PT2_A.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(PT2.CurrentPosition) * 6]);
                PT2_B.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(PT2.CurrentPosition) * 6 + 2]);
                PT2_C.Address_In_Pattern = ay_sys_getword(&module[PT2_PatternsPointer + PT2_PositionList(PT2.CurrentPosition) * 6 + 4]);
            }
            PT2_PatternInterpreter(info, PT2_A);
        }
        PT2_B.Note_Skip_Counter--;
        if (PT2_B.Note_Skip_Counter < 0)
            PT2_PatternInterpreter(info, PT2_B);
        PT2_C.Note_Skip_Counter--;
        if (PT2_C.Note_Skip_Counter < 0)
            PT2_PatternInterpreter(info, PT2_C);
        PT2.DelayCounter = PT2.Delay;
    }

    TempMixer = 0;
    PT2_GetRegisters(info, PT2_A, TempMixer);
    PT2_GetRegisters(info, PT2_B, TempMixer);
    PT2_GetRegisters(info, PT2_C, TempMixer);

    regs[AY_MIXER] = TempMixer;

    regs[AY_CHNL_A_FINE] = PT2_A.Ton & 0xff;
    regs[AY_CHNL_A_COARSE] = (PT2_A.Ton >> 8) & 0xf;
    regs[AY_CHNL_B_FINE] = PT2_B.Ton & 0xff;
    regs[AY_CHNL_B_COARSE] = (PT2_B.Ton >> 8) & 0xf;
    regs[AY_CHNL_C_FINE] = PT2_C.Ton & 0xff;
    regs[AY_CHNL_C_COARSE] = (PT2_C.Ton >> 8) & 0xf;
    regs[AY_CHNL_A_VOL] = PT2_A.Amplitude;
    regs[AY_CHNL_B_VOL] = PT2_B.Amplitude;
    regs[AY_CHNL_C_VOL] = PT2_C.Amplitude;

}

void DecodePT2::PT2_Cleanup(AYSongInfo& info)
{
    if (info.data)
    {
        delete (PT2_SongInfo*)info.data;
        info.data = 0;
    }
}
