#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodeASC : public Decoder
{
#pragma pack(push, 1)
    struct Header
    {
        uint8_t ASC1_Delay, ASC1_LoopingPosition;
        uint16_t ASC1_PatternsPointers;
        uint16_t ASC1_SamplesPointers;
        uint16_t ASC1_OrnamentsPointers;
        uint8_t ASC1_Number_Of_Positions;
        uint8_t ASC1_Positions[65536 - 8];
    };
#pragma pack(pop)

    struct Channel
    {
        uint16_t Initial_Point_In_Sample;
        uint16_t Point_In_Sample;
        uint16_t Loop_Point_In_Sample;
        uint16_t Initial_Point_In_Ornament;
        uint16_t Point_In_Ornament;
        uint16_t Loop_Point_In_Ornament;
        uint16_t Address_In_Pattern;
        uint16_t Ton;
        uint16_t Ton_Deviation;

        uint8_t Note;
        uint8_t Addition_To_Note;
        uint8_t Number_Of_Notes_To_Skip;
        uint8_t Initial_Noise;
        uint8_t Current_Noise;
        uint8_t Volume;
        uint8_t Ton_Sliding_Counter;
        uint8_t Amplitude;
        uint8_t Amplitude_Delay;
        uint8_t Amplitude_Delay_Counter;

        int16_t Current_Ton_Sliding;
        int16_t Substruction_for_Ton_Sliding;

        int8_t Note_Skip_Counter;
        int8_t Addition_To_Amplitude;

        bool Envelope_Enabled;
        bool Sound_Enabled;
        bool Sample_Finished;
        bool Break_Sample_Loop;
        bool Break_Ornament_Loop;
    };

public:
    bool Open(Module& module) override;
    bool Decode(Frame& frame) override;
    void Close(Module& module) override;

private:
    bool Init();
    void PatternInterpreter(Channel& chan);
    void GetRegisters(Channel& chan, uint8_t& mixer);
    bool Play();

private:
    uint8_t* m_data;

    uint8_t Delay;
    uint8_t DelayCounter;
    uint8_t CurrentPosition;

    Channel ASC_A;
    Channel ASC_B;
    Channel ASC_C;

    uint8_t m_regs[16];
};
