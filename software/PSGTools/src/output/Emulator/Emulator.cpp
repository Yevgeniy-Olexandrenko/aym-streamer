#include <thread>
#include "Emulator.h"
#include "module/Module.h"

namespace
{
    const int k_sample_rate = 44100;
}

Emulator::Emulator()
    : m_ay{0}
{
}

Emulator::~Emulator()
{
    Close();
}

std::string Emulator::name() const
{
    return ("Emulator -> " + chip.toString());
}

bool Emulator::Open()
{
    if (!m_isOpened)
    {
        if (WaveAudio::Open(k_sample_rate, 100, 2, 2))
        {
            m_isOpened = true;

            // make some delay for warm up the wave audio
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return m_isOpened;
}

bool Emulator::Init(const Module& module)
{
    if (m_isOpened)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        chip.count(module.chip.count());

        chip.model(Chip::Model::AY);
        if (module.chip.modelKnown())
        {
            if (module.chip.model() == Chip::Model::YM)
            {
                chip.model(Chip::Model::YM);
            }
        }

        chip.frequency(Chip::Frequency::F1750000);
        if (module.chip.frequencyKnown())
        {
            chip.frequency(module.chip.frequency());
        }

        chip.channels(Chip::Channels::ABC);
        if (module.chip.channelsKnown())
        {
            chip.channels(module.chip.channels());
        }

        m_isOpened &= InitChip(0);
        if (chip.count() == Chip::Count::TurboSound)
        {
            m_isOpened &= InitChip(1);
        }
    }
    return m_isOpened;
}

bool Emulator::OutFrame(const Frame& frame, bool force)
{
    if (m_isOpened)
    {
        WriteToChip(0, frame, force);
        if (chip.count() == Chip::Count::TurboSound)
        {
            WriteToChip(1, frame, force);
        }
    }
    return m_isOpened;
}

void Emulator::Close()
{
    WaveAudio::Close();
}

void Emulator::FillBuffer(unsigned char* buffer, unsigned long size)
{
    // buffer format must be 2 ch x 16 bit
    auto sampbuf = (int16_t*)buffer;
    auto samples = (int)(size / sizeof(int16_t));

    if (chip.count() == Chip::Count::TurboSound)
    {
        for (int i = 0; i < samples;)
        {
            ayumi_process(&m_ay[0]);
            ayumi_process(&m_ay[1]);
            ayumi_remove_dc(&m_ay[0]);
            ayumi_remove_dc(&m_ay[1]);

            double L = 0.5 * (m_ay[0].left  + m_ay[1].left );
            double R = 0.5 * (m_ay[0].right + m_ay[1].right);

            L = L > +1.0 ? +1.0 : (L < -1.0 ? -1.0 : L);
            R = R > +1.0 ? +1.0 : (R < -1.0 ? -1.0 : R);

            sampbuf[i++] = (int16_t)(INT16_MAX * L + 0.5);
            sampbuf[i++] = (int16_t)(INT16_MAX * R + 0.5);
        }
    }
    else
    {
        ayumi& chip = m_ay[0];
        for (int i = 0; i < samples;)
        {
            ayumi_process(&chip);
            ayumi_remove_dc(&chip);

            double L = 0.5 * chip.left;
            double R = 0.5 * chip.right;

            L = L > +1.0 ? +1.0 : (L < -1.0 ? -1.0 : L);
            R = R > +1.0 ? +1.0 : (R < -1.0 ? -1.0 : R);

            sampbuf[i++] = (int16_t)(INT16_MAX * L + 0.5);
            sampbuf[i++] = (int16_t)(INT16_MAX * R + 0.5);
        }
    }
}

bool Emulator::InitChip(uint8_t chipIndex)
{
    ayumi* ay = &m_ay[chipIndex];
    if (ayumi_configure(ay, (chip.model() == Chip::Model::YM), chip.freqValue(), k_sample_rate))
    {
        switch (chip.channels())
        {
        case Chip::Channels::MONO:
            ayumi_set_pan(ay, 0, 0.5, true);
            ayumi_set_pan(ay, 1, 0.5, true);
            ayumi_set_pan(ay, 2, 0.5, true);
            break;

        case Chip::Channels::ACB:
            ayumi_set_pan(ay, 0, 0.1, true);
            ayumi_set_pan(ay, 1, 0.9, true);
            ayumi_set_pan(ay, 2, 0.5, true);
            break;

        default: // ABC
            ayumi_set_pan(ay, 0, 0.1, true);
            ayumi_set_pan(ay, 1, 0.5, true);
            ayumi_set_pan(ay, 2, 0.9, true);
            break;
        }
        return true;
    }
    return false;
}

void Emulator::WriteToChip(uint8_t chipIndex, const Frame& frame, bool force)
{
    ayumi* ay = &m_ay[chipIndex];

    uint8_t r7 = frame.data(chipIndex, Mixer_Flags);
    uint8_t r8 = frame.data(chipIndex, VolA_EnvFlg);
    uint8_t r9 = frame.data(chipIndex, VolB_EnvFlg);
    uint8_t rA = frame.data(chipIndex, VolC_EnvFlg);

    ayumi_set_tone(ay, 0, frame.data(chipIndex, TonA_PeriodL) | frame.data(chipIndex, TonA_PeriodH) << 8);
    ayumi_set_tone(ay, 1, frame.data(chipIndex, TonB_PeriodL) | frame.data(chipIndex, TonB_PeriodH) << 8);
    ayumi_set_tone(ay, 2, frame.data(chipIndex, TonC_PeriodL) | frame.data(chipIndex, TonC_PeriodH) << 8);

    ayumi_set_noise(ay, frame.data(chipIndex, Noise_Period));

    ayumi_set_mixer(ay, 0, r7 >> 0 & 0x01, r7 >> 3 & 0x01, r8 >> 4);
    ayumi_set_mixer(ay, 1, r7 >> 1 & 0x01, r7 >> 4 & 0x01, r9 >> 4);
    ayumi_set_mixer(ay, 2, r7 >> 2 & 0x01, r7 >> 5 & 0x01, rA >> 4);

    ayumi_set_volume(ay, 0, r8 & 0x0F);
    ayumi_set_volume(ay, 1, r9 & 0x0F);
    ayumi_set_volume(ay, 2, rA & 0x0F);

    ayumi_set_envelope(ay, frame.data(chipIndex, Env_PeriodL) | frame.data(chipIndex, Env_PeriodH) << 8);
    if (force || frame.changed(chipIndex, Env_Shape))
    {
        ayumi_set_envelope_shape(ay, frame.data(chipIndex, Env_Shape));
    }
}

