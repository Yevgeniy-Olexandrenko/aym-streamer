#pragma once

#include "ChipSim.h"
#include "nes/NesApu.h"

class SimRP2A03 : public ChipSim, public NesApu
{
public:
    enum class ConvertMethod
    {
        SingleChip, DoubleChip, AY8930Chip
    };

    SimRP2A03();
    void Configure(ConvertMethod convertMethod, uint32_t dstClockRate);

    void Reset() override;
    void Write(int chip, Register reg, uint8_t data) override;
    void Simulate(int samples) override;
    void Convert(Frame& frame) override;

private:
    struct State;

    void ConvertToSingleChip(const State& state, Frame& frame);
    void ConvertToDoubleChip(const State& state, Frame& frame);
    void ConvertToAY8930Chip(const State& state, Frame& frame);

    uint16_t ConvertPeriod(uint16_t period) const;
    uint8_t  ConvertVolume(uint8_t  volume) const;

    void EnableTone(uint8_t& mixer, int chan) const;
    void EnableNoise(uint8_t& mixer, int chan) const;

private:
    ConvertMethod m_convertMethod;
    uint32_t m_dstClockRate;
};
