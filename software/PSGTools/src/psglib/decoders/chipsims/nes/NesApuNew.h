#pragma once

#include <stdint.h>
#include "NesCpu.h"

class NesApuNew
{
	struct Envelope
    {
        bool    loop_halt;
        bool    const_vol;
        uint8_t vol_period;
        uint8_t counter;
        uint8_t divider;
        bool    start;
        uint8_t out;
    };

    struct Sweep
    {
        bool     enable;
        uint8_t  period;
        bool     negate;
        uint8_t  shift;
        uint8_t  divider;
        uint16_t target;
        bool     silence;
        bool     reload;
    };

    struct Pulse
    {
        uint8_t  duty;         // duty cycle of the pulse
        Envelope envelope;
        Sweep    sweep;
        uint16_t timer_period; // controls the frequency of the pulse
        uint8_t  len_counter;  // length counter; when reaches zero, channel is silenced
        uint16_t timer;        // set to timer_period * 8 (left shift) when timer period set, or this overflows
        uint8_t  phase;        // pulse wave phase/sequence
        bool     enabled;
    };

    struct Triangle
    {
        bool     control;
        bool     halt;
        uint8_t  lin_reload;   // linear counter reload value
        uint16_t timer_period; // period for the wave frequency timer
        uint8_t  len_counter;  // length counter
        uint8_t  lin_counter;  // linear counter
        uint16_t timer;        // sequence timer
        uint8_t  phase;        // triangle wave phase/sequence
        bool     enabled;
    };

    struct Noise
    {
        Envelope envelope;
        bool     mode;
        uint8_t  period_lut;
        uint16_t timer_period;
        uint16_t timer;
        uint8_t  len_counter;
        uint16_t shiftreg;     // shift reg for noise
        bool     enabled;
    };

    struct DMC
    {
        bool     irq;
        bool     loop;
        uint8_t  rate;
        uint16_t rate_actual;
        uint16_t timer;

        // memory reader
        uint16_t address;
        uint16_t addresscur;
        uint16_t length;
        uint16_t bytesleft;
        uint8_t  sample;
        bool     buffered;

        // output unit
        uint8_t  counter;
        uint8_t  shiftreg;
        uint8_t  bitsleft;
        uint8_t  silence;

        bool     control;
    };

    struct FrameCnt
    {
        bool     mode;
        bool     interrupt;
        bool     int_inhibit;
        bool     updated;
        uint16_t count;
    };

public:
    enum
    {
        PULSE1DUTYVOL = 0x4000,
        PULSE1SWEEP   = 0x4001,
        PULSE1TMRL    = 0x4002,
        PULSE1TMRH    = 0x4003,
        PULSE2DUTYVOL = 0x4004,
        PULSE2SWEEP   = 0x4005,
        PULSE2TMRL    = 0x4006,
        PULSE2TMRH    = 0x4007,
        TRICOUNTER    = 0x4008,
        TRITMRL       = 0x400A,
        TRITMRH       = 0x400B,
        NOISEVOL      = 0x400C,
        NOISEPERIOD   = 0x400E,
        NOISELCL      = 0x400F,
        DMCIRQ        = 0x4010,
        DMCCOUNTER    = 0x4011,
        DMCADDR       = 0x4012,
        DMCLENGTH     = 0x4013,
        STATUS        = 0x4015,
        FRAMECNTR     = 0x4017
    };

public:
    NesApuNew();

    void Init(int sampleRate, int cpuClock);
    void Reset();

    void Write(uint16_t addr, uint8_t data);
    uint8_t Read(uint16_t addr);

    int32_t Output();

protected:
    void Process(uint32_t cpu_cycles);

private:
    void CalculateSweepPulse1();
    void CalculateSweepPulse2();
    void UpdateQuarterFrame();
    void UpdateHalfFrame();
    void UpdateNoise();
    
    uint8_t CpuRead(uint16_t addr);

protected:
    Pulse    m_pulse1{};
    Pulse    m_pulse2{};
    Triangle m_triangle{};
    Noise    m_noise{};
    DMC      m_dmc{};
    FrameCnt m_frame{};
    uint32_t m_cpuCycles;

    uint32_t m_cpuClock;
    uint32_t m_cpuCyclesPerSample;
    const uint16_t* m_noisePeriods;
    const uint16_t* m_dmcPeriods;

    uint32_t m_P12MixLut[31];
    uint32_t m_TNDMixLut[203];
};
