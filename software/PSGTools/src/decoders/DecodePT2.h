#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodePT2 : public Decoder
{
    struct AYSongInfo
    {
        uint8_t* module;
        void* data;
    };

    struct PT2_File
    {
        uint8_t PT2_Delay;
        uint8_t PT2_NumberOfPositions;
        uint8_t PT2_LoopPosition;
        uint8_t PT2_SamplesPointers0[64];
        uint8_t PT2_OrnamentsPointers0[32];
        uint8_t PT2_PatternsPointer0, PT2_PatternsPointer1;
        char PT2_MusicName[30];
        uint8_t PT2_PositionList[65535 - 131];
    };

    struct PT2_Channel_Parameters
    {
        uint16_t Address_In_Pattern, OrnamentPointer, SamplePointer, Ton;
        uint8_t Loop_Ornament_Position, Ornament_Length, Position_In_Ornament, Loop_Sample_Position, Sample_Length, Position_In_Sample, Volume, Number_Of_Notes_To_Skip, Note, Slide_To_Note, Amplitude;
        int8_t Current_Ton_Sliding, Ton_Delta;
        int GlissType;
        bool Envelope_Enabled, Enabled;
        int8_t Glissade, Addition_To_Noise, Note_Skip_Counter;
    };

    struct PT2_Parameters
    {
        uint8_t DelayCounter, Delay, CurrentPosition;
    };

    struct PT2_SongInfo
    {
        PT2_Parameters PT2;
        PT2_Channel_Parameters PT2_A, PT2_B, PT2_C;
    };

public:
    bool Open(Module& module) override;
    bool Decode(Frame& frame) override;
    void Close(Module& module) override;

private:
    bool Init(AYSongInfo& info);
    bool Step();

    void PT2_PatternInterpreter(AYSongInfo& info, PT2_Channel_Parameters& chan);
    void PT2_GetRegisters(AYSongInfo& info, PT2_Channel_Parameters& chan, uint8_t& TempMixer);
    bool PT2_Play(AYSongInfo& info);
    void PT2_Cleanup(AYSongInfo& info);

private:
    unsigned loop;
    unsigned tick;
    AYSongInfo info;
    uint8_t regs[16];
};
