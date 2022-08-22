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
    uint8_t mixer = 0x3F;

    // Pulse 1 -> Square A
    if (state.pulse1_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse1_period);
        uint8_t  volume = ConvertVolume(state.pulse1_volume);

        frame.UpdatePeriod(A_Period, period);
        frame.Update(A_Volume, volume);
        mixer &= ~(1 << 0);

        if (volume > m_maxVol[0]) m_maxVol[0] = volume;
    }

    // Pulse 2 -> Square C
    if (state.pulse2_enable)
    {
        uint16_t period = ConvertPeriod(state.pulse2_period);
        uint8_t  volume = ConvertVolume(state.pulse2_volume);

        frame.UpdatePeriod(C_Period, period);
        frame.Update(C_Volume, volume);
        mixer &= ~(1 << 2);

        if (volume > m_maxVol[1]) m_maxVol[1] = volume;
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 2);

        if (frame.data(0, E_Shape) != 0x0E) frame.Update(E_Shape, 0x0E);
        frame.UpdatePeriod(E_Period, period);
        frame.Update(B_Volume, 0x10);
    }
    else
    {
        frame.UpdatePeriod(E_Period, 0xFFFF);
    }

    // Noise -> automatically chosen channel
    if (state.noise_enable)
    {
        uint16_t period = ConvertPeriod(state.noise_period >> 6);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        frame.UpdatePeriod(N_Period, period);

        // choose Square B if Triangle is disabled
        if (!state.triangle_enable)
        {
            frame.Update(B_Volume, volume);
            mixer &= ~(1 << 4);
        }

        // choose Square A if Pulse 1 is disabled
        else if (!state.pulse1_enable)
        {
            frame.Update(A_Volume, volume);
            mixer &= ~(1 << 3);
        }

        // choose Square C if Pulse 2 is disabled
        else if (!state.pulse2_enable)
        {
            frame.Update(C_Volume, volume);
            mixer &= ~(1 << 5);
        }

        // choose Square A if Noise volume is very close
        else if (std::abs(int(frame.data(0, A_Volume)) - int(volume)) < 2)
        {
            mixer &= ~(1 << 3);
        }

        // choose Square C if Noise volume is very close
        else if (std::abs(int(frame.data(0, C_Volume)) - int(volume)) < 2)
        {
            mixer &= ~(1 << 5);
        }

        // choose Square B ignoring Triangle
        else
        {
            frame.Update(B_Volume, volume);
            mixer &= ~(1 << 4);
        }

        if (volume > m_maxVol[2]) m_maxVol[2] = volume;
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
        uint8_t  volume0 = ConvertVolume(state.pulse1_volume);

        uint16_t period1 = period0;
        if (state.pulse1_duty == 0) period1 ^= 1;
        if (state.pulse1_duty == 1 || state.pulse1_duty == 3) period1 >>= 1;
        uint8_t volume1 = (volume0 ? volume0 - 1 : volume0);

        frame.UpdatePeriod(0, A_Period, period0);
        frame.Update(0, A_Volume, volume0);
        mixer0 &= ~(1 << 0);

        frame.UpdatePeriod(1, A_Period, period1);
        frame.Update(1, A_Volume, volume1);
        mixer1 &= ~(1 << 0);

        if (volume0 > m_maxVol[0]) m_maxVol[0] = volume0;
    }

    // Pulse 2 -> Chip 0 Square C + Chip 1 Square C
    if (state.pulse2_enable)
    {
        uint16_t period0 = ConvertPeriod(state.pulse2_period);
        uint8_t  volume0 = ConvertVolume(state.pulse2_volume);

        uint16_t period1 = period0;
        if (state.pulse2_duty == 0) period1 ^= 1;
        if (state.pulse2_duty == 1 || state.pulse2_duty == 3) period1 >>= 1;
        uint8_t  volume1 = (volume0 ? volume0 - 1 : volume0);

        frame.UpdatePeriod(0, C_Period, period0);
        frame.Update(0, C_Volume, volume0);
        mixer0 &= ~(1 << 2);

        frame.UpdatePeriod(1, C_Period, period1);
        frame.Update(1, C_Volume, volume1);
        mixer1 &= ~(1 << 2);

        if (volume0 > m_maxVol[1]) m_maxVol[1] = volume0;
    }

    // Triangle -> Chip 0 Envelope in Channel B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 2);

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
        uint16_t period = ConvertPeriod(state.noise_period >> 6);
        uint8_t  volume = ConvertVolume(state.noise_volume);

        frame.UpdatePeriod(1, N_Period, period);
        frame.Update(1, B_Volume, volume);
        mixer1 &= ~(1 << 4);

        if (volume > m_maxVol[2]) m_maxVol[2] = volume;
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

        if (volume > m_maxVol[0]) m_maxVol[0] = volume;
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

        if (volume > m_maxVol[1]) m_maxVol[1] = volume;
    }

    // Triangle -> Envelope B
    if (state.triangle_enable)
    {
        uint16_t period = ConvertPeriod(state.triangle_period >> 2);

        // workaround for mixer
        frame.UpdatePeriod(B_Period, 0x0000);
        frame.Update(B_Duty, 0x08);
        mixer &= ~(1 << 1);

        // turn on triangle waveform
        if (frame.data(0, EB_Shape) != 0x0E) frame.Update(EB_Shape, 0x0E);
        frame.UpdatePeriod(EB_Period, period);
        frame.Update(B_Volume, 0x20);
    }
    
    // Noise
    // TODO

    // Mixers
    {
        frame.Update(Mixer, mixer);
    }
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
        if (m_convertMethod == ConvertMethod::AY8930)
            return uint8_t(11.5f + 20.f * factor);
        else
            return uint8_t(5.5f + 10.f * factor);
    }
    return 0;
}
