#include "SimSN76489.h"

namespace
{
    const float k_gainDefaultValue = 1.0f; // 1.75f;
    const float k_noiseVolumePower = std::sqrt(2.0f);
}

SimSN76489::SimSN76489()
	: ChipSim(Type::SN76489)
{
	Reset();
}

void SimSN76489::Reset()
{
    for (int i = 0; i <= 3; i++)
    {
        /* Initialise PSG state */
        SN76489.Registers[2 * i] = 1; /* tone freq=1 */
        SN76489.Registers[2 * i + 1] = 0xf; /* vol=off */
    }

    /* Initialise latched register index */
    SN76489.LatchedRegister = 0;

    /* Initialise noise generator */
    SN76489.NoiseFreq = 0x10;
}

void SimSN76489::Write(int chip, Register reg, uint8_t data)
{
    unsigned int index;

    if (data & 0x80)
    {
        /* latch byte  %1 cc t dddd */
        SN76489.LatchedRegister = index = (data >> 4) & 0x07;
    }
    else
    {
        /* restore latched register index */
        index = SN76489.LatchedRegister;
    }

    switch (index)
    {
    case 0:
    case 2:
    case 4: /* Tone Channels frequency */
    {
        if (data & 0x80)
        {
            /* Data byte  %1 cc t dddd */
            SN76489.Registers[index] = (SN76489.Registers[index] & 0x3f0) | (data & 0xf);
        }
        else
        {
            /* Data byte  %0 - dddddd */
            SN76489.Registers[index] = (SN76489.Registers[index] & 0x00f) | ((data & 0x3f) << 4);
        }

        /* zero frequency behaves the same as a value of 1 */
        if (SN76489.Registers[index] == 0)
        {
            SN76489.Registers[index] = 1;
        }
        break;
    }

    case 1:
    case 3:
    case 5: /* Tone Channels attenuation */
    {
        data &= 0x0f;
        SN76489.Registers[index] = data;

        //data = PSGVolumeValues[data];
        //index >>= 1;
        //SN76489.Channel[index][0] = (data * SN76489.PreAmp[index][0]) / 100;
        //SN76489.Channel[index][1] = (data * SN76489.PreAmp[index][1]) / 100;
        break;
    }

    case 6: /* Noise control */
    {
        SN76489.Registers[6] = data & 0x0f;

        /* reset shift register */
        //SN76489.NoiseShiftRegister = NoiseInitialState;

        /* set noise signal generator frequency */
        SN76489.NoiseFreq = 0x10 << (data & 0x3);
        break;
    }

    case 7: /* Noise attenuation */
    {
        data &= 0x0f;
        SN76489.Registers[7] = data;

        //data = PSGVolumeValues[data];
        //SN76489.Channel[3][0] = (data * SN76489.PreAmp[3][0]) / 100;
        //SN76489.Channel[3][1] = (data * SN76489.PreAmp[3][1]) / 100;
        break;
    }
    }
}

void SimSN76489::Simulate(int samples)
{
	// TODO
}

void SimSN76489::Convert(Frame& frame)
{
    bool enable0 = (SN76489.Registers[1] != 0x0F);
    bool enable1 = (SN76489.Registers[3] != 0x0F);
    bool enable2 = (SN76489.Registers[5] != 0x0F);
    bool enable3 = (SN76489.Registers[7] != 0x0F && (SN76489.Registers[6] & 0x04));

    uint8_t mixer = 0x3F;

    //
    if (enable0)
    {
        uint16_t period = ConvertPeriod(SN76489.Registers[0] << 1);
        uint8_t  volume = ConvertVolume(SN76489.Registers[1]);

        EnableTone(mixer, Frame::Channel::A);
        frame[0].UpdatePeriod(A_Period, period);
        frame[0].Update(A_Volume, volume);
    }

    //
    if (enable1)
    {
        uint16_t period = ConvertPeriod(SN76489.Registers[2] << 1);
        uint8_t  volume = ConvertVolume(SN76489.Registers[3]);

        EnableTone(mixer, Frame::Channel::B);
        frame[0].UpdatePeriod(B_Period, period);
        frame[0].Update(B_Volume, volume);
    }

    //
    if (enable2)
    {
        uint16_t period = ConvertPeriod(SN76489.Registers[4] << 1);
        uint8_t  volume = ConvertVolume(SN76489.Registers[5]);

        EnableTone(mixer, Frame::Channel::C);
        frame[0].UpdatePeriod(C_Period, period);
        frame[0].Update(C_Volume, volume);
    }

    //
    if (enable3)
    {
        int NoiseFreq = SN76489.NoiseFreq;
        if (NoiseFreq == 0x80)
            NoiseFreq = SN76489.Registers[2 * 2];

        uint16_t period = ConvertPeriod(NoiseFreq);
        if (period > 0x1F) period = 0x1F;

        int volumeN = ConvertVolume(SN76489.Registers[7]);
        int volumeA = frame[0].Read(A_Volume);
        int volumeC = frame[0].Read(C_Volume);

        frame[0].UpdatePeriod(N_Period, period);

        if (!enable0)
        {
            volumeA = int(0.5f * (volumeN * k_noiseVolumePower));
            frame[0].Update(A_Volume, uint8_t(volumeA));
        }
        if (!enable1)
        {
            volumeC = int(0.5f * (volumeN * k_noiseVolumePower));
            frame[0].Update(C_Volume, uint8_t(volumeC));
        }

        if (volumeA + volumeC <= volumeN * k_noiseVolumePower)
        {
            EnableNoise(mixer, Frame::Channel::A);
            EnableNoise(mixer, Frame::Channel::C);
        }
        else if (std::abs(volumeN - volumeA) < std::abs(volumeN - volumeC))
            EnableNoise(mixer, Frame::Channel::A);
        else
            EnableNoise(mixer, Frame::Channel::C);
    }

    // Mixer
    {
        frame[0].Update(Mixer, mixer);
    }
}

uint8_t SimSN76489::ConvertVolume(uint8_t volume) const
{
    return (0x0F - volume);
}
