#include "SimRP2A03.h"
#include "stream/Frame.h"
#include "stream/Stream.h"

namespace
{
    constexpr uint8_t lengthLut[] =
    {
        10, 254, 20, 2, 40, 4, 80, 6,
        160, 8, 60, 10, 14, 12, 26, 14,
        12, 16, 24, 18, 48, 20, 96, 22,
        192, 24, 72, 26, 16, 28, 32, 30,
    };

    constexpr uint16_t noiseLut[] =
    {
        0x002, 0x004, 0x008, 0x010,
        0x020, 0x030, 0x040, 0x050,
        0x065, 0x07F, 0x0BE, 0x0FE,
        0x17D, 0x1FC, 0x3F9, 0x7F2,
    };

    constexpr uint32_t noise_freq_table[] =
    {
        0x002, 0x004, 0x008, 0x010,
        0x020, 0x030, 0x040, 0x050,
        0x065, 0x07F, 0x0BE, 0x0FE,
        0x17D, 0x1FC, 0x3F9, 0x7F2
    };

    enum
    {
        APU_RECT_VOL1         = 0x00,  // 4000
        APU_SWEEP1            = 0x01,  // 4001
        APU_RECT_FREQ1        = 0x02,  // 4002
        APU_RECT_LEN1         = 0x03,  // 4003

        APU_RECT_VOL2         = 0x04,  // 4004
        APU_SWEEP2            = 0x05,  // 4005
        APU_RECT_FREQ2        = 0x06,  // 4006
        APU_RECT_LEN2         = 0x07,  // 4007

        APU_TRIANGLE          = 0x08,  // 4008
        // NOT USED 0x09
        APU_TRI_FREQ          = 0x0A,  // 400A
        APU_TRI_LEN           = 0x0B,  // 400B

        APU_NOISE_VOL         = 0x0C,  // 400C
        // NOT USED 0x0D
        APU_NOISE_FREQ        = 0x0E,  // 400E
        APU_NOISE_LEN         = 0x0F,  // 400F

        APU_DMC_DMA_FREQ      = 0x10,  // 4010
        APU_DMC_DELTA_COUNTER = 0x11,  // 4011
        APU_DMC_ADDR          = 0x12,  // 4012
        APU_DMC_LEN           = 0x13,  // 4013
        APU_STATUS            = 0x15,  // 4014

        APU_LOW_TIMER         = 0x17,  // 4017
    };

    constexpr uint32_t counterScaler = ((NES_CPU_FREQUENCY << CONST_SHIFT_BITS) / SAMPLING_RATE);
    constexpr uint32_t frameCounterPeriod = ((NES_CPU_FREQUENCY << CONST_SHIFT_BITS) / 240);
}

#define VALUE_VOL_MASK      (0x0F)
#define FIXED_VOL_MASK      (0x10)
#define DISABLE_LEN_MASK    (0x20)
#define ENABLE_LOOP_MASK    (0x20)
#define DUTY_CYCLE_MASK     (0xC0)

#define SWEEP_ENABLE_MASK   (0x80)
#define SWEEP_SHIFT_MASK    (0x07)
#define SWEEP_DIR_MASK      (0x08)
#define SWEEP_RATE_MASK     (0x70)

#define PAL_MODE_MASK       (0x80)

#define CHAN1_ENABLE_MASK   (0x01)
#define CHAN2_ENABLE_MASK   (0x02)
#define TRI_ENABLE_MASK     (0x04)

#define NOISE_FREQ_MASK     (0x0F)
#define NOISE_MODE_MASK     (0x80)

#define DMC_ENABLE_MASK     (0x10)
#define DMC_RATE_MASK       (0x0F)
#define DMC_LOOP_MASK       (0x40)
#define DMC_IRQ_ENABLE_MASK (0x80)

void SimRP2A03::updateFrameCounter()
{
    const uint8_t upperThreshold = (m_regs[APU_LOW_TIMER] & PAL_MODE_MASK) ? 5 : 4;

    m_quaterSignal = false;
    m_halfSignal = false;
    m_fullSignal = false;

    m_lastFrameCounter += counterScaler;
    if (m_lastFrameCounter >= frameCounterPeriod)
    {
        m_lastFrameCounter -= frameCounterPeriod;
        m_apuFrames++;
        if (m_apuFrames != 4 || upperThreshold != 5)
        {
            m_quaterSignal = true;
        }
        m_halfSignal = (m_apuFrames == 2) || (m_apuFrames >= upperThreshold);
        if (m_apuFrames >= upperThreshold)
        {
            m_fullSignal = true;
            m_apuFrames = 0;
        }
    }
}

void SimRP2A03::updateRectChannel(int i)
{
    ChannelInfo& chan = m_chan[i];
    //static constexpr uint8_t sequencerTable[] =
    //{
    //    0b01000000,
    //    0b01100000,
    //    0b01111000,
    //    0b10011111,
    //};

    if (!(m_regs[APU_STATUS] & (1 << i)))
    {
        chan.volume = 0;
        chan.output = chan.volume;
        return;
    }

    uint8_t volumeReg = m_regs[APU_RECT_VOL1 + i * 4];

    // Decay counters are always enabled
    // So, run the envelope anyway
    if (m_quaterSignal)
    {
        if (chan.updateEnvelope)
        {
            chan.decayCounter = 0x0F;
            chan.divider = volumeReg & VALUE_VOL_MASK;
            chan.updateEnvelope = false;
        }
        else if (chan.divider)
        {
            chan.divider--;
        }
        else
        {
            chan.divider = volumeReg & VALUE_VOL_MASK;
            if (chan.decayCounter)
            {
                chan.decayCounter--;
            }
            else if (volumeReg & ENABLE_LOOP_MASK)
            {
                chan.decayCounter = 0x0F;
            }
        }
    }
    if (volumeReg & FIXED_VOL_MASK) // fixed volume
    {
        chan.volume = volumeReg & VALUE_VOL_MASK;
    }
    else
    {
        chan.volume = chan.decayCounter;
    }
    // Countdown len counter if enabled
    if (!(volumeReg & DISABLE_LEN_MASK))
    {
        if (chan.lenCounter && m_halfSignal) // once per frame
        {
            chan.lenCounter--;
        }
    }
    // Sequencer
    // Sweep works only if lengthCounter is non-zero
    if (!chan.lenCounter)
    {
        chan.volume = 0;
        chan.output = chan.volume;
        return;
    }
    // Sweep works
    uint8_t sweepReg = m_regs[APU_SWEEP1 + i * 4];
    if ((sweepReg & SWEEP_ENABLE_MASK) && (sweepReg & SWEEP_RATE_MASK) &&
        chan.period >= (0x008 << (CONST_SHIFT_BITS + 4)) &&
        chan.period <= (0x7FF << (CONST_SHIFT_BITS + 4)))
    {
        if (m_halfSignal)
        {
            if (chan.sweepCounter == (sweepReg & SWEEP_RATE_MASK))
            {
                chan.sweepCounter = 0;
                auto delta = chan.period >> (sweepReg & SWEEP_SHIFT_MASK);
                if (sweepReg & SWEEP_DIR_MASK)
                {
                    delta = ~delta;
                    if (i == 1) delta += 1 << (CONST_SHIFT_BITS + 4);
                }
                delta &= (0xFFFF << (CONST_SHIFT_BITS + 4));
            }
            else
            {
                chan.sweepCounter += 0x10;
            }
        }
    }

    if (chan.period < (0x008 << (CONST_SHIFT_BITS + 4)) ||
        chan.period > (0x7FF << (CONST_SHIFT_BITS + 4)))
    {
        chan.volume = 0;
        chan.output = chan.volume;
        return;
    }

    //chan.counter += counterScaler << 3;
    //while (chan.counter >= chan.period + (1 << (CONST_SHIFT_BITS + 4)))
    //{
    //    chan.sequencer++;
    //    chan.sequencer &= 0x07;
    //    chan.counter -= (chan.period + (1 << (CONST_SHIFT_BITS + 4)));
    //}
    //if (!(sequencerTable[(volumeReg & DUTY_CYCLE_MASK) >> 6] & (1 << chan.sequencer)))
    //{
    //    chan.volume = 0;
    //}
    chan.output = chan.volume;
    chan.duty = (volumeReg & DUTY_CYCLE_MASK) >> 6;
}

void SimRP2A03::updateTriangleChannel(ChannelInfo& chan)
{
    /*static constexpr uint8_t triangleTable[] =
    {
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
        15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
    };*/
    bool disabled = !(m_regs[APU_STATUS] & TRI_ENABLE_MASK);

    if (disabled)
    {
        chan.output = 0;
        return;
    }
    // Linear counter control
    uint8_t triangleReg = m_regs[APU_TRIANGLE];
    if (m_quaterSignal)
    {
        if (chan.linearReloadFlag)
            chan.linearCounter = triangleReg & 0x7F;
        else if (chan.linearCounter)
            chan.linearCounter--;
        if (!(triangleReg & 0x80))
        {
            chan.linearReloadFlag = false;
        }
    }
    // Length counter control
    if (m_halfSignal && !(triangleReg & 0x80) && chan.lenCounter) // once per frame
    {
        chan.lenCounter--;
    }

    if ((!chan.lenCounter || !chan.linearCounter))
    {
        chan.output = 0;
        return;
    }

    /*chan.counter += counterScaler << 4;
    while (chan.counter >= (chan.period + (1 << (CONST_SHIFT_BITS + 4))))
    {
        chan.sequencer++;
        chan.sequencer &= 0x1F;
        chan.counter -= (chan.period + (1 << (CONST_SHIFT_BITS + 4)));
        chan.volume = triangleTable[chan.sequencer];
    }*/
    chan.output = 1;
}

void SimRP2A03::updateNoiseChannel(ChannelInfo& chan)
{
    if (!(m_regs[APU_STATUS] & (1 << 3)))
    {
        chan.volume = 0;
        chan.output = chan.volume;
        return;
    }

    uint8_t volumeReg = m_regs[APU_NOISE_VOL];

    // Decay counters are always enabled
    // So, run the envelope anyway
    if (m_quaterSignal)
    {
        if (chan.updateEnvelope)
        {
            chan.decayCounter = 0x0F;
            chan.divider = volumeReg & VALUE_VOL_MASK;
            chan.updateEnvelope = false;
        }
        else if (chan.divider)
        {
            chan.divider--;
        }
        else
        {
            chan.divider = volumeReg & VALUE_VOL_MASK;
            if (chan.decayCounter)
            {
                chan.decayCounter--;
            }
            else if (volumeReg & ENABLE_LOOP_MASK)
            {
                chan.decayCounter = 0x0F;
            }
        }
    }

    if (volumeReg & FIXED_VOL_MASK) // fixed volume
    {
        chan.volume = volumeReg & VALUE_VOL_MASK;
    }
    else
    {
        chan.volume = chan.decayCounter;
    }

    // Countdown len counter if enabled
    if (!(volumeReg & DISABLE_LEN_MASK))
    {
        if (chan.lenCounter && m_halfSignal) // once per frame
        {
            chan.lenCounter--;
        }
    }

    if (!chan.lenCounter)
    {
        chan.volume = 0;
        chan.output = chan.volume;
        return;
    }

    //chan.counter += counterScaler << 3;
    //while (chan.counter >= chan.period + (1 << (CONST_SHIFT_BITS + 4)))
    //{
    //    uint8_t temp;
    //    if (m_regs[APU_NOISE_FREQ] & NOISE_MODE_MASK)
    //    {
    //        // 93-bits
    //        temp = ((m_shiftNoise >> 6) ^ m_shiftNoise) & 1;
    //    }
    //    else
    //    {
    //        // 32768-bits
    //        temp = ((m_shiftNoise >> 1) ^ m_shiftNoise) & 1;
    //    }
    //    m_shiftNoise >>= 1;
    //    m_shiftNoise |= (temp << 14);
    //    chan.counter -= (chan.period + (1 << (CONST_SHIFT_BITS + 4)));
    //}
    //if (m_shiftNoise & 0x01)
    //{
    //    chan.volume = 0;
    //}
    chan.output = chan.volume;
}

void SimRP2A03::updateDmcChannel(ChannelInfo& info)
{
    //if (info.dmcActive && !info.sequencer)
    //{
    //    if (info.dmcLen == 0)
    //    {
    //        if (m_regs[APU_DMC_DMA_FREQ] & DMC_LOOP_MASK)
    //        {
    //            info.dmcAddr = m_regs[APU_DMC_ADDR] * 0x40 + 0xC000;
    //            info.dmcLen = m_regs[APU_DMC_LEN] * 16 + 1;
    //        }
    //        else
    //        {
    //            info.dmcIrqFlag = !!(m_regs[APU_DMC_DMA_FREQ] & DMC_IRQ_ENABLE_MASK);
    //            info.dmcActive = false;
    //            info.output = (static_cast<uint32_t>(m_dmcVolTable[15]) * info.volume) >> 7;
    //            return;
    //        }
    //    }
    //    info.dmcBuffer = m_cpu->read(info.dmcAddr);
    //    info.sequencer = 8;
    //    info.dmcAddr++;
    //    info.dmcLen--;
    //    if (info.dmcAddr == 0x0000) info.dmcAddr = 0x8000;
    //}

    //if (info.sequencer)
    //{
    //    info.counter += counterScaler;
    //    while (info.counter >= info.period)
    //    {
    //        if (info.dmcBuffer & 1)
    //        {
    //            if (info.volume <= 125) info.volume += 2;
    //        }
    //        else
    //        {
    //            if (info.volume >= 2) info.volume -= 2;
    //        }
    //        info.sequencer--;
    //        info.dmcBuffer >>= 1;
    //        info.counter -= info.period;
    //    }
    //}
    //info.output = (static_cast<uint32_t>(m_dmcVolTable[15]) * info.volume) >> 7;
}

SimRP2A03::SimRP2A03()
    : ChipSim(Type::RP2A03)
{
    Reset();
}

void SimRP2A03::Reset()
{
    // TODO
}

void SimRP2A03::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
    uint16_t originReg = reg;
    uint8_t oldVal = data;

    //LOGI("Write 0x%02X to [%04X] reg\n", val, getRegAddress(reg));

    reg &= 0x00FF;

    if (reg < 0x20)
    {
        oldVal = m_regs[reg];
        m_regs[reg] = data;
    }
    int chanIndex = (reg - APU_RECT_VOL1) / 4;

    switch (reg)
    {
    case APU_RECT_VOL1:
    case APU_RECT_VOL2:
    case APU_NOISE_VOL:
        //            m_chan[chanIndex].divider = val & VALUE_VOL_MASK;
        break;

    case APU_TRIANGLE:
        break;

    case APU_SWEEP1:
    case APU_SWEEP2:
        break;

    case APU_RECT_FREQ1:
    case APU_RECT_FREQ2:
    case APU_TRI_FREQ:
        m_chan[chanIndex].period = (m_chan[chanIndex].period & (0xFF00 << (CONST_SHIFT_BITS + 4))) | (data << (CONST_SHIFT_BITS + 4));
        /* if (m_chan[chanIndex].counter > m_chan[chanIndex].period)
             m_chan[chanIndex].counter = m_chan[chanIndex].period;*/
        break;

    case APU_NOISE_FREQ:
    {
        // reset noise generator when switching modes
        if ((oldVal & NOISE_MODE_MASK) != (data & NOISE_MODE_MASK))
        {
            m_shiftNoise = 0x0001;
        }
        m_chan[chanIndex].period = (noiseLut[data & NOISE_FREQ_MASK]) << (CONST_SHIFT_BITS + 4);
        /* if (m_chan[chanIndex].counter > m_chan[chanIndex].period)
             m_chan[chanIndex].counter = m_chan[chanIndex].period;*/
        break;
    }

    case APU_RECT_LEN1:
    case APU_RECT_LEN2:
        // We must reset duty cycle sequencer only for channels 1, 2 according to datasheet
        /*m_chan[chanIndex].sequencer = 0;*/
        // Reset counter also to prevent unexpected clicks
       /* m_chan[chanIndex].counter = 0;*/
        m_chan[chanIndex].updateEnvelope = true;
        // Unused on noise channel
        m_chan[chanIndex].period = (m_chan[chanIndex].period & (0x000000FF << (CONST_SHIFT_BITS + 4))) | (static_cast<uint32_t>(data & 0x07) << (8 + CONST_SHIFT_BITS + 4));
        m_chan[chanIndex].lenCounter = lengthLut[data >> 3];
        /*m_chan[chanIndex].counter = 0;*/
        break;

    case APU_TRI_LEN:
        // Do not reset triangle sequencer to prevent clicks. This is required per datasheet
        // Unused on noise channel
        m_chan[2].period = (m_chan[2].period & (0x000000FF << (CONST_SHIFT_BITS + 4))) | (static_cast<uint32_t>(data & 0x07) << (8 + CONST_SHIFT_BITS + 4));
        m_chan[2].lenCounter = lengthLut[data >> 3];
        m_chan[2].linearReloadFlag = true;
        /*m_chan[2].counter = 0;*/
        break;

    case APU_NOISE_LEN:
        m_chan[3].updateEnvelope = true;
        m_chan[3].lenCounter = lengthLut[data >> 3];
        /*m_chan[3].counter = 0;*/
        break;

    case APU_DMC_DMA_FREQ:
        //m_chan[4].period = dmcLut[val & DMC_RATE_MASK] << CONST_SHIFT_BITS;
        //if (m_chan[4].counter >= m_chan[4].period)
        //    m_chan[4].counter = m_chan[4].period;
        break;

    case APU_DMC_DELTA_COUNTER:
        //m_chan[4].volume = val & 0x7F;
        break;

    case APU_DMC_ADDR:
        break;

    case APU_DMC_LEN:
        break;

    case APU_STATUS:
        for (int i = 0; i < 4; i++)
        {
            if (!(data & (1 << i)))
            {
                m_chan[i].counter = 0;
                m_chan[i].lenCounter = 0;
            }
        }
        /*if ((m_regs[APU_STATUS] & 0x10) && !m_chan[4].dmcActive)
        {
            m_chan[4].dmcActive = true;
            m_chan[4].dmcAddr = m_regs[APU_DMC_ADDR] * 0x40 + 0xC000;
            m_chan[4].dmcLen = m_regs[APU_DMC_LEN] * 16 + 1;
            m_chan[4].dmcIrqFlag = false;
        }
        else if (!(m_regs[APU_STATUS] & 0x10))
        {
            m_chan[4].dmcActive = false;
        }*/
        break;

    case APU_LOW_TIMER:
        m_lastFrameCounter = 0;
        m_apuFrames = 0;
        break;

    default:
        // Check for sweep support on channels TRI and NOISE
        if (reg != 0x09 && reg != 0x0D)
        {
            //LOGE("Unknown reg 0x%02X [0x%04X]\n", reg, originReg);
        }
        break;
    }
}

void SimRP2A03::Simulate(int samples)
{
    while (samples--)
    {
        updateFrameCounter();
        updateRectChannel(0);
        updateRectChannel(1);
        updateTriangleChannel(m_chan[2]);
        updateNoiseChannel(m_chan[3]);
        updateDmcChannel(m_chan[4]);
    }
}

void SimRP2A03::ConvertToPSG(Frame& frame)
{
    auto ProcessVolume = [](uint8_t& volume)
    {
        if (volume > 0)
        {
            float factor = float(volume - 1) / 14.f;
            volume = uint8_t(10 * factor + 5);
        }
    };

    // process
    uint8_t  a1_volume = m_chan[0].output;
    uint16_t a1_period = m_chan[0].period >> 8;
    uint8_t  a1_enable = bool(m_chan[0].output > 0);
    uint8_t  a1_mode = m_chan[0].duty;

    uint8_t  c2_volume = m_chan[1].output;
    uint16_t c2_period = m_chan[1].period >> 8;
    uint8_t  c2_enable = bool(m_chan[1].output > 0);
    uint8_t  c2_mode = m_chan[1].duty;

    uint16_t bt_period = m_chan[2].period >> (8 + 2);
    uint8_t  bt_enable = bool(m_chan[2].output > 0);
    if (!bt_enable || !bt_period) bt_period = 0xFFFF;

    uint8_t  bn_volume = m_chan[3].output << 0;
    uint16_t bn_period = m_chan[3].period >> (8 + 6);
    uint8_t  bn_enable = bool(m_chan[3].output > 0);
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
    if (frame.Read(0, E_Shape) != 0x0E)
    {
        frame.Write(0, E_Shape, 0x0E);
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

void SimRP2A03::PostProcess(Stream& stream)
{
#if 1
    float pulseVolFactor = 15.0f / std::max(m_maxVol[0], m_maxVol[1]);
    float noiseVolFactor = 15.0f / m_maxVol[2];

    for (int i = 0, c = stream.frames.count(); i < c; ++i)
    {
        Frame& frame = const_cast<Frame&>(stream.frames.get(i));

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
