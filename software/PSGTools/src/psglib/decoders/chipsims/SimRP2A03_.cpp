#include "SimRP2A03_.h"
#include "stream/Frame.h"
#include "stream/Stream.h"

namespace
{
    const float k_gainDefaultValue = 1.0f; // 1.75f;
}

SimRP2A03_::SimRP2A03_()
    : ChipSim(Type::RP2A03)
    , NesApu(44100, NesCpu::Clock::NTSC)
    , m_convertMethod(ConvertMethod::AY8910)
    , m_dstClockRate(m_cpuClock)
{
    Reset();
}

void SimRP2A03_::Configure(ConvertMethod convertMethod, uint32_t dstClockRate)
{
    m_convertMethod = convertMethod;
    m_dstClockRate  = dstClockRate;
}

void SimRP2A03_::Reset()
{
    NesApu::Reset();
}

void SimRP2A03_::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
    if (chip == 0)
    {
        NesApu::Write(0x4000 | reg, data);
    }
}

void SimRP2A03_::Simulate(int samples)
{
    while (samples--)
    {
        m_cpuCycles += m_cpuCyclesPerSample;
        NesApu::Process(m_cpuCycles >> 16);
        m_cpuCycles &= 0xFFFF;
    }
}

void SimRP2A03_::ConvertToPSG(Frame& frame)
{
    State state{};

    // pulse 1 channel
    state.pulse1_period = m_pulse1.timer_period;
    state.pulse1_volume = m_pulse1.envelope.out;
    state.pulse1_duty   = m_pulse1.duty;
    state.pulse1_enable = (m_pulse1.len_counter > 0 && !m_pulse1.sweep.silence);

    // pulse 2 channel
    state.pulse2_period = m_pulse2.timer_period;
    state.pulse2_volume = m_pulse2.envelope.out;
    state.pulse2_duty   = m_pulse2.duty;
    state.pulse2_enable = (m_pulse2.len_counter > 0 && !m_pulse2.sweep.silence);

    // triangle channel
    state.triangle_period = m_triangle.timer_period;
    state.triangle_enable = (m_triangle.lin_counter > 0 && m_triangle.len_counter > 0 && m_triangle.timer_period >= 2);

    // noise channel
    state.noise_period = m_noise.timer_period;
    state.noise_volume = m_noise.envelope.out;
    state.noise_mode   = m_noise.mode;
    state.noise_enable = (m_noise.len_counter > 0);

    switch (m_convertMethod)
    {
    case ConvertMethod::AY8910:
        ConvertToAY8910(state, frame);
        break;

    case ConvertMethod::AY8910x2:
        ConvertToAY8910x2(state, frame);
        break;

    case ConvertMethod::AY8930:
        ConvertToAY8930(state, frame);
        break;
    }
}

void SimRP2A03_::PostProcess(Stream& stream)
{
    //
}

void SimRP2A03_::ConvertToAY8910(const State& state, Frame& frame)
{
    uint8_t mixer = 0x3F;

    // Pulse 1 -> Square A
    if (state.pulse1_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse1_period);
        uint8_t  volume = ConvertVolume(state.pulse1_volume);

        frame.UpdatePeriod(A_Period, period);
        frame.Update(A_Volume, volume);
        mixer &= ~(1 << 0);
    }

    // Pulse 2 -> Square C
    if (state.pulse2_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse2_period);
        uint8_t  volume = ConvertVolume(state.pulse2_volume);

        frame.UpdatePeriod(C_Period, period);
        frame.Update(C_Volume, volume);
        mixer &= ~(1 << 2);
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);

        if (frame.data(0, E_Shape) != 0x0E)
        {
            frame.Update(E_Shape, 0x0E);
        }
        frame.UpdatePeriod(E_Period, period);
        frame.Update(B_Volume, 0x10);
    }
    else
    {
        frame.UpdatePeriod(E_Period, 0xFFFF);
    }

    // Noise -> automatically chosen channel A or C
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 7);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        frame.UpdatePeriod(N_Period, period);
        if (!state.pulse1_enable)
        {
            frame.Update(A_Volume, volume);
            mixer &= ~(1 << 3);
        }
        else if (!state.pulse2_enable)
        {
            frame.Update(C_Volume, volume);
            mixer &= ~(1 << 5);
        }
        else
        {
            auto proximityToA = std::abs(int(volume) - int(frame.data(0, A_Volume)));
            auto proximityToC = std::abs(int(volume) - int(frame.data(0, C_Volume)));
            mixer &= ~(1 << (proximityToA < proximityToC ? 3 : 5));
        }
    }
    
    // Mixers
    {
        frame.Update(Mixer, mixer);
    }
}

void SimRP2A03_::ConvertToAY8910x2(const State& state, Frame& frame)
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

        frame.UpdatePeriod(0, A_Period, period0);
        frame.Update(0, A_Volume, volume);
        mixer0 &= ~(1 << 0);
        
        if (period1 != period0)
        {
            frame.UpdatePeriod(1, A_Period, period1);
            frame.Update(1, A_Volume, volume);
            mixer1 &= ~(1 << 0);
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

        frame.UpdatePeriod(0, C_Period, period0);
        frame.Update(0, C_Volume, volume);
        mixer0 &= ~(1 << 2);

        if (period1 != period0)
        {
            frame.UpdatePeriod(1, C_Period, period1);
            frame.Update(1, C_Volume, volume);
            mixer1 &= ~(1 << 2);
        }
    }

    // Triangle -> Chip 0 Envelope in Channel B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);

        if (frame.data(0, E_Shape) != 0x0E) frame.Update(0, E_Shape, 0x0E);
        frame.UpdatePeriod(0, E_Period, period);
        frame.Update(0, B_Volume, 0x10);
    }
    else
    {
        frame.UpdatePeriod(0, E_Period, 0xFFFF);
    }

    // Noise -> Chip 1 Noise in Channel B
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 7);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        frame.UpdatePeriod(1, N_Period, period);
        frame.Update(1, B_Volume, volume);
        mixer1 &= ~(1 << 4);
    }
    
    // Mixers
    {
        frame.Update(0, Mixer, mixer0);
        frame.Update(1, Mixer, mixer1);
    }
}

void SimRP2A03_::ConvertToAY8930(const State& state, Frame& frame)
{
    uint8_t mixer = 0x3F;

    // go to expanded mode
    if (frame.data(0, Mode_Bank) != 0xA0) frame.Update(Mode_Bank, 0xA0);

    // workaround for mixer
    frame.UpdatePeriod(B_Period, 0x0000);
    frame.Update(B_Duty, 0x08);
    mixer &= ~(1 << 1);

    // Pulse 1 -> Square A
    if (state.pulse1_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse1_period << 1);
        uint8_t  volume = ConvertVolume(state.pulse1_volume);
        uint8_t  duty = 0x02 + state.pulse1_duty;

        frame.UpdatePeriod(A_Period, period);
        frame.Update(A_Volume, volume);
        frame.Update(A_Duty, duty);
        mixer &= ~(1 << 0);
    }

    // Pulse 2 -> Square C
    if (state.pulse2_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse2_period << 1);
        uint8_t  volume = ConvertVolume(state.pulse2_volume);
        uint8_t  duty = 0x02 + state.pulse2_duty;

        frame.UpdatePeriod(C_Period, period);
        frame.Update(C_Volume, volume);
        frame.Update(C_Duty, duty);
        mixer &= ~(1 << 2);
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 3);
       
        if (frame.data(0, EB_Shape) != 0x0E)
        {
            frame.Update(EB_Shape, 0x0E);
        }
        frame.UpdatePeriod(EB_Period, period);
        frame.Update(B_Volume, 0x20);
    }
    else
    {
        frame.UpdatePeriod(EB_Period, 0xFFFF);
    }

    // Noise -> automatically chosen channel A or C
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 4);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        frame.Update(N_AndMask, 0x0F);
        frame.Update(N_OrMask,  0x00);
        frame.UpdatePeriod(N_Period, period);

        if (!state.pulse1_enable)
        {
            frame.Update(A_Volume, volume);
            mixer &= ~(1 << 3);
        }
        else if (!state.pulse2_enable)
        {
            frame.Update(C_Volume, volume);
            mixer &= ~(1 << 5);
        }
        else
        {
            auto proximityToA = std::abs(int(volume) - int(frame.data(0, A_Volume)));
            auto proximityToC = std::abs(int(volume) - int(frame.data(0, C_Volume)));
            mixer &= ~(1 << (proximityToA < proximityToC ? 3 : 5));
        }
    }

    // Mixers
    {
        frame.Update(Mixer, mixer);
    }
}

uint16_t SimRP2A03_::ConvertPeriod(uint16_t period) const
{
    double result = double(period) * double(m_dstClockRate) / double(m_cpuClock);
    return uint16_t(result + 0.5);
}

uint8_t SimRP2A03_::ConvertVolume(uint8_t volume) const
{
    if (volume)
    {
        float signal = std::min(std::sqrt(float(volume) * k_gainDefaultValue / 15.f), 1.f);
        float maxVol = (m_convertMethod == ConvertMethod::AY8930 ? 31.f : 15.f);
        return uint8_t(maxVol * signal + 0.5f);
    }
    return 0;
    
}
