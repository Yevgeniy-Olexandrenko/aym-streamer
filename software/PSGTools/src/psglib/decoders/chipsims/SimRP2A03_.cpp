#include "SimRP2A03_.h"
#include "stream/Frame.h"
#include "stream/Stream.h"

SimRP2A03_::SimRP2A03_()
    : ChipSim(Type::RP2A03)
    , NesApu(44100, NesCpu::Clock::NTSC)
{
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
    uint8_t pulse1;
    uint8_t pulse2;
    uint8_t triangle;
    uint8_t noise;

    if (!m_pulse1.len_counter || m_pulse1.sweep.silence)
        pulse1 = 0;
    else
        pulse1 = m_pulse1.envelope.out;

    if (!m_pulse2.len_counter || m_pulse2.sweep.silence)
        pulse2 = 0;
    else
        pulse2 = m_pulse2.envelope.out;

    if (m_triangle.timer_period < 2)
        triangle = 0;
    else
        triangle = 1;

    noise = m_noise.envelope.out * (m_noise.len_counter > 0);

    auto ProcessVolume = [](uint8_t& volume)
    {
        if (volume > 0)
        {
            float factor = float(volume - 1) / 14.f;
            volume = uint8_t(10 * factor + 5);
        }
    };

    // process
    uint8_t  a1_volume = pulse1;
    uint16_t a1_period = m_pulse1.timer_period;
    uint8_t  a1_enable = bool(pulse1 > 0);
    uint8_t  a1_mode = m_pulse1.duty;

    uint8_t  c2_volume = pulse2;
    uint16_t c2_period = m_pulse2.timer_period;
    uint8_t  c2_enable = bool(pulse2 > 0);
    uint8_t  c2_mode = m_pulse2.duty;

    uint16_t bt_period = m_triangle.timer_period >> 2;
    uint8_t  bt_enable = bool(triangle > 0);
    if (!bt_enable || !bt_period) bt_period = 0xFFFF;

    uint8_t  bn_volume = noise;
    uint16_t bn_period = m_noise.timer_period >> 6;
    uint8_t  bn_enable = bool(noise > 0);
    if (bn_volume > 0x0F) bn_volume = 0x0F;

    ProcessVolume(a1_volume);
    ProcessVolume(c2_volume);
    ProcessVolume(bn_volume);

    // output
    uint8_t mixer1 = 0x3F;
    uint8_t mixer2 = 0x3F;

    if (a1_enable)
    {
        uint16_t a1_period_m = a1_period;
        if (a1_mode == 0) a1_period_m ^= 1;
        if (a1_mode == 1 || a1_mode == 3) a1_period_m >>= 1;
        uint8_t  a1_volume_m = (a1_volume ? a1_volume - 1 : a1_volume);

        // chip 0 ch A
        frame.Update(0, A_Fine, a1_period & 0xFF);
        frame.Update(0, A_Coarse, a1_period >> 8 & 0x0F);
        frame.Update(0, A_Volume, a1_volume);

        // chip 1 ch A
        frame.Update(1, A_Fine, a1_period_m & 0xFF);
        frame.Update(1, A_Coarse, a1_period_m >> 8 & 0x0F);
        frame.Update(1, A_Volume, a1_volume_m);

        if (a1_volume > m_maxVol[0]) m_maxVol[0] = a1_volume;
    }
    mixer1 &= ~(a1_enable << 0);
    mixer2 &= ~(a1_enable << 0);

    if (c2_enable)
    {
        uint16_t c2_period_m = c2_period;
        if (c2_mode == 0) c2_period_m ^= 1;
        if (c2_mode == 1 || c2_mode == 3) c2_period_m >>= 1;
        uint8_t  c2_volume_m = (c2_volume ? c2_volume - 1 : c2_volume);

        // chip 0 ch C
        frame.Update(0, C_Fine, c2_period & 0xFF);
        frame.Update(0, C_Coarse, c2_period >> 8 & 0x0F);
        frame.Update(0, C_Volume, c2_volume);

        // chip 1 ch C
        frame.Update(1, C_Fine, c2_period_m & 0xFF);
        frame.Update(1, C_Coarse, c2_period_m >> 8 & 0x0F);
        frame.Update(1, C_Volume, c2_volume_m);

        if (c2_volume > m_maxVol[1]) m_maxVol[1] = c2_volume;
    }
    mixer1 &= ~(c2_enable << 2);
    mixer2 &= ~(c2_enable << 2);

    // chip 0 ch B
    frame.Update(0, E_Fine, bt_period & 0xFF);
    frame.Update(0, E_Coarse, bt_period >> 8 & 0xFF);
    frame.Update(0, B_Volume, 0x10);
    if (frame.data(0, E_Shape) != 0x0E)
    {
        frame.Update(0, E_Shape, 0x0E);
    }

    if (bn_enable)
    {
        // chip 1 ch B
        frame.Update(1, N_Period, bn_period & 0x1F);
        frame.Update(1, B_Volume, bn_volume);

        if (bn_volume > m_maxVol[2]) m_maxVol[2] = bn_volume;
    }
    mixer2 &= ~(bn_enable << 4);

    // mixers
    frame.Update(0, Mixer, mixer1);
    frame.Update(1, Mixer, mixer2);
}

void SimRP2A03_::PostProcess(Stream& stream)
{
#if 1
    float pulseVolFactor = 15.0f / std::max(m_maxVol[0], m_maxVol[1]);
    float noiseVolFactor = 15.0f / m_maxVol[2];

    for (FrameId id = 0; id < stream.framesCount(); ++id)
    {
        Frame& frame = const_cast<Frame&>(stream.GetFrame(id));

        // chip 0
        frame.data(0, A_Volume) = uint8_t(float(frame.data(0, A_Volume)) * pulseVolFactor);
        frame.data(0, C_Volume) = uint8_t(float(frame.data(0, C_Volume)) * pulseVolFactor);

        // chip 1
        frame.data(1, A_Volume) = uint8_t(float(frame.data(1, A_Volume)) * pulseVolFactor);
        frame.data(1, B_Volume) = uint8_t(float(frame.data(1, B_Volume)) * noiseVolFactor);
        frame.data(1, C_Volume) = uint8_t(float(frame.data(1, C_Volume)) * pulseVolFactor);
    }
#else
    int pulseVolDelta = (0x0F - std::max(m_maxVol[0], m_maxVol[1]));
    int noiseVolDelta = (0x0F - m_maxVol[2]);

    for (int i = 0, c = stream.frames.count(); i < c; ++i)
    {
        Frame& frame = const_cast<Frame&>(stream.frames.get(i));

        // chip 0
        if (frame.data(0, VolA_EnvFlg) < 0x0F)
            frame.data(0, VolA_EnvFlg) += pulseVolDelta;

        if (frame.data(0, VolC_EnvFlg) < 0x0F)
            frame.data(0, VolC_EnvFlg) += pulseVolDelta;

        // chip 1
        if (frame.data(1, VolA_EnvFlg) < 0x0F)
            frame.data(1, VolA_EnvFlg) += pulseVolDelta;

        if (frame.data(1, VolB_EnvFlg) < 0x0F)
            frame.data(1, VolB_EnvFlg) += noiseVolDelta;

        if (frame.data(1, VolC_EnvFlg) < 0x0F)
            frame.data(1, VolC_EnvFlg) += pulseVolDelta;
    }
#endif
}
