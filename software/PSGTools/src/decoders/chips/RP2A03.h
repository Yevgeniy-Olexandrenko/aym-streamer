#pragma once

#include <stdint.h>

#define NES_CPU_FREQUENCY (1789773)
#define SAMPLING_RATE     (44100)
#define CONST_SHIFT_BITS  (4)

class RP2A03
{
    typedef struct
    {
        uint16_t lenCounter;
        uint16_t linearCounter;
        bool linearReloadFlag;

        uint32_t period;
        uint32_t counter;

        uint8_t decayCounter;
        uint8_t divider;
        bool updateEnvelope;
        uint8_t envVolume;

        uint8_t sequencer;

        uint8_t volume;
        uint32_t output;

        uint8_t sweepCounter;

        bool dmcActive;
        uint32_t dmcAddr;
        uint32_t dmcLen;
        uint8_t dmcBuffer;
        bool    dmcIrqFlag;
    } ChannelInfo;

public:
	void Write(uint16_t reg, uint8_t val);
    void Update(int samples);


private:
    void updateFrameCounter();
    void updateRectChannel(int i);
    void updateTriangleChannel(ChannelInfo& chan);
    void updateNoiseChannel(ChannelInfo& chan);
    void updateDmcChannel(ChannelInfo& chan);

public:
    uint8_t m_regs[0x20]{};
    ChannelInfo m_chan[5]{};

    //uint32_t m_rectVolTable[16]{};
    //uint32_t m_triVolTable[16]{};
    //uint32_t m_noiseVolTable[16]{};
    //uint32_t m_dmcVolTable[16]{};

    uint32_t m_lastFrameCounter = 0;
    uint8_t m_apuFrames = 0;
    uint16_t m_shiftNoise;

    bool m_quaterSignal = false;
    bool m_halfSignal = false;
    bool m_fullSignal = false;
};
