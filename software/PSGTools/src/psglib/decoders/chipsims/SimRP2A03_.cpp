#include "SimRP2A03_.h"

#include <string>
#include <initializer_list>

#define BIT(v, b) (((v>>b)&1) == 1)

const uint8_t pulseseq[4][8] =
{
    { 0, 1, 0, 0, 0, 0, 0, 0 },  // 12.5%
    { 0, 1, 1, 0, 0, 0, 0, 0 },  // 25.0%
    { 0, 1, 1, 1, 1, 0, 0, 0 },  // 50.0%
    { 1, 0, 0, 1, 1, 1, 1, 1 }   // 25.0% negated
};

const uint8_t triseq[32] =
{
    0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

const uint16_t noise_periods_ntsc[16] =
{
     4, 8, 16, 32, 64, 96, 128, 160,
     202, 254, 380, 508, 762, 1016, 2034, 4068
};

const uint16_t noise_periods_pal[16] =
{
     4, 7, 14, 30, 60, 88, 118, 148,
     188, 236, 354, 472, 708,  944, 1890, 3778
};

const uint16_t dmc_periods_ntsc[16] =
{
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106,  84,  72,  54
};

const uint16_t dmc_periods_pal[16] =
{
    398, 354, 316, 298, 276, 236, 210, 198,
    176, 148, 132, 118,  98,  78,  66,  50
};

const uint8_t length_lut[32] =
{
    10, 254, 20,  2, 40,  4, 80,  6,
    160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22,
    192, 24, 72, 26, 16, 28, 32, 30
};

////////////////////////////////////////////////////////////////////////////////

RP2A03::RP2A03(int samplerate, uint8_t clockstandard)
{
    if (clockstandard == APU_PAL)
    {
        m_cpuClock = CPU_CLOCK_PAL;
        m_noisePeriods = noise_periods_pal;
        m_dmcPeriods = dmc_periods_pal;
    }
    else
    {
        m_cpuClock = CPU_CLOCK_NTSC;
        m_noisePeriods = noise_periods_ntsc;
        m_dmcPeriods = dmc_periods_ntsc;
    }

    m_cpuCyclesPerSample = (uint32_t)((((uint64_t)m_cpuClock) << 16) / samplerate);

    for (int n = 0; n < 31; ++n)
    {
        m_P12MixLut[n] = (uint32_t)((95.52 / (8128.0 / (double)n + 100.0)) * 0xFFFFFFFFUL);
    }

    for (int n = 0; n < 203; ++n)
    {
        m_TNDMixLut[n] = (uint32_t)((163.67 / (24329.0 / (double)n + 100.0)) * 0xFFFFFFFFUL);
    }
}

void RP2A03::Reset()
{
    memset(&m_pulse1, 0, sizeof(m_pulse1));
    memset(&m_pulse2, 0, sizeof(m_pulse2));
    memset(&m_triangle, 0, sizeof(m_triangle));
    memset(&m_noise, 0, sizeof(m_noise));
    memset(&m_dmc, 0, sizeof(m_dmc));
    memset(&m_frame, 0, sizeof(m_frame));
    m_cpuCycles = 0;

    for (int i = 0x00; i <= 0x13; ++i) Write(i, 0x00);
    Write(STATUS, 0x0f);
    Write(FRAMECNTR, 0x40);
    m_noise.shiftreg = 1;
}

void RP2A03::Write(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case PULSE1DUTYVOL:
        m_pulse1.duty = (data >> 6);
        m_pulse1.env.loop_halt = BIT(data, 5);
        m_pulse1.env.const_vol = BIT(data, 4);
        m_pulse1.env.vol_period = (data & 0xF);
        break;

    case PULSE1SWEEP:
        m_pulse1.sweep_enable = BIT(data, 7);
        m_pulse1.sweep_period = ((data >> 4) & 0x07);
        m_pulse1.sweep_negate = BIT(data, 3);
        m_pulse1.sweep_shift = (data & 0x07);
        m_pulse1.sweep_reload = true;
        break;

    case PULSE1TMRL:
        m_pulse1.timer_period = ((m_pulse1.timer_period & 0x0700) | data);
        CalculateSweepPulse1();
        break;

    case PULSE1TMRH:
        m_pulse1.timer_period = ((m_pulse1.timer_period & 0x00FF) | ((data & 0x07) << 8));
        CalculateSweepPulse1();
        if (m_pulse1.enabled) m_pulse1.counter = length_lut[data >> 3];
        m_pulse1.phase = 0;
        m_pulse1.env.start = 1;
        break;

    case PULSE2DUTYVOL:
        m_pulse2.duty = (data >> 6);
        m_pulse2.env.loop_halt = BIT(data, 5);
        m_pulse2.env.const_vol = BIT(data, 4);
        m_pulse2.env.vol_period = (data & 0xF);
        break;

    case PULSE2SWEEP:
        m_pulse2.sweep_enable = BIT(data, 7);
        m_pulse2.sweep_period = ((data >> 4) & 0x07);
        m_pulse2.sweep_negate = BIT(data, 3);
        m_pulse2.sweep_shift = (data & 0x07);
        m_pulse2.sweep_reload = true;
        break;

    case PULSE2TMRL:
        m_pulse2.timer_period = ((m_pulse2.timer_period & 0x0700) | data);
        CalculateSweepPulse2();
        break;

    case PULSE2TMRH:
        m_pulse2.timer_period = ((m_pulse2.timer_period & 0x00FF) | ((data & 0x07) << 8));
        CalculateSweepPulse2();
        if (m_pulse2.enabled) m_pulse2.counter = length_lut[data >> 3];
        m_pulse2.phase = 0;
        m_pulse2.env.start = 1;
        break;

    case TRICOUNTER:
        m_triangle.control = BIT(data, 7);
        if (m_triangle.control) m_triangle.halt = 1;
        m_triangle.cnt_reload = (data & 0x7f);
        break;

    case TRITMRL:
        m_triangle.timer_period = ((m_triangle.timer_period & 0x0700) | data);
        break;

    case TRITMRH:
        m_triangle.timer_period = ((m_triangle.timer_period & 0x00FF) | ((data & 0x07) << 8));
        if (m_triangle.enabled) m_triangle.counter = length_lut[data >> 3];
        m_triangle.halt = 1;
        break;

    case NOISEVOL:
        m_noise.env.loop_halt = BIT(data, 5);
        m_noise.env.const_vol = BIT(data, 4);
        m_noise.env.vol_period = (data & 0x0F);
        break;

    case NOISEPERIOD:
        m_noise.mode = BIT(data, 7);
        m_noise.period = (data & 0x0F);
        m_noise.period_actual = m_noisePeriods[m_noise.period];
        break;

    case NOISELCL:
        if (m_noise.enabled) m_noise.counter = length_lut[data >> 3];
        m_noise.env.start = 1;
        break;

    case DMCIRQ:
        m_dmc.irq = BIT(data, 7);
        m_dmc.loop = BIT(data, 6);
        m_dmc.rate = (data & 0x0f);
        m_dmc.rate_actual = m_dmcPeriods[m_dmc.rate];
        break;

    case DMCCOUNTER:
        m_dmc.counter = (data & 0x7f);
        break;

    case DMCADDR:
        m_dmc.address = ((data << 6) | 0xC000);
        m_dmc.addresscur = m_dmc.address;
        break;

    case DMCLENGTH:
        m_dmc.length = ((data << 4) | 1);
        m_dmc.bytesleft = m_dmc.length;
        break;

    case STATUS:
        m_dmc.control = BIT(data, 4);
        m_noise.enabled = BIT(data, 3);
        m_triangle.enabled = BIT(data, 2);
        m_pulse2.enabled = BIT(data, 1);
        m_pulse1.enabled = BIT(data, 0);
        break;

    case FRAMECNTR:
        m_frame.count = 0;
        m_frame.mode = BIT(data, 7);
        m_frame.int_inhibit = BIT(data, 6);
        m_frame.updated = true;
        break;
    }
}

uint8_t RP2A03::Read(uint16_t addr)
{
    if (addr == STATUS)
    {
        return (m_dmc.irq << 7
            | m_frame.interrupt << 6
            | (m_noise.counter > 0) << 3
            | (m_triangle.counter > 0) << 2
            | (m_pulse2.counter > 0) << 1
            | (m_pulse1.counter > 0));
    }
    return 0;
}

int32_t RP2A03::Output()
{
    m_cpuCycles += m_cpuCyclesPerSample;

    Process(m_cpuCycles >> 16);
    m_cpuCycles &= 0xFFFF;

    uint8_t pulse1;
    uint8_t pulse2;
    uint8_t triangle;
    uint8_t noise;
    uint8_t dmc;

    if (!m_pulse1.counter || m_pulse1.sweep_silence)
        pulse1 = 0;
    else
    {
        pulse1 = pulseseq[m_pulse1.duty][m_pulse1.phase] * m_pulse1.env.out * (m_pulse1.counter > 0);
    }

    if (!m_pulse2.counter || m_pulse2.sweep_silence)
        pulse2 = 0;
    else
    {
        pulse2 = pulseseq[m_pulse2.duty][m_pulse2.phase] * m_pulse2.env.out * (m_pulse2.counter > 0);
    }

    if (m_triangle.timer_period < 2)
        triangle = 0x7;
    else
        triangle = triseq[m_triangle.phase];

    noise = (m_noise.shiftreg & 1) * m_noise.env.out * (m_noise.counter > 0);
    dmc = m_dmc.counter;

    // "tri + tri<<1" (tri + tri*2 == tri*3) + noise<<1 (noise*2) + dmc
    return (int32_t)((int64_t)(m_P12MixLut[pulse1 + pulse2] + m_TNDMixLut[(triangle + (triangle << 1)) + (noise << 1) + dmc]) - 0x7fffffffL);
}

void RP2A03::Process(uint32_t cpu_cycles)
{
    for (uint32_t cycle = 0; cycle < cpu_cycles; ++cycle)
    {
        if ((cycle & 1) || m_frame.updated)
        {
            // clock frame counter
            if (m_frame.mode)
            {
                // 5 step sequence
                if (m_frame.updated || (m_frame.count == 7456) || (m_frame.count == 18640))
                {
                    // "quarter frame" and "half frame"
                    UpdateQuarterFrame();
                    UpdateHalfFrame();
                    m_frame.updated = false;
                }
                else if ((m_frame.count == 3728) || (m_frame.count == 11185))
                {
                    // "quarter frame"
                    UpdateQuarterFrame();
                }

                if (m_frame.count == 18640) m_frame.count = 0;
                else ++m_frame.count;
            }
            else
            {
                // 4 step sequence
                if (m_frame.updated || (m_frame.count == 7456) || (m_frame.count == 14914))
                {
                    // "quarter frame" and "half frame"
                    UpdateQuarterFrame();
                    UpdateHalfFrame();
                    m_frame.updated = false;
                }
                else if ((m_frame.count == 3728) || (m_frame.count == 11185))
                {
                    // "quarter frame"
                    UpdateQuarterFrame();
                }

                if (m_frame.count == 14914)
                {
                    m_frame.count = 0;
                    if (!m_frame.int_inhibit)
                    {
                        m_frame.interrupt = 1;
                        // Int6502(&cpu, INT_IRQ);
                        // Run6502(&cpu);
                    }
                }
                else ++m_frame.count;
            }
        }

        if (cycle & 1)
        {
            // process timers for all waves
            if (m_pulse1.timer) --m_pulse1.timer;
            else
            {
                m_pulse1.timer = m_pulse1.timer_period;
                if (m_pulse1.phase) --m_pulse1.phase;
                else m_pulse1.phase = 7;
            }

            if (m_pulse2.timer) --m_pulse2.timer;
            else
            {
                m_pulse2.timer = m_pulse2.timer_period;
                if (m_pulse2.phase) --m_pulse2.phase;
                else m_pulse2.phase = 7;
            }

            if (m_noise.timer) --m_noise.timer;
            else
            {
                m_noise.timer = m_noise.period_actual;
                UpdateNoise();
            }
        }

        // DMC shite
        if (m_dmc.control)
        {
            if (!m_dmc.buffered && m_dmc.bytesleft)
            {
                m_dmc.sample = CpuRead(m_dmc.addresscur);

                if (!(--m_dmc.bytesleft))
                {
                    if (m_dmc.loop)
                    {
                        m_dmc.addresscur = m_dmc.address;
                        m_dmc.bytesleft = m_dmc.length;
                    }
                    else
                        m_dmc.irq = true;
                }
                else if (m_dmc.addresscur == 0xFFFF)
                    m_dmc.addresscur = 0x8000;
                else
                    ++m_dmc.addresscur;
                m_dmc.buffered = true;
            }
        }

        if (m_dmc.timer) --m_dmc.timer;
        else
        {
            m_dmc.timer = m_dmc.rate_actual;

            if (!m_dmc.bitsleft)
            {
                m_dmc.bitsleft = 8;
                if (!m_dmc.buffered) m_dmc.silence = true;
                else
                {
                    m_dmc.silence = false;
                    m_dmc.shiftreg = m_dmc.sample;
                    m_dmc.buffered = false;
                }
            }

            if (!m_dmc.silence)
            {
                if ((m_dmc.counter > 1) && !(m_dmc.shiftreg & 1))
                    m_dmc.counter -= 2;
                else if ((m_dmc.counter < 126) && (m_dmc.shiftreg & 1))
                    m_dmc.counter += 2;
            }

            m_dmc.shiftreg >>= 1;
            --m_dmc.bitsleft;
        }

        // clocked on every cycle
        if ((m_triangle.lincount > 0) && (m_triangle.counter > 0))
        {
            if (m_triangle.timer) --m_triangle.timer;
            else
            {
                m_triangle.timer = m_triangle.timer_period;
                if (m_triangle.phase) --m_triangle.phase;
                else m_triangle.phase = 31;
            }
        }
    }
}

void RP2A03::CalculateSweepPulse1()
{
    m_pulse1.sweep_target = m_pulse1.sweep_negate
        ? m_pulse1.timer_period - ((m_pulse1.timer_period >> m_pulse1.sweep_shift) - 1)
        : m_pulse1.timer_period + (m_pulse1.timer_period >> m_pulse1.sweep_shift);
    m_pulse1.sweep_silence = ((m_pulse1.timer_period < 8) || (m_pulse1.sweep_target > 0x7FF));
}

void RP2A03::CalculateSweepPulse2()
{
    m_pulse2.sweep_target = m_pulse2.sweep_negate
        ? m_pulse2.timer_period - (m_pulse2.timer_period >> m_pulse2.sweep_shift)
        : m_pulse2.timer_period + (m_pulse2.timer_period >> m_pulse2.sweep_shift);
    m_pulse2.sweep_silence = ((m_pulse2.timer_period < 8) || (m_pulse2.sweep_target > 0x7FF));
}

void RP2A03::UpdateQuarterFrame()
{
    // process envelopes (pulses and noise)
    for (Envelope* env : { &m_pulse1.env, &m_pulse2.env, &m_noise.env })
    {
        if (env->start)
        {
            env->start = false;
            env->counter = 0xF;
            env->divider = (env->vol_period + 1);
        }
        else
        {
            if (!(--env->divider))
            {
                if (env->counter) --env->counter;
                else if (env->loop_halt) env->counter = 0xF;
                env->divider = (env->vol_period + 1);
            }
        }
        env->out = env->const_vol ? env->vol_period : env->counter;
    }

    // process triangle's linear counter
    if (m_triangle.halt) m_triangle.lincount = m_triangle.cnt_reload;
    else if (m_triangle.lincount) --m_triangle.lincount;
    if (!m_triangle.control) m_triangle.halt = 0;
}

void RP2A03::UpdateHalfFrame()
{
    // process length counters of pulses, triangle, noise, and DMC
    if (m_pulse1.enabled)
    {
        if (m_pulse1.counter && (!m_pulse1.env.loop_halt)) --m_pulse1.counter;
    }
    else m_pulse1.counter = 0;

    if (m_pulse2.enabled)
    {
        if (m_pulse2.counter && (!m_pulse2.env.loop_halt)) --m_pulse2.counter;
    }
    else m_pulse2.counter = 0;

    if (m_triangle.enabled)
    {
        if (m_triangle.counter && (!m_triangle.halt)) --m_triangle.counter;
    }
    else m_triangle.counter = 0;

    if (m_noise.enabled)
    {
        if (m_noise.counter && (!m_noise.env.loop_halt)) --m_noise.counter;
    }
    else m_noise.counter = 0;

    if (m_pulse1.sweep_divider)
    {
        // sweep units
        --m_pulse1.sweep_divider;
        if (m_pulse1.sweep_reload)
        {
            m_pulse1.sweep_reload = false;
            m_pulse1.sweep_divider = (m_pulse1.sweep_period + 1);
        }
    }
    else
    {
        if (m_pulse1.sweep_enable && m_pulse1.sweep_shift)
        {
            m_pulse1.sweep_divider = (m_pulse1.sweep_period + 1);
            m_pulse1.timer_period = m_pulse1.sweep_target;
            CalculateSweepPulse1();
        }
    }

    if (m_pulse2.sweep_divider)
    {
        // sweep units
        --m_pulse2.sweep_divider;
        if (m_pulse2.sweep_reload)
        {
            m_pulse2.sweep_reload = false;
            m_pulse2.sweep_divider = (m_pulse2.sweep_period + 1);
        }
    }
    else
    {
        if (m_pulse2.sweep_enable && m_pulse2.sweep_shift)
        {
            m_pulse2.sweep_divider = (m_pulse2.sweep_period + 1);
            m_pulse2.timer_period = m_pulse2.sweep_target;
            CalculateSweepPulse2();
        }
    }
}

void RP2A03::UpdateNoise()
{
    uint16_t feedback = m_noise.mode
        ? ((m_noise.shiftreg & 1) ^ ((m_noise.shiftreg >> 5) & 1))
        : ((m_noise.shiftreg & 1) ^ ((m_noise.shiftreg >> 1) & 1)) << 14;
    m_noise.shiftreg = (m_noise.shiftreg >> 1) | feedback;
}

uint8_t RP2A03::CpuRead(uint16_t addr)
{
    // TODO

    return 0x00;
}

////////////////////////////////////////////////////////////////////////////////

SimRP2A03_::SimRP2A03_(int samplerate, uint8_t clockstandard)
    : ChipSim(Type::RP2A03)
    ,RP2A03(samplerate, clockstandard)
{
}

void SimRP2A03_::Reset()
{
    RP2A03::Reset();
}

void SimRP2A03_::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
    if (chip == 0)
    {
        RP2A03::Write(0x4000 | reg, data);
    }
}

void SimRP2A03_::Simulate(int samples)
{
    // TODO
}

void SimRP2A03_::ConvertToPSG(Frame& frame)
{
    // TODO
}

void SimRP2A03_::PostProcess(Stream& stream)
{
    // TODO
}
