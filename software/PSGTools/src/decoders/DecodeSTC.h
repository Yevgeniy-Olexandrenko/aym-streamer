#pragma once

#include "Decode.h"

class DecodeSTC : public ModuleDecoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint8_t  delay;
        uint16_t positionsPointer;
        uint16_t ornamentsPointer;
        uint16_t patternsPointer;
        uint8_t  identifier[18];
        uint16_t size;
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t addressInPattern;
        uint16_t samplePointer;
        uint16_t ornamentPointer;
        uint16_t ton;

        uint8_t amplitude;
        uint8_t note;
        uint8_t positionInSample;
        uint8_t numberOfNotesToSkip;

        int8_t sampleTikCounter;
        int8_t noteSkipCounter;

        bool envelopeEnabled;
    };

public:
    bool Open(Stream& stream) override;

protected:
    void Init() override;
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
