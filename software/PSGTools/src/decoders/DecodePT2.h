#pragma once

#include <stdint.h>
#include "Decoder.h"

class DecodePT2 : public ModuleDecoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint8_t  delay;
        uint8_t  numberOfPositions;
        uint8_t  loopPosition;
        uint16_t samplesPointers[32];
        uint16_t ornamentsPointers[16];
        uint16_t patternsPointer;
        uint8_t  musicName[30];
        uint8_t  positionList[256];
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t addressInPattern;
        uint16_t ornamentPointer;
        uint16_t samplePointer;
        uint16_t ton;

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
        uint8_t amplitude;
        
        int8_t currentTonSliding;
        int8_t tonDelta;
        int8_t glissType;
        int8_t glissade;
        int8_t additionToNoise;
        int8_t noteSkipCounter;

        bool envelopeEnabled;
        bool enabled;
    };

public:
    bool Open(Module& module) override;

protected:
    void Init() override;
    void Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition) override;
    bool Play() override;

private:
    void PatternInterpreter(Channel& chan);
    void GetRegisters(Channel& chan, uint8_t& mixer);

private:
    uint8_t m_delay;
    uint8_t m_delayCounter;
    uint8_t m_currentPosition;

    Channel m_chA;
    Channel m_chB;
    Channel m_chC;
};
