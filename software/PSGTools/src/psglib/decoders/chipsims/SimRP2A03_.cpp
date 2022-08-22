#include "SimRP2A03_.h"
#include "stream/Frame.h"
#include "stream/Stream.h"

SimRP2A03_::SimRP2A03_()
    : ChipSim(Type::RP2A03)
    , NesApu(44100, NesCpu::Clock::NTSC)
    , m_convertMethod(ConvertMethod::AY8910)
{
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
    state.pulse1_volume = (!m_pulse1.len_counter || m_pulse1.sweep.silence ? 0 : m_pulse1.envelope.out);
    state.pulse1_duty   = m_pulse1.duty;
    state.pulse1_enable = (state.pulse1_volume > 0);

    // pulse 2 channel
    state.pulse2_period = m_pulse2.timer_period;
    state.pulse2_volume = (!m_pulse2.len_counter || m_pulse2.sweep.silence ? 0 : m_pulse2.envelope.out);
    state.pulse2_duty   = m_pulse2.duty;
    state.pulse2_enable = (state.pulse2_volume > 0);

    // triangle channel
    state.triangle_period = m_triangle.timer_period;
    state.triangle_enable = (m_triangle.timer_period >= 2);

    // noise channel
    state.noise_period = m_noise.timer_period;
    state.noise_volume = (m_noise.len_counter > 0 ? m_noise.envelope.out : 0);
    state.noise_mode   = m_noise.mode;
    state.noise_enable = (state.noise_volume > 0);

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
//#if 1
//    float pulseVolFactor = 15.0f / std::max(m_maxVol[0], m_maxVol[1]);
//    float noiseVolFactor = 15.0f / m_maxVol[2];
//
//    for (FrameId id = 0; id < stream.framesCount(); ++id)
//    {
//        Frame& frame = const_cast<Frame&>(stream.GetFrame(id));
//
//        // chip 0
//        frame.data(0, A_Volume) = uint8_t(float(frame.data(0, A_Volume)) * pulseVolFactor);
//        frame.data(0, C_Volume) = uint8_t(float(frame.data(0, C_Volume)) * pulseVolFactor);
//
//        // chip 1
//        frame.data(1, A_Volume) = uint8_t(float(frame.data(1, A_Volume)) * pulseVolFactor);
//        frame.data(1, B_Volume) = uint8_t(float(frame.data(1, B_Volume)) * noiseVolFactor);
//        frame.data(1, C_Volume) = uint8_t(float(frame.data(1, C_Volume)) * pulseVolFactor);
//    }
//#else
//    int pulseVolDelta = (0x0F - std::max(m_maxVol[0], m_maxVol[1]));
//    int noiseVolDelta = (0x0F - m_maxVol[2]);
//
//    for (int i = 0, c = stream.frames.count(); i < c; ++i)
//    {
//        Frame& frame = const_cast<Frame&>(stream.frames.get(i));
//
//        // chip 0
//        if (frame.data(0, VolA_EnvFlg) < 0x0F)
//            frame.data(0, VolA_EnvFlg) += pulseVolDelta;
//
//        if (frame.data(0, VolC_EnvFlg) < 0x0F)
//            frame.data(0, VolC_EnvFlg) += pulseVolDelta;
//
//        // chip 1
//        if (frame.data(1, VolA_EnvFlg) < 0x0F)
//            frame.data(1, VolA_EnvFlg) += pulseVolDelta;
//
//        if (frame.data(1, VolB_EnvFlg) < 0x0F)
//            frame.data(1, VolB_EnvFlg) += noiseVolDelta;
//
//        if (frame.data(1, VolC_EnvFlg) < 0x0F)
//            frame.data(1, VolC_EnvFlg) += pulseVolDelta;
//    }
//#endif
}

void SimRP2A03_::ConvertToAY8910(const State& state, Frame& frame)
{
}

void SimRP2A03_::ConvertToAY8910x2(const State& state, Frame& frame)
{
    uint16_t a0_period = ConvertPeriod(state.pulse1_period);
    uint8_t  a0_volume = ConvertVolume(state.pulse1_volume);
    
    uint16_t c0_period = ConvertPeriod(state.pulse2_period);
    uint8_t  c0_volume = ConvertVolume(state.pulse2_volume);
    
    uint16_t e0_period = ConvertPeriod(state.triangle_period >> 2);
    if (!state.triangle_enable || !e0_period) e0_period = 0xFFFF;

    uint16_t n1_period = ConvertPeriod(state.noise_period >> 6);
    uint8_t  n1_volume = ConvertVolume(state.noise_volume);

    uint8_t mixer0 = 0x3F;
    uint8_t mixer1 = 0x3F;

    // square A
    if (state.pulse1_enable)
    {
        mixer0 &= ~(1 << 0);
        mixer1 &= ~(1 << 0);

        uint16_t a1_period = a0_period;
        if (state.pulse1_duty == 0) a1_period ^= 1;
        if (state.pulse1_duty == 1 || state.pulse1_duty == 3) a1_period >>= 1;
        uint8_t a1_volume = (a0_volume ? a0_volume - 1 : a0_volume);

        // chip 0 ch A
        frame.UpdatePeriod(0, A_Period, a0_period);
        frame.Update(0, A_Volume, a0_volume);

        // chip 1 ch A
        frame.UpdatePeriod(1, A_Period, a1_period);
        frame.Update(1, A_Volume, a1_volume);

        if (a0_volume > m_maxVol[0]) m_maxVol[0] = a0_volume;
    }

    // square C
    if (state.pulse2_enable)
    {
        mixer0 &= ~(1 << 2);
        mixer1 &= ~(1 << 2);

        uint16_t c1_period = c0_period;
        if (state.pulse2_duty == 0) c1_period ^= 1;
        if (state.pulse2_duty == 1 || state.pulse2_duty == 3) c1_period >>= 1;
        uint8_t  c1_volume = (c0_volume ? c0_volume - 1 : c0_volume);

        // chip 0 ch C
        frame.UpdatePeriod(0, C_Period, c0_period);
        frame.Update(0, C_Volume, c0_volume);

        // chip 1 ch C
        frame.UpdatePeriod(1, C_Period, c1_period);
        frame.Update(1, C_Volume, c1_volume);

        if (c0_volume > m_maxVol[1]) m_maxVol[1] = c0_volume;
    }

    // Envelope
    {
        // chip 0 ch B
        frame.Update(0, B_Volume, 0x10);
        frame.UpdatePeriod(0, E_Period, e0_period);
        if (frame.data(0, E_Shape) != 0x0E) frame.Update(0, E_Shape, 0x0E);
    }

    // Noise
    if (state.noise_enable)
    {
        mixer1 &= ~(1 << 4);

        // chip 1 ch B
        frame.UpdatePeriod(1, N_Period, n1_period);
        frame.Update(1, B_Volume, n1_volume);

        if (n1_volume > m_maxVol[2]) m_maxVol[2] = n1_volume;
    }
    
    // Mixers
    {
        frame.Update(0, Mixer, mixer0);
        frame.Update(1, Mixer, mixer1);
    }
}

void SimRP2A03_::ConvertToAY8930(const State& state, Frame& frame)
{
}

uint16_t SimRP2A03_::ConvertPeriod(uint16_t period) const
{
#if 0
    return period;
#else
    double srcClockPeriod = 1.0 / double(m_cpuClock);
    double dstClockPeriod = 1.0 / double(m_dstClockRate);
    double result = double(period) * srcClockPeriod / dstClockPeriod;
    return uint16_t(result + 0.5);
#endif
}

uint8_t SimRP2A03_::ConvertVolume(uint8_t volume) const
{
    if (volume > 0)
    {
        auto factor = float(volume - 1) / 14.f;
        return uint8_t(5.5f + 10.f * factor);
    }
    return 0;
}
