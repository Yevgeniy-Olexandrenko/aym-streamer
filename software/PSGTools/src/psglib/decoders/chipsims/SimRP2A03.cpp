#include "SimRP2A03.h"
#include "stream/Frame.h"
#include "stream/Stream.h"

namespace
{
    const float k_gainDefaultValue = 1.0f; // 1.75f;
}

struct SimRP2A03::State
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

SimRP2A03::SimRP2A03()
    : ChipSim(Type::RP2A03)
    , NesApu(44100, NesCpu::Clock::NTSC)
    , m_convertMethod(ConvertMethod::SingleChip)
    , m_dstClockRate(m_cpuClock)
{
    Reset();
}

void SimRP2A03::Configure(ConvertMethod convertMethod, uint32_t dstClockRate)
{
    m_convertMethod = convertMethod;
    m_dstClockRate  = dstClockRate;
}

void SimRP2A03::Reset()
{
    NesApu::Reset();
}

void SimRP2A03::Write(int chip, Register reg, uint8_t data)
{
    if (chip == 0)
    {
        NesApu::Write(0x4000 | reg, data);
    }
}

void SimRP2A03::Simulate(int samples)
{
    while (samples--)
    {
        m_cpuCycles += m_cpuCyclesPerSample;
        NesApu::Process(m_cpuCycles >> 16);
        m_cpuCycles &= 0xFFFF;
    }
}

void SimRP2A03::Convert(Frame& frame)
{
    State state{};

    // pulse 1 channel
    state.pulse1_period = m_pulse1.timer_period;
    state.pulse1_volume = m_pulse1.envelope.out;
    state.pulse1_duty   = m_pulse1.duty;
    state.pulse1_enable = (m_pulse1.len_counter && !m_pulse1.sweep.silence && m_pulse1.envelope.out);

    // pulse 2 channel
    state.pulse2_period = m_pulse2.timer_period;
    state.pulse2_volume = m_pulse2.envelope.out;
    state.pulse2_duty   = m_pulse2.duty;
    state.pulse2_enable = (m_pulse2.len_counter && !m_pulse2.sweep.silence && m_pulse2.envelope.out);

    // triangle channel
    state.triangle_period = m_triangle.timer_period;
    state.triangle_enable = (m_triangle.lin_counter > 0 && m_triangle.len_counter > 0 && m_triangle.timer_period >= 2);

    // noise channel
    state.noise_period = m_noise.timer_period;
    state.noise_volume = m_noise.envelope.out;
    state.noise_mode   = m_noise.mode;
    state.noise_enable = (m_noise.len_counter && m_noise.envelope.out);

    switch (m_convertMethod)
    {
    case ConvertMethod::SingleChip: ConvertToSingleChip(state, frame); break;
    case ConvertMethod::DoubleChip: ConvertToDoubleChip(state, frame); break;
    case ConvertMethod::AY8930Chip: ConvertToAY8930Chip(state, frame); break;
    }
}

void SimRP2A03::ConvertToSingleChip(const State& state, Frame& frame)
{
    uint8_t mixer = 0x3F;

    // Pulse 1 -> Square A
    if (state.pulse1_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse1_period);
        uint8_t  volume = ConvertVolume(state.pulse1_volume);

        EnableTone(mixer, Frame::Channel::A);
        frame.UpdatePeriod(A_Period, period);
        frame.Update(A_Volume, volume);
    }

    // Pulse 2 -> Square C
    if (state.pulse2_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse2_period);
        uint8_t  volume = ConvertVolume(state.pulse2_volume);

        EnableTone(mixer, Frame::Channel::C);
        frame.UpdatePeriod(C_Period, period);
        frame.Update(C_Volume, volume);
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);

        if (frame.data(0, E_Shape) != 0x0A) frame.Update(E_Shape, 0x0A);
        frame.UpdatePeriod(E_Period, period);
        frame.Update(B_Volume, 0x10);
    }
    else
    {
        frame.Update(B_Volume, 0x08);
    }
    
    // Noise -> automatically chosen channel A or C
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 7);

        int volumeN = ConvertVolume(state.noise_volume);
        int volumeA = frame.data(0, A_Volume);
        int volumeC = frame.data(0, C_Volume);

        frame.UpdatePeriod(N_Period, period);

        if (!state.pulse1_enable)
            frame.Update(A_Volume, volumeN / std::sqrt(2.f));

        else if (!state.pulse2_enable)
            frame.Update(C_Volume, volumeN / std::sqrt(2.f));

        if (volumeA + volumeC <= volumeN * std::sqrt(2.f))
        {
            EnableNoise(mixer, Frame::Channel::A);
            EnableNoise(mixer, Frame::Channel::C);
        }
        else if (std::abs(volumeN - volumeA) < std::abs(volumeN - volumeC))
            EnableNoise(mixer, Frame::Channel::A);
        else
            EnableNoise(mixer, Frame::Channel::C);
    }
        
    // Mixer
    {
        frame.Update(Mixer, mixer);
    }
}

void SimRP2A03::ConvertToDoubleChip(const State& state, Frame& frame)
{
    uint8_t mixer0 = 0x3F;
    uint8_t mixer1 = 0x3F;

    // Pulse 1 -> Chip 0 Square A + Chip 1 Square A
    if (state.pulse1_enable)
    {
        uint16_t period0 = ConvertPeriod(state.pulse1_period);
        uint8_t  volume  = ConvertVolume(state.pulse1_volume);

        uint16_t period1 = period0;
        if (state.pulse1_duty == 0) period1 ^= 1;
        if (state.pulse1_duty == 1 || state.pulse1_duty == 3) period1 >>= 1;

        EnableTone(mixer0, Frame::Channel::A);
        frame.UpdatePeriod(0, A_Period, period0);
        frame.Update(0, A_Volume, volume);
        
        if (period1 != period0)
        {
            EnableTone(mixer1, Frame::Channel::A);
            frame.UpdatePeriod(1, A_Period, period1);
            frame.Update(1, A_Volume, volume);
        }
    }

    // Pulse 2 -> Chip 0 Square C + Chip 1 Square C
    if (state.pulse2_enable)
    {
        uint16_t period0 = ConvertPeriod(state.pulse2_period);
        uint8_t  volume  = ConvertVolume(state.pulse2_volume);

        uint16_t period1 = period0;
        if (state.pulse2_duty == 0) period1 ^= 1;
        if (state.pulse2_duty == 1 || state.pulse2_duty == 3) period1 >>= 1;

        EnableTone(mixer0, Frame::Channel::C);
        frame.UpdatePeriod(0, C_Period, period0);
        frame.Update(0, C_Volume, volume);

        if (period1 != period0)
        {
            EnableTone(mixer1, Frame::Channel::C);
            frame.UpdatePeriod(1, C_Period, period1);
            frame.Update(1, C_Volume, volume);
        }
    }

    // Triangle -> Chip 0 Envelope in Channel B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);

        if (frame.data(0, E_Shape) != 0x0A) frame.Update(0, E_Shape, 0x0A);
        frame.UpdatePeriod(0, E_Period, period);
        frame.Update(0, B_Volume, 0x10);
    }
    else
    {
        frame.Update(0, B_Volume, 0x08);
    }
    
    // Noise -> Chip 1 Noise in Channel B
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 7);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        EnableNoise(mixer1, Frame::Channel::B);
        frame.UpdatePeriod(1, N_Period, period);
        frame.Update(1, B_Volume, volume);
    }
    
    // Mixers
    {
        frame.Update(0, Mixer, mixer0);
        frame.Update(1, Mixer, mixer1);
    }
}

void SimRP2A03::ConvertToAY8930Chip(const State& state, Frame& frame)
{
    uint8_t mixer = 0x3F;

    // go to expanded mode
    if (!frame.IsExpMode(0)) frame.SetExpMode(0, true);

    // workaround for mixer
    EnableTone(mixer, Frame::Channel::B);
    frame.UpdatePeriod(B_Period, 0x0000);
    frame.Update(B_Duty, 0x08);

    // Pulse 1 -> Square A
    if (state.pulse1_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse1_period << 1);
        uint8_t  volume = ConvertVolume(state.pulse1_volume);
        uint8_t  duty = 0x02 + state.pulse1_duty;

        EnableTone(mixer, Frame::Channel::A);
        frame.UpdatePeriod(A_Period, period);
        frame.Update(A_Volume, volume);
        frame.Update(A_Duty, duty);
    }

    // Pulse 2 -> Square C
    if (state.pulse2_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse2_period << 1);
        uint8_t  volume = ConvertVolume(state.pulse2_volume);
        uint8_t  duty = 0x02 + state.pulse2_duty;

        EnableTone(mixer, Frame::Channel::C);
        frame.UpdatePeriod(C_Period, period);
        frame.Update(C_Volume, volume);
        frame.Update(C_Duty, duty);
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);

        if (frame.data(0, EB_Shape) != 0x0A) frame.Update(EB_Shape, 0x0A);
        frame.UpdatePeriod(EB_Period, period);
        frame.Update(B_Volume, 0x20);
    }
    else
    {
        frame.Update(B_Volume, 0x10);
    }

    // Noise -> automatically chosen channel A or C
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 4);

        int volumeN = ConvertVolume(state.noise_volume);
        int volumeA = frame.data(0, A_Volume);
        int volumeC = frame.data(0, C_Volume);

        frame.Update(N_AndMask, 0x0F);
        frame.Update(N_OrMask,  0x00);
        frame.UpdatePeriod(N_Period, period);

        if (!state.pulse1_enable)
            frame.Update(A_Volume, volumeN / std::sqrt(2.f));

        else if (!state.pulse2_enable)
            frame.Update(C_Volume, volumeN / std::sqrt(2.f));
        
        if (volumeA + volumeC <= volumeN * std::sqrt(2.f))
        {
            EnableNoise(mixer, Frame::Channel::A);
            EnableNoise(mixer, Frame::Channel::C);
        }
        else if (std::abs(volumeN - volumeA) < std::abs(volumeN - volumeC))
            EnableNoise(mixer, Frame::Channel::A);
        else
            EnableNoise(mixer, Frame::Channel::C);
    }

    // Mixers
    {
        frame.Update(Mixer, mixer);
    }
}

uint16_t SimRP2A03::ConvertPeriod(uint16_t period) const
{
    auto converted = double(period) * m_dstClockRate / m_cpuClock;
    return uint16_t(converted + 0.5f);
}

uint8_t SimRP2A03::ConvertVolume(uint8_t volume) const
{
    auto signal = std::min(std::sqrt(float(volume) * k_gainDefaultValue / 15.f), 1.f);
    auto maxVol = (m_convertMethod == ConvertMethod::AY8930Chip ? 31.f : 15.f);
    return uint8_t(maxVol * signal + 0.5f);
}

void SimRP2A03::EnableTone(uint8_t& mixer, int chan) const
{
    mixer &= ~(1 << chan);
}

void SimRP2A03::EnableNoise(uint8_t& mixer, int chan) const
{
    mixer &= ~(1 << (3 + chan));
}

void SimRP2A03::DisableTone(uint8_t& mixer, int chan) const
{
    mixer |= (1 << chan);
}

void SimRP2A03::DisableNoise(uint8_t& mixer, int chan) const
{
    mixer |= (1 << (3 + chan));
}
