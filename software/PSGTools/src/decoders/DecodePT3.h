#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodePT3 : public Decoder
{
    #pragma pack(push, 1)
    struct PT3Header
    {
        char     MusicName[0x63];
        uint8_t  TonTableId;
        uint8_t  Delay;
        uint8_t  NumberOfPositions;
        uint8_t  LoopPosition;
        uint16_t PatternsPointer;
        uint8_t  SamplesPointers_w[32 * 2];   // WORD array
        uint8_t  OrnamentsPointers_w[16 * 2]; // WORD array
        uint8_t  PositionList[1];             // open array
    };
    #pragma pack(pop)

    struct PT3_Channel
    {
        unsigned Address_In_Pattern, OrnamentPointer, SamplePointer, Ton;
        int      Current_Amplitude_Sliding, Current_Noise_Sliding, Current_Envelope_Sliding,
                 Ton_Slide_Count, Current_OnOff, OnOff_Delay, OffOn_Delay,
                 Ton_Slide_Delay, Current_Ton_Sliding,
                 Ton_Accumulator, Ton_Slide_Step, Ton_Delta;
        int8_t   Note_Skip_Counter;
        uint8_t  Loop_Ornament_Position, Ornament_Length, Position_In_Ornament,
                 Loop_Sample_Position, Sample_Length, Position_In_Sample,
                 Volume, Number_Of_Notes_To_Skip, Note, Slide_To_Note, Amplitude;
        bool     Envelope_Enabled, Enabled, SimpleGliss;
    };

    struct PT3_Module
    {
        int      Cur_Env_Slide, Env_Slide_Add;
        uint8_t  Env_Base_lo, Env_Base_hi;
        uint8_t  Noise_Base, Delay, AddToNoise, DelayCounter, CurrentPosition;
        int8_t   Cur_Env_Delay, Env_Delay;
    };

    struct PlConst
    {
        int TS;
    };

public:
	bool Open   (Module& module) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Module& module) override;

public:
    bool Init();
    bool Step();

    int  GetNoteFreq(int cnum, int j);
    bool GetRegisters(int cnum);
    void PatternInterpreter(int cnum, PT3_Channel& chan);
    void ChangeRegisters(int cnum, PT3_Channel& chan);
    
private:
    uint8_t* body;
    uint32_t mod_size;

    unsigned loop;
    unsigned tick;

    uint8_t regs[2][16];
    uint8_t version;
    bool tsMode;

    struct {
        PT3Header*  header;
        uint8_t*    module;
        PT3_Module  mod;
        PT3_Channel ch[3];
        PlConst     plconst;
    } chip[2];

    int AddToEnv;
    uint8_t TempMixer;
};