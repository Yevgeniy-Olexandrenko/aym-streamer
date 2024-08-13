#pragma once

#include <stdint.h>

namespace PT2
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

    void Init();
    void Play();
}

