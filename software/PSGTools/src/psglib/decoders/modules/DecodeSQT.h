#pragma once

#include "decoders/Decoder.h"

class DecodeSQT : public ModuleDecoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint16_t size;
        uint16_t samplesPointer;
        uint16_t ornamentsPointer;
        uint16_t patternsPointer;
        uint16_t positionsPointer;
        uint16_t loopPointer;
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t addressInPattern;
        uint16_t samplePointer;
        uint16_t pointInSample;
        uint16_t ornamentPointer;
        uint16_t pointInOrnament;
        uint16_t ton;
        uint16_t ix27;

        uint8_t volume;
        uint8_t amplitude;
        uint8_t note;
        uint8_t ix21;

        int16_t tonSlideStep;
        int16_t currentTonSliding;

        int8_t sampleTikCounter;
        int8_t ornamentTikCounter;
        int8_t transposit;

        bool enabled;
        bool envelopeEnabled;
        bool ornamentEnabled;
        bool gliss;
        bool mixNoise;
        bool mixTon;
        bool b4ix0;
        bool b6ix0;
        bool b7ix0;
    };

public:
    bool Open(Stream& stream) override;

protected:
    void Init() override;
    void Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition) override;
    bool Play() override;

private:
    bool PreInit();
    bool NextPosition(Channel& chan);
    void PatternInterpreter(Channel& chan);
    void GetRegisters(Channel& chan, uint8_t& mixer);

    void Call_LC191(Channel& chan, uint16_t& ptr);
    void Call_LC283(Channel& chan, uint16_t& ptr);
    void Call_LC1D1(Channel& chan, uint16_t& ptr, uint8_t a);
    void Call_LC2A8(Channel& chan, uint8_t a);
    void Call_LC2D9(Channel& chan, uint8_t a);

private:
    uint8_t  m_delay;
    uint8_t  m_delayCounter;
    uint8_t  m_linesCounter;
    uint16_t m_positionsPointer;
    uint8_t  m_lastPosition;

    Channel m_chA;
    Channel m_chB;
    Channel m_chC;
};