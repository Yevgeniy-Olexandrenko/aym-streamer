#pragma once

#include "decoders/Decode.h"

class DecodeSTP : public ModuleDecoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint8_t  delay;
        uint16_t positionsPointer;
        uint16_t patternsPointer;
        uint16_t ornamentsPointer;
        uint16_t samplesPointer;
        uint8_t  initId;
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t ornamentPointer;
        uint16_t samplePointer;
        uint16_t addressInPattern;
        uint16_t ton;

        int16_t currentTonSliding;

        uint8_t positionInOrnament;
        uint8_t loopOrnamentPosition;
        uint8_t ornamentLength;
        uint8_t positionInSample;
        uint8_t loopSamplePosition;
        uint8_t sampleLength;
        uint8_t volume;
        uint8_t numberOfNotesToSkip;
        uint8_t note;
        uint8_t amplitude;

        int8_t glissade;
        int8_t noteSkipCounter;
        
        bool envelopeEnabled;
        bool enabled;
    };

public:
    bool Open(Stream& stream) override;

protected:
    void Init() override;
    void Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition) override;
    bool Play() override;

private:
    void PatternInterpreter(Channel& chan);
    void GetRegisters(Channel& chan, uint8_t& mixer);

private:
    uint8_t m_delayCounter;
    uint8_t m_transposition;
    uint8_t m_currentPosition;

    Channel m_chA;
    Channel m_chB;
    Channel m_chC;
};