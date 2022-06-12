#include "ChipAY8910.h"

namespace
{
    const double AY_DAC_TABLE[] = 
    {
        0.0, 0.0,
        0.00999465934234, 0.00999465934234,
        0.01445029373620, 0.01445029373620,
        0.02105745021740, 0.02105745021740,
        0.03070115205620, 0.03070115205620,
        0.04554818036160, 0.04554818036160,
        0.06449988555730, 0.06449988555730,
        0.10736247806500, 0.10736247806500,
        0.12658884565500, 0.12658884565500,
        0.20498970016000, 0.20498970016000,
        0.29221026932200, 0.29221026932200,
        0.37283894102400, 0.37283894102400,
        0.49253070878200, 0.49253070878200,
        0.63532463569100, 0.63532463569100,
        0.80558480201400, 0.80558480201400,
        1.0, 1.0
    };

    const double YM_DAC_TABLE[] = 
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

bool ChipAY8910::Configure(double clockRate, int sampleRate, bool isYM)
{
    m_dacTable = (isYM ? YM_DAC_TABLE : AY_DAC_TABLE);
    return ChipAY89xx::Configure(clockRate, sampleRate);
}

void ChipAY8910::InternalSetPan(int chan, double panL, double panR)
{
    m_channels[chan].panL = panL;
    m_channels[chan].panR = panR;
}

void ChipAY8910::InternalReset()
{
    m_noise.Reset();
    m_envelope.Reset();

    for (Channel& channel : m_channels)
    {
        channel.tone.Reset();
        channel.mixer.Reset();
    }
}

void ChipAY8910::InternalWrite(byte reg, byte data)
{
    switch (reg)
    {
    case 0x00:
    case 0x02: 
    case 0x04:
        m_channels[reg >> 1].tone.SetPeriodL(data);
        break;

    case 0x01: 
    case 0x03:
    case 0x05:
        m_channels[reg >> 1].tone.SetPeriodH(data);
        break;

    case 0x06:
        m_noise.SetPeriod(data);
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
        m_channels[reg - 0x08].mixer.SetVolume(data);
        break;

    case 0x0B:
        m_envelope.SetPeriodL(data);
        break;

    case 0x0C:
        m_envelope.SetPeriodH(data);
        break;

    case 0x0D:
        m_envelope.SetShape(data);
        break;
    }
}

void ChipAY8910::InternalUpdate(double& outL, double& outR)
{
    int outN = m_noise.Update();
    int outE = m_envelope.Update();

    for (Channel& channel : m_channels)
    {
        int outT = channel.tone.Update();
        int outM = channel.mixer.GetOutput(outT, outN, outE);

        outL += m_dacTable[outM] * channel.panL;
        outR += m_dacTable[outM] * channel.panR;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Tone Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8910::ToneUnit::Reset()
{
    m_tone = 0;
    SetPeriodL(0);
    SetPeriodH(0);
}

void ChipAY8910::ToneUnit::SetPeriodL(int period)
{
    m_period &= 0xFF00;
    m_period |= (period & 0xFF);
    m_period |= (m_period == 0);
}

void ChipAY8910::ToneUnit::SetPeriodH(int period)
{
    m_period &= 0x00FF;
    m_period |= (period & 0x0F) << 8;
    m_period |= (m_period == 0);
}

int ChipAY8910::ToneUnit::Update()
{
    if (++m_counter >= m_period)
    {
        m_counter = 0;
        m_tone ^= 1;
    }
    return m_tone;
}

////////////////////////////////////////////////////////////////////////////////
// Noise Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8910::NoiseUnit::SetPeriod(int period)
{
    m_period  = (period & 0x1F);
    m_period |= (m_period == 0);
}

void ChipAY8910::NoiseUnit::Reset()
{
    m_noise = 1;
    SetPeriod(0);
}

int ChipAY8910::NoiseUnit::Update()
{
    if (++m_counter >= (m_period << 1))
    {
        m_counter = 0;
        m_noise = (m_noise >> 1) | (((m_noise ^ (m_noise >> 3)) & 1) << 16);
    }
    return (m_noise & 1);
}

////////////////////////////////////////////////////////////////////////////////
// Mixer Unit Implementation
////////////////////////////////////////////////////////////////////////////////

void ChipAY8910::MixerUnit::Reset()
{
    SetEnable(0);
    SetVolume(0);
}

void ChipAY8910::MixerUnit::SetEnable(int flags)
{
    m_T_Off = bool(flags & 0x01);
    m_N_Off = bool(flags & 0x08);
}

void ChipAY8910::MixerUnit::SetVolume(int volume)
{
    m_volume = ((volume & 0x0F) << 1) + 1;
    m_E_On = bool(volume & 0x10);
}

int ChipAY8910::MixerUnit::GetOutput(int tone, int noise, int envelope) const
{
    int out = (tone | m_T_Off) & (noise | m_N_Off);
    return (out * (m_E_On ? envelope : m_volume));
}

////////////////////////////////////////////////////////////////////////////////
// Envelope Unit Implementation
////////////////////////////////////////////////////////////////////////////////

namespace
{
    enum { SU = +1, SD = -1, HT = 0, HB = 0, RT = 0x1F, RB = 0x00 };

    int envelopes[16][2][2]
    {
        { { SD, RT }, { HB, RB }, },
        { { SD, RT }, { HB, RB }, },
        { { SD, RT }, { HB, RB }, },
        { { SD, RT }, { HB, RB }, },
        { { SU, RB }, { HB, RB }, },
        { { SU, RB }, { HB, RB }, },
        { { SU, RB }, { HB, RB }, },
        { { SU, RB }, { HB, RB }, },
        { { SD, RT }, { SD, RT }, },
        { { SD, RT }, { HB, RB }, },
        { { SD, RT }, { SU, RB }, },
        { { SD, RT }, { HT, RT }, },
        { { SU, RB }, { SU, RB }, },
        { { SU, RB }, { HT, RT }, },
        { { SU, RB }, { SD, RT }, },
        { { SU, RB }, { HB, RB }, },
    };
}

void ChipAY8910::EnvelopeUnit::Reset()
{
    SetPeriodL(0);
    SetPeriodH(0);
    SetShape(0);
}

void ChipAY8910::EnvelopeUnit::SetPeriodL(int period)
{
    m_period &= 0xFF00;
    m_period |= (period & 0xFF);
    m_period |= (m_period == 0);
}

void ChipAY8910::EnvelopeUnit::SetPeriodH(int period)
{
    m_period &= 0x00FF;
    m_period |= (period & 0xFF) << 8;
    m_period |= (m_period == 0);
}

void ChipAY8910::EnvelopeUnit::SetShape(int shape)
{
    m_shape = (shape & 0x0F);
    m_counter = 0;
    m_segment = 0;
    m_envelope = envelopes[m_shape][m_segment][1];
}

int ChipAY8910::EnvelopeUnit::Update()
{
    if (++m_counter >= m_period)
    {
        m_counter = 0;

        m_envelope += envelopes[m_shape][m_segment][0];
        if (m_envelope < RB || m_envelope > RT)
        {
            m_segment ^= 1;
            m_envelope = envelopes[m_shape][m_segment][1];
        }
    }
    return m_envelope;
}
