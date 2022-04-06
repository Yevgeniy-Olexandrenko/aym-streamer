#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodeASC : public Decoder
{
#pragma pack(push, 1)
    struct ASC1_File
    {
        unsigned char ASC1_Delay, ASC1_LoopingPosition;
        unsigned short ASC1_PatternsPointers;
        unsigned short ASC1_SamplesPointers;
        unsigned short ASC1_OrnamentsPointers;
        unsigned char ASC1_Number_Of_Positions;
        unsigned char ASC1_Positions[65536 - 8];
    };
#pragma pack(pop)

    struct ASC_Channel_Parameters
    {
        unsigned short Initial_Point_In_Sample, Point_In_Sample, Loop_Point_In_Sample, Initial_Point_In_Ornament, Point_In_Ornament, Loop_Point_In_Ornament, Address_In_Pattern, Ton, Ton_Deviation;
        unsigned char Note, Addition_To_Note, Number_Of_Notes_To_Skip, Initial_Noise, Current_Noise, Volume, Ton_Sliding_Counter, Amplitude, Amplitude_Delay, Amplitude_Delay_Counter;
        short Current_Ton_Sliding, Substruction_for_Ton_Sliding;
        signed char Note_Skip_Counter, Addition_To_Amplitude;
        bool Envelope_Enabled, Sound_Enabled, Sample_Finished, Break_Sample_Loop, Break_Ornament_Loop;
    };

    struct ASC_Parameters
    {
        unsigned char Delay, DelayCounter, CurrentPosition;
    };

public:
    bool Open(Module& module) override;
    bool Decode(Frame& frame) override;
    void Close(Module& module) override;

private:
    bool Init();
    void PatternInterpreter(ASC_Channel_Parameters& chan);
    void GetRegisters(ASC_Channel_Parameters& chan, uint8_t& mixer);
    bool Play();

private:
    uint8_t* m_data;

    ASC_Parameters ASC;
    ASC_Channel_Parameters ASC_A;
    ASC_Channel_Parameters ASC_B;
    ASC_Channel_Parameters ASC_C;

    uint8_t m_regs[16];
};
