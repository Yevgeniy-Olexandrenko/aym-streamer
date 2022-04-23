#pragma once

#include "decoders/Decode.h"

class DecodePT3 : public ModuleDecoder
{
    #pragma pack(push, 1)
    struct Header
    {
        char     musicName[0x63];
        uint8_t  tonTableId;
        uint8_t  delay;
        uint8_t  numberOfPositions;
        uint8_t  loopPosition;
        uint16_t patternsPointer;
        uint8_t  samplesPointers[32 * 2];
        uint8_t  ornamentsPointers[16 * 2];
        uint8_t  positionList[1];
    };
    #pragma pack(pop)

    struct Channel
    {
        uint16_t addressInPattern;
        uint16_t ornamentPointer;
        uint16_t samplePointer;
        
        int currentAmplitudeSliding;
        int currentNoiseSliding;
        int currentEnvelopeSliding;
        int tonSlideCount;
        int currentOnOff;
        int onOffDelay;
        int offOnDelay;
        int tonSlideDelay;
        int currentTonSliding;
        int tonAccumulator;
        int tonSlideStep;
        int tonDelta;

        int8_t noteSkipCounter;

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
        
        bool envelopeEnabled;
        bool enabled;
        bool simpleGliss;

        uint16_t ton;
        uint8_t amplitude;
    };

    struct Global
    {
        int curEnvSlide;
        int envSlideAdd;
        uint8_t envBaseLo;
        uint8_t envBaseHi;
        uint8_t noiseBase;
        uint8_t delay;
        uint8_t addToNoise;
        uint8_t delayCounter;
        uint8_t currentPosition;
        int8_t curEnvDelay;
        int8_t envDelay;
    };

public:
	bool Open(Stream& stream) override;

protected:
    void Init() override;
    void Loop(uint8_t& currPosition, uint8_t& lastPosition, uint8_t& loopPosition) override;
    bool Play() override;

private:
    bool Play(int chip);
    void PatternInterpreter(int chip, Channel& chan);
    void GetRegisters(int chip, Channel& chan, uint8_t& mixer, int& envAdd);
    int  GetNoteFreq(int chip, int note);
    
private:
    uint32_t m_size;
    uint8_t  m_ver;

    struct {
        Header*  header;
        uint8_t* data;
        Global   glob;
        Channel  chan[3];
        int      ts;
    } m_chip[2];
};