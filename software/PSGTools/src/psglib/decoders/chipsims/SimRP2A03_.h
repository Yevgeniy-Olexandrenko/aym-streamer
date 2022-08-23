#pragma once

#include "ChipSim.h"
#include "nes/NesApu.h"

class SimRP2A03_ : public ChipSim, public NesApu
{
public:
    enum class ConvertMethod
    {
        AY8910, AY8910x2, AY8930
    };

    SimRP2A03_();
    void Configure(ConvertMethod convertMethod, uint32_t dstClockRate);

    void Reset() override;
    void Write(uint8_t chip, uint8_t reg, uint8_t data) override;
    void Simulate(int samples) override;
    void ConvertToPSG(Frame& frame) override;
    void PostProcess(Stream& stream) override;

private:
    struct State
    {
        uint16_t pulse1_period;
        uint8_t  pulse1_volume;
        uint8_t  pulse1_duty;
        bool     pulse1_enable;

        uint16_t pulse2_period;
        uint8_t  pulse2_volume;
        uint8_t  pulse2_duty;
        bool     pulse2_enable;

        uint16_t triangle_period;
        bool     triangle_enable;
        
        uint16_t noise_period;
        uint8_t  noise_volume;
        bool     noise_mode;
        bool     noise_enable;
    };

    void ConvertToAY8910(const State& state, Frame& frame);
    void ConvertToAY8910x2(const State& state, Frame& frame);
    void ConvertToAY8930(const State& state, Frame& frame);

    uint16_t ConvertPeriod(uint16_t period) const;
    uint8_t  ConvertVolume(uint8_t  volume) const;

private:
    ConvertMethod m_convertMethod;
    uint32_t m_dstClockRate;
};
