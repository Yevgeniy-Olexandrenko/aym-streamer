#include <thread>
#include "Emulator.h"
#include "stream/Stream.h"

namespace
{
    const int k_sample_rate = 44100;
}

Emulator::Emulator()
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
    if (!m_ay[0]) return;

    // buffer format must be 2 ch x 16 bit
    auto sampbuf = (int16_t*)buffer;
    auto samples = (int)(size / sizeof(int16_t));

    if (chip.count() == Chip::Count::TwoChips)
    {
        for (int i = 0; i < samples;)
        {
            m_ay[0]->Process();
            m_ay[1]->Process();
            m_ay[0]->RemoveDC();
            m_ay[1]->RemoveDC();

            double L = 0.5 * (m_ay[0]->GetOutL() + m_ay[1]->GetOutL());
            double R = 0.5 * (m_ay[0]->GetOutR() + m_ay[1]->GetOutR());

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
            m_ay[0]->Process();
            m_ay[0]->RemoveDC();

            double L = 0.5 * m_ay[0]->GetOutL();
            double R = 0.5 * m_ay[0]->GetOutR();

            L = L > +1.0 ? +1.0 : (L < -1.0 ? -1.0 : L);
            R = R > +1.0 ? +1.0 : (R < -1.0 ? -1.0 : R);

            sampbuf[i++] = (int16_t)(INT16_MAX * L + 0.5);
            sampbuf[i++] = (int16_t)(INT16_MAX * R + 0.5);
        }
    }
}

bool Emulator::InitChip(uint8_t chipIndex)
{
    switch (chip.model())
    {
    case Chip::Model::AY8910: m_ay[chipIndex].reset(new ChipAY8910(chip.freqValue(), k_sample_rate)); break;
    case Chip::Model::YM2149: m_ay[chipIndex].reset(new ChipYM2149(chip.freqValue(), k_sample_rate)); break;
    case Chip::Model::AY8930: m_ay[chipIndex].reset(new ChipAY8930(chip.freqValue(), k_sample_rate)); break;
    }

    if (m_ay[chipIndex])
    {
        m_ay[chipIndex]->Reset();
        switch (chip.channels())
        {
        case Chip::Channels::MONO:
            m_ay[chipIndex]->SetPan(0, 0.5, true);
            m_ay[chipIndex]->SetPan(1, 0.5, true);
            m_ay[chipIndex]->SetPan(2, 0.5, true);
            break;

        case Chip::Channels::ACB:
            m_ay[chipIndex]->SetPan(0, 0.1, true);
            m_ay[chipIndex]->SetPan(1, 0.9, true);
            m_ay[chipIndex]->SetPan(2, 0.5, true);
            break;

        default: // ABC
            m_ay[chipIndex]->SetPan(0, 0.1, true);
            m_ay[chipIndex]->SetPan(1, 0.5, true);
            m_ay[chipIndex]->SetPan(2, 0.9, true);
            break;
        }
        return true;
    }
    return false;
}

void Emulator::WriteToChip(uint8_t chip, const Frame& frame, bool force)
{
    for (int reg = 0x00; reg <= 0x0F; ++reg)
    {
        if (force || frame.IsChanged(reg))
        {
            m_ay[chip]->Write(reg, frame.Read(chip, reg));
        }
    }

    for (int reg = 0x10; reg <= 0x1F; ++reg)
    {
        if (reg == 0x1d) continue;

        if (force || frame.IsChanged(reg))
        {
            m_ay[chip]->Write(reg, frame.Read(chip, reg));
        }
    }
}

