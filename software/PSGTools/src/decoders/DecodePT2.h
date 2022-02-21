#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodePT2 : public Decoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint8_t  delay;
        uint8_t  numberOfPositions;
        uint8_t  loopPosition;
        uint8_t  samplesPointers[32 * 2];
        uint8_t  ornamentsPointers[16 * 2];
        uint16_t patternsPointer;
        char     musicName[30];
        uint8_t  positionList[256];
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t addressInPattern;
        uint16_t ornamentPointer;
        uint16_t samplePointer;

        uint8_t loopOrnamentPosition;
        uint8_t ornamentLength;
        uint8_t positionInOrnament;

        uint8_t loopSamplePosition;
        uint8_t sampleLength;
        uint8_t positionInSample;

        uint8_t volume;
        uint8_t numberOfNotesToSkip;
        uint8_t note;
        uint8_t slideToNote;
        int8_t currentTonSliding;
        int8_t tonDelta;
        uint8_t glisType;
        bool envelopeEnabled;
        bool enabled;
        int8_t glissade;
        int8_t additionToNoise;
        int8_t noteSkipCounter;

        uint16_t ton;
        uint8_t amplitude;
    };

public:
    bool Open(Module& module) override;
    bool Decode(Frame& frame) override;
    void Close(Module& module) override;

private:
    bool Init();
    bool Step();

    void PatternInterpreter(Channel& chan);
    void GetRegisters(Channel& chan, uint8_t& mixer);
    bool Play();

private:
    uint8_t* m_data;
    uint32_t m_loop;
    uint32_t m_tick;

    uint8_t m_delay;
    uint8_t m_delayCounter;
    uint8_t m_currentPosition;

    Channel m_chA;
    Channel m_chB;
    Channel m_chC;

    uint8_t m_regs[16];
};
