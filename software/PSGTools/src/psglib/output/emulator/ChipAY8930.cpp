#include "ChipAY8930.h"

#pragma warning( disable : 6385)

namespace
{
    const double DAC_TABLE[] =
    {
        0.0, 0.0,
        0.00465400167849, 0.00772106507973,
        0.01095597772180, 0.01396200503550,
        0.01699855039290, 0.02001983672850,
        0.02436865796900, 0.02969405661100,
        0.03506523231860, 0.04039063096060,
        0.04853894865340, 0.05833524071110,
        0.06805523765930, 0.07777523460750,
        0.09251544975970, 0.11108567940800,
        0.12974746318800, 0.14848554207700,
        0.17666895552000, 0.21155107957600,
        0.24638742656600, 0.28110170138100,
        0.33373006790300, 0.40042725261300,
        0.46738384069600, 0.53443198291000,
        0.63517204547200, 0.75800717174000,
        0.87992675669500, 1.0
    };
}

void ChipAY8930::InternalSetPan(int chan, double panL, double panR)
{
    m_channels[chan].panL = panL;
    m_channels[chan].panR = panR;
}

void ChipAY8930::InternalReset()
{
    m_expMode = false;
    m_expBank = 0x00;
    ResetMode();
}

void ChipAY8930::InternalWrite(byte reg, byte data)
{
    reg &= 0x0F;
    switch (reg | m_expBank)
    {
    case 0x00:
    case 0x02:
    case 0x04:
        m_channels[reg >> 1].tone.SetPeriodL(m_expMode, data);
        break;

    case 0x01:
    case 0x03:
    case 0x05:
        m_channels[reg >> 1].tone.SetPeriodH(m_expMode, data);
        break;

    case 0x06:
        m_noise.SetPeriod(m_expMode, data);
        break;

    case 0x07:
        for (int i = 0; i < 3; ++i, data >>= 1)
        {
            m_channels[i].mixer.SetEnable(data);
        }
        break;

    case 0x08:
    case 0x09:
    case 0x0A:
        m_channels[reg - 0x08].mixer.SetVolume(m_expMode, data);
        break;

    case 0x0B:
        m_channels[0].envelope.SetPeriodL(data);
        break;

    case 0x0C:
        m_channels[0].envelope.SetPeriodH(data);
        break;

    case 0x0D:
    case 0x1D:
        {
            bool prevMode = m_expMode;
            m_expMode = (data & 0b11100000) == 0b10100000;
            m_expBank = (data & 0b00010000);
            if (m_expMode != prevMode) ResetMode();
        }
        m_channels[0].envelope.SetShape(data);
        break;

    case 0x10:
    case 0x12:
        m_channels[1 + (reg >> 1)].envelope.SetPeriodL(data);
        break;

    case 0x11:
    case 0x13:
        m_channels[1 + (reg >> 1)].envelope.SetPeriodH(data);
        break;

    case 0x14:
    case 0x15:
        m_channels[1 + (reg - 0x04)].envelope.SetShape(data);
        break;

    case 0x16:
    case 0x17:
    case 0x18:
        m_channels[reg - 0x06].tone.SetDuty(data);
        break;

    case 0x19:
        m_noise.SetAndMask(data);
        break;

    case 0x1A:
        m_noise.SetOrMask(data);
        break;
    }
}

void ChipAY8930::InternalUpdate(double& outL, double& outR)
{
    if (m_expMode)
    {
        int outN = m_noise.Update(true);
        for (Channel& channel : m_channels)
        {
            int outE = channel.envelope.Update();
            int outT = channel.tone.Update(true);
            int outM = channel.mixer.GetOutput(true, outT, outN, outE);

            outL += DAC_TABLE[outM] * channel.panL;
            outR += DAC_TABLE[outM] * channel.panR;
        }
    }
    else
    {
        int outN = m_noise.Update(false);
        int outE = m_channels[0].envelope.Update();

        for (Channel& channel : m_channels)
        {
            int outT = channel.tone.Update(false);
            int outM = channel.mixer.GetOutput(false, outT, outN, outE);

            outL += DAC_TABLE[outM] * channel.panL;
            outR += DAC_TABLE[outM] * channel.panR;
        }
    }
}

void ChipAY8930::ResetMode()
{
    m_noise.Reset();

    for (Channel& channel : m_channels)
    {
        channel.tone.Reset();
        channel.mixer.Reset();
        channel.envelope.Reset();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Tone Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8930::ToneUnit::Reset()
{
    ChipAY8910::ToneUnit::Reset();

    // TODO
}

void ChipAY8930::ToneUnit::SetPeriodL(bool exp, int period)
{
    ChipAY8910::ToneUnit::SetPeriodL(period);
}

void ChipAY8930::ToneUnit::SetPeriodH(bool exp, int period)
{
    if (exp)
    {
        m_period &= 0x00FF;
        m_period |= (period & 0xFF) << 8;
        m_period |= (m_period == 0);
    }
    else
        ChipAY8910::ToneUnit::SetPeriodH(period);
}

void ChipAY8930::ToneUnit::SetDuty(int duty)
{
    // TODO
}

int ChipAY8930::ToneUnit::Update(bool exp)
{
    if (exp)
    {
        // TODO
        return 0;
    }
    return ChipAY8910::ToneUnit::Update();
}

////////////////////////////////////////////////////////////////////////////////
// Noise Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8930::NoiseUnit::Reset()
{
    ChipAY8910::NoiseUnit::Reset();

    // TODO
}

void ChipAY8930::NoiseUnit::SetPeriod(bool exp, int period)
{
    if (exp)
    {
        m_period = (period & 0xFF);
        m_period |= (m_period == 0);
    }
    else
        ChipAY8910::NoiseUnit::SetPeriod(period);
}

void ChipAY8930::NoiseUnit::SetAndMask(int mask)
{
    // TODO
}

void ChipAY8930::NoiseUnit::SetOrMask(int mask)
{
    // TODO
}

int ChipAY8930::NoiseUnit::Update(bool exp)
{
    if (exp)
    {
        // TODO
        return 0;
    }

    if (++m_counter >= (m_period << 1))
    {
        m_counter = 0;
        m_noise = (m_noise >> 1) | (((m_noise ^ (m_noise >> 2)) & 1) << 16);
    }
    return (m_noise & 1);
}

////////////////////////////////////////////////////////////////////////////////
// Mixer Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8930::MixerUnit::SetVolume(bool exp, int volume)
{
    if (exp)
    {
        m_volume = (volume & 0x1F);
        m_E_On = bool(volume & 0x20);
    }
    else
        ChipAY8910::MixerUnit::SetVolume(volume);
}

int ChipAY8930::MixerUnit::GetOutput(bool exp, int tone, int noise, int envelope) const
{
    int out = (tone & ~m_T_Off) | (noise & ~m_N_Off);
    return (out * (m_E_On ? envelope : m_volume));
}
