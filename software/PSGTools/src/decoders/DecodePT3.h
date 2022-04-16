#pragma once

#include "Decoder.h"

class DecodePT3 : public Decoder
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
	bool Open   (Module& module) override;
	bool Decode (Frame&  frame ) override;
	void Close  (Module& module) override;

private:
    bool Init();
    bool Step();

    int  GetNoteFreq(int chip, int note);
    bool GetRegisters(int chip);
    void PatternInterpreter(int chip, Channel& chan);
    void ChangeRegisters(int chip, Channel& chan);
    
private:
    uint8_t* m_data;
    uint32_t m_size;
    uint32_t m_loop;
    uint32_t m_tick;

    uint8_t  m_ver;
    bool     m_ts;

    struct {
        Header*  header;
        uint8_t* data;
        Global   glob;
        Channel  chan[3];
        int      ts;
    } m_chip[2];

    int AddToEnv;
    uint8_t TempMixer;
    //

    uint8_t m_regs[2][16];
};