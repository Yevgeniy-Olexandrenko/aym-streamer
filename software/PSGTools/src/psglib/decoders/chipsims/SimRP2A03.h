#pragma once

#define NEW_NES_APU 1

#include "ChipSim.h"
#if NEW_NES_APU
#include "nes/NesApuNew.h"
using NesApu = NesApuNew;
#else
#include "nes/NesApu.h"
#endif

class SimRP2A03 : public ChipSim, public NesApu
{
public:
    enum class OutputType
    {
        SingleChip, DoubleChip, AY8930Chip
    };

    SimRP2A03();

    void ConfigureClock(int srcClock, int dstClock) override;
    void ConfigureOutput(OutputType outputType);

    void Reset() override;
    void Write(int chip, Register reg, uint8_t data) override;
    void Simulate(int samples) override;
    void Convert(Frame& frame) override;

private:
    struct State;

    void ConvertToSingleChip(const State& state, Frame& frame);
    void ConvertToDoubleChip(const State& state, Frame& frame);
    void ConvertToAY8930Chip(const State& state, Frame& frame);
    void DistributeNoiseBetweenChannels(const State& state, Frame& frame, uint8_t& mixer);

    float   VolumeToLevel(uint8_t volume) const;
    uint8_t LevelToVolume(float   level ) const;
    uint8_t ConvertVolume(uint8_t volume) const;

private:
    OutputType m_outputType;
};
