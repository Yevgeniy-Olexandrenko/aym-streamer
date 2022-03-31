#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodeSTC : public Decoder
{
    #pragma pack(push, 1)
    struct STC_File
    {
        unsigned char ST_Delay;
        unsigned char ST_PositionsPointer0, ST_PositionsPointer1;
        unsigned char ST_OrnamentsPointer0, ST_OrnamentsPointer1;
        unsigned char ST_PatternsPointer0, ST_PatternsPointer1;
        signed char   ST_Name[18];
        unsigned char ST_Size0, ST_Size1;
    };
    #pragma pack(pop)

    struct STC_Channel_Parameters
    {
        unsigned short Address_In_Pattern, SamplePointer, OrnamentPointer, Ton;
        unsigned char Amplitude, Note, Position_In_Sample, Number_Of_Notes_To_Skip;
        signed char Sample_Tik_Counter, Note_Skip_Counter;
        bool Envelope_Enabled;
    };

    struct STC_Parameters
    {
        unsigned char DelayCounter, Transposition, CurrentPosition;
    };

public:
    bool STC_Init();
    bool Step();

    void STC_PatternInterpreter(STC_Channel_Parameters& chan);
    void STC_GetRegisters(STC_Channel_Parameters& chan, unsigned char& TempMixer);
    bool STC_Play();

private:
    uint8_t* m_data;

    STC_Parameters STC;
    STC_Channel_Parameters STC_A;
    STC_Channel_Parameters STC_B;
    STC_Channel_Parameters STC_C;

    uint8_t m_regs[16];
};
