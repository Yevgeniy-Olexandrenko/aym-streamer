#include <thread>
#include "Emulator.h"
#include "stream/Stream.h"

namespace
{
    const int k_sample_rate = 44100;
}

Emulator::Emulator()
#ifndef USE_NEW_AY8910
    : m_ay{0}
#endif
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

bool Emulator::Init(const Stream& stream)
{
    if (m_isOpened)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        chip.count(stream.chip.count());

        chip.model(Chip::Model::AY8910);
        if (stream.chip.modelKnown())
        {
            if (stream.chip.model() == Chip::Model::YM2149)
            {
                chip.model(Chip::Model::YM2149);
            }
        }

        chip.frequency(Chip::Frequency::F1750000);
        if (stream.chip.frequencyKnown())
        {
            chip.frequency(stream.chip.frequency());
        }

        chip.channels(Chip::Channels::ABC);
        if (stream.chip.channelsKnown())
        {
            chip.channels(stream.chip.channels());
        }

        m_isOpened &= InitChip(0);
        if (chip.count() == Chip::Count::TwoChips)
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
        if (chip.count() == Chip::Count::TwoChips)
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

    if (chip.count() == Chip::Count::TwoChips)
    {
        for (int i = 0; i < samples;)
        {
#ifdef USE_NEW_AY8910
            m_ay[0].Process();
            m_ay[1].Process();
            m_ay[0].RemoveDC();
            m_ay[1].RemoveDC();

            double L = 0.5 * (m_ay[0].GetOutL() + m_ay[1].GetOutL());
            double R = 0.5 * (m_ay[0].GetOutR() + m_ay[1].GetOutR());
#else
            ayumi_process(&m_ay[0]);
            ayumi_process(&m_ay[1]);
            ayumi_remove_dc(&m_ay[0]);
            ayumi_remove_dc(&m_ay[1]);

            double L = 0.5 * (m_ay[0].left + m_ay[1].left);
            double R = 0.5 * (m_ay[0].right + m_ay[1].right);
#endif
            L = L > +1.0 ? +1.0 : (L < -1.0 ? -1.0 : L);
            R = R > +1.0 ? +1.0 : (R < -1.0 ? -1.0 : R);

            sampbuf[i++] = (int16_t)(INT16_MAX * L + 0.5);
            sampbuf[i++] = (int16_t)(INT16_MAX * R + 0.5);
        }
    }
    else
    {
        for (int i = 0; i < samples;)
        {
#ifdef USE_NEW_AY8910
            m_ay[0].Process();
            m_ay[0].RemoveDC();

            double L = 0.5 * m_ay[0].GetOutL();
            double R = 0.5 * m_ay[0].GetOutR();
#else
            ayumi_process(&m_ay[0]);
            ayumi_remove_dc(&m_ay[0]);

            double L = 0.5 * m_ay[0].left;
            double R = 0.5 * m_ay[0].right;
#endif
            L = L > +1.0 ? +1.0 : (L < -1.0 ? -1.0 : L);
            R = R > +1.0 ? +1.0 : (R < -1.0 ? -1.0 : R);

            sampbuf[i++] = (int16_t)(INT16_MAX * L + 0.5);
            sampbuf[i++] = (int16_t)(INT16_MAX * R + 0.5);
        }
    }
}

bool Emulator::InitChip(uint8_t chipIndex)
{
#ifdef USE_NEW_AY8910
//    if (m_ay[chipIndex].Configure(chip.freqValue(), k_sample_rate, chip.model() == Chip::Model::YM2149))
    if (m_ay[chipIndex].Configure(chip.freqValue(), k_sample_rate))
    {
        switch (chip.channels())
        {
        case Chip::Channels::MONO:
            m_ay[chipIndex].SetPan(0, 0.5, true);
            m_ay[chipIndex].SetPan(1, 0.5, true);
            m_ay[chipIndex].SetPan(2, 0.5, true);
            break;

        case Chip::Channels::ACB:
            m_ay[chipIndex].SetPan(0, 0.1, true);
            m_ay[chipIndex].SetPan(1, 0.9, true);
            m_ay[chipIndex].SetPan(2, 0.5, true);
            break;

        default: // ABC
            m_ay[chipIndex].SetPan(0, 0.1, true);
            m_ay[chipIndex].SetPan(1, 0.5, true);
            m_ay[chipIndex].SetPan(2, 0.9, true);
            break;
        }
        return true;
    }
#else
    ayumi* ay = &m_ay[chipIndex];
    if (ayumi_configure(ay, (chip.model() == Chip::Model::YM2149), chip.freqValue(), k_sample_rate))
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
#endif
    return false;
}

void Emulator::WriteToChip(uint8_t chip, const Frame& frame, bool force)
{
#ifdef USE_NEW_AY8910
    for (uint8_t reg = 0; reg < 16; ++reg)
    {
        if (force || frame.IsChanged(reg))
        {
            m_ay[chip].Write(reg, frame.Read(chip, reg));
        }
    }
#else
    ayumi* ay = &m_ay[chip];

    uint8_t r7 = frame.Read(chip, Mixer);
    uint8_t r8 = frame.Read(chip, A_Volume);
    uint8_t r9 = frame.Read(chip, B_Volume);
    uint8_t rA = frame.Read(chip, C_Volume);

    ayumi_set_tone(ay, 0, frame.ReadPeriod(chip, A_Period));
    ayumi_set_tone(ay, 1, frame.ReadPeriod(chip, B_Period));
    ayumi_set_tone(ay, 2, frame.ReadPeriod(chip, C_Period));

    ayumi_set_noise(ay, frame.Read(chip, N_Period));

    ayumi_set_mixer(ay, 0, r7 >> 0 & 0x01, r7 >> 3 & 0x01, r8 >> 4);
    ayumi_set_mixer(ay, 1, r7 >> 1 & 0x01, r7 >> 4 & 0x01, r9 >> 4);
    ayumi_set_mixer(ay, 2, r7 >> 2 & 0x01, r7 >> 5 & 0x01, rA >> 4);

    ayumi_set_volume(ay, 0, r8 & 0x0F);
    ayumi_set_volume(ay, 1, r9 & 0x0F);
    ayumi_set_volume(ay, 2, rA & 0x0F);

    ayumi_set_envelope(ay, frame.ReadPeriod(chip, E_Period));
    if (force || frame.IsChanged(chip, E_Shape))
    {
        ayumi_set_envelope_shape(ay, frame.Read(chip, E_Shape));
    }
#endif
}

