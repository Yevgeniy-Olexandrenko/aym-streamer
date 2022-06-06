#include "ChipAY8930.h"

void ChipAY8930::InternalWrite(byte reg, byte data)
{
	// TODO

	ChipAY8910::InternalWrite(reg, data);
}

void ChipAY8930::InternalUpdate(double& outL, double& outR)
{
    int noise = m_noise.Update();
    int envelope = m_envelope.Update();
    int tone, out;
    for (Channel& channel : m_channels)
    {
        tone = channel.tone.Update();
        out  = (tone & !channel.t_off) | (noise & !channel.n_off);
        out  = (channel.e_on ? envelope : channel.volume) * out;

        outL += m_dacTable[out] * channel.panL;
        outR += m_dacTable[out] * channel.panR;
    }
}
