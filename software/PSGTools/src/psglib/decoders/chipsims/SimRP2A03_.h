#pragma once

#include "ChipSim.h"
#include "nes/NesApu.h"

class SimRP2A03_ : public ChipSim, public NesApu
{
public:
    SimRP2A03_();

    void Reset() override;
    void Write(uint8_t chip, uint8_t reg, uint8_t data) override;
    void Simulate(int samples) override;
    void ConvertToPSG(Frame& frame) override;
    void PostProcess(Stream& stream) override;

private:
    int m_maxVol[3]{};
};
