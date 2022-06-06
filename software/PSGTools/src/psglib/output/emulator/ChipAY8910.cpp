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
        channel.SetMixer(false, false, false);
        channel.SetVolume(0);
    }
}

void ChipAY8910::InternalWrite(byte reg, byte data)
{
    switch (reg)
    {
    case 0x00: case 0x02: case 0x04:
    {
        Channel& channel = m_channels[reg >> 1];
        channel.tone.SetPeriod((channel.tone.GetPeriod() & 0xFF00) | data);
        break;
    }

    case 0x01: case 0x03: case 0x05:
    {
        Channel& channel = m_channels[reg >> 1];
        channel.tone.SetPeriod((channel.tone.GetPeriod() & 0x00FF) | (data << 8));
        break;
    }

    case 0x06:
        m_noise.SetPeriod(data);
        break;

    case 0x07:
        for (Channel& channel : m_channels)
        {
            channel.SetMixer(data & 0x01, data & 0x08, channel.e_on);
            data >>= 1;
        }
        break;

    case 0x08: case 0x09: case 0x0A:
    {
        Channel& channel = m_channels[reg - 0x08];
        channel.SetMixer(channel.t_off, channel.n_off, data & 0x10);
        channel.SetVolume(data);
        break;
    }

    case 0x0B:
        m_envelope.SetPeriod((m_envelope.GetPeriod() & 0xFF00) | data);
        break;

    case 0x0C:
        m_envelope.SetPeriod((m_envelope.GetPeriod() & 0x00FF) | (data << 8));
        break;

    case 0x0D:
        m_envelope.SetShape(data);
        break;
    }
}

void ChipAY8910::InternalUpdate(double& outL, double& outR)
{
    int noise = m_noise.Update();
    int envelope = m_envelope.Update();
    int tone, out;
    for (Channel& channel : m_channels)
    {
        tone = channel.tone.Update();
        out  = (tone | channel.t_off) & (noise | channel.n_off);
        out  = (channel.e_on ? envelope : channel.volume) * out;

        outL += m_dacTable[out] * channel.panL;
        outR += m_dacTable[out] * channel.panR;
    }
}

void ChipAY8910::Channel::SetVolume(int volume)
{
    this->volume = ((volume & 0x0F) << 1) + 1;
}

void ChipAY8910::Channel::SetMixer(bool t_off, bool n_off, bool e_on)
{
    this->t_off = t_off;
    this->n_off = n_off;
    this->e_on  = e_on;
}
