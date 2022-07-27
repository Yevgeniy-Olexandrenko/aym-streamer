#include <thread>
#include "Emulator.h"
#include "stream/Stream.h"

Emulator::Emulator()
    : Output()
    , WaveAudio()
{
}

Emulator::~Emulator()
{
    Close();
}

bool Emulator::Open()
{
    if (!m_isOpened)
    {
        if (WaveAudio::Open(k_emulatorSampleRate, 100, 2, 2))
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
        
#if AY8930_FORCE_TO_CHOOSE
        m_chip.model(Chip::Model::AY8930);
#else
        switch (stream.chip.model())
        {
        case Chip::Model::AY8930:
        case Chip::Model::YM2149:
            m_chip.model(stream.chip.model());
            break;
        default:
            m_chip.model(Chip::Model::AY8910);
            break;
        }
#endif
        m_chip.count(stream.chip.count());
        m_chip.clock(stream.chip.clockKnown() ? stream.chip.clock() : Chip::Clock::F1750000);
        m_chip.output(stream.chip.outputKnown() ? stream.chip.output() : Chip::Output::Stereo);
        m_chip.stereo(stream.chip.stereoKnown() ? stream.chip.stereo() : Chip::Stereo::ABC);

        for (int chip = 0; chip < m_chip.countValue(); ++chip)
        {
            m_isOpened &= InitChip(chip);
        }
    }
    return Output::Init(stream);
}

void Emulator::Close()
{
    WaveAudio::Close();
    m_isOpened = false;

    m_ay[0].reset();
    m_ay[1].reset();
}

bool Emulator::InitChip(int chip)
{
    switch (m_chip.model())
    {
    case Chip::Model::AY8910: m_ay[chip].reset(new ChipAY8910(m_chip.clockValue(), k_emulatorSampleRate)); break;
    case Chip::Model::YM2149: m_ay[chip].reset(new ChipYM2149(m_chip.clockValue(), k_emulatorSampleRate)); break;
    case Chip::Model::AY8930: m_ay[chip].reset(new ChipAY8930(m_chip.clockValue(), k_emulatorSampleRate)); break;
    }

    if (m_ay[chip])
    {
        m_ay[chip]->Reset();
        if (m_chip.output() == Chip::Output::Mono)
        {
            m_ay[chip]->SetPan(0, 0.5, true);
            m_ay[chip]->SetPan(1, 0.5, true);
            m_ay[chip]->SetPan(2, 0.5, true);
        }
        else
        {
            m_ay[chip]->SetPan(0, 0.1, true);
            m_ay[chip]->SetPan(1, 0.5, true);
            m_ay[chip]->SetPan(2, 0.9, true);
        }
        return true;
    }
    return false;
}

void Emulator::WriteToChip(int chip, const std::vector<uint8_t>& data)
{
    uint8_t reg, val;
    const uint8_t* dataPtr = data.data();

    while ((reg = *dataPtr++) != 0xFF)
    {
        val = *dataPtr++;
        m_ay[chip]->Write(reg, val);
    }
}

void Emulator::FillBuffer(unsigned char* buffer, unsigned long size)
{
    if (m_ay[0])
    {
        // buffer format must be 2 ch x 16 bit
        auto sampbuf = (int16_t*)buffer;
        auto samples = (int)(size / sizeof(int16_t));

        if (m_chip.count() == Chip::Count::TwoChips)
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
}

const std::string Emulator::GetOutputDeviceName() const
{
    return "Emulator";
}
