#include <thread>
#include "Emulator.h"
#include "stream/Stream.h"

Emulator::Emulator()
{
}

Emulator::~Emulator()
{
    CloseDevice();
}

const std::string Emulator::GetDeviceName() const
{
    return "Emulator";
}

bool Emulator::OpenDevice()
{
    if (WaveAudio::Open(k_emulatorSampleRate, 100, 2, 2))
    {
        // make some delay for warm up the wave audio
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }
    return false;
}

bool Emulator::ConfigureChip(const Chip& schip, Chip& dchip)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // configure model of the 1st destination chip
    if (dchip.first.model() == Chip::Model::Compatible)
    {
        switch (schip.first.model())
        {
        case Chip::Model::AY8930:
        case Chip::Model::YM2149:
            dchip.first.model(schip.first.model());
            break;
        default:
            dchip.first.model(Chip::Model::AY8910);
            break;
        }
    }

    // configure model of the 2nd destination chip
    if (schip.second.modelKnown())
    {
        if (!dchip.second.modelKnown() || dchip.second.model() == Chip::Model::Compatible)
        {
            switch (schip.second.model())
            {
            case Chip::Model::AY8910:
            case Chip::Model::AY8930:
            case Chip::Model::YM2149:
                dchip.second.model(schip.second.model());
                break;
            default:
                dchip.second.model(dchip.first.model());
                break;
            }
        }
    }
    else
    {
        dchip.second.model(Chip::Model::Unknown);
    }

    // configure clock rate, output type and stereo type of destination chip
    if (!dchip.clockKnown ()) dchip.clock (schip.clockKnown () ? schip.clock () : Chip::Clock::F1750000);
    if (!dchip.outputKnown()) dchip.output(schip.outputKnown() ? schip.output() : Chip::Output::Stereo);
    if (!dchip.stereoKnown()) dchip.stereo(schip.stereoKnown() ? schip.stereo() : Chip::Stereo::ABC);

    // create sound chip emulator instances
    for (int chip = 0; chip < dchip.count(); ++chip)
    {
        // create sound chip emulator by model
        switch (dchip.model(chip))
        {
        case Chip::Model::AY8910: m_ay[chip].reset(new ChipAY8910(dchip.clockValue(), k_emulatorSampleRate)); break;
        case Chip::Model::YM2149: m_ay[chip].reset(new ChipYM2149(dchip.clockValue(), k_emulatorSampleRate)); break;
        case Chip::Model::AY8930: m_ay[chip].reset(new ChipAY8930(dchip.clockValue(), k_emulatorSampleRate)); break;
        }
        if (!m_ay[chip]) return false;
        
        // initialize sound chip emulator
        m_ay[chip]->Reset();
        if (dchip.output() == Chip::Output::Mono)
        {
            m_ay[chip]->SetPan(0, 0.5, false);
            m_ay[chip]->SetPan(1, 0.5, false);
            m_ay[chip]->SetPan(2, 0.5, false);
        }
        else
        {
            m_ay[chip]->SetPan(0, 0.1, false);
            m_ay[chip]->SetPan(1, 0.5, false);
            m_ay[chip]->SetPan(2, 0.9, false);
        }        
    }
    return true;
}

bool Emulator::WriteToChip(int chip, const Data& data)
{
    for (const auto& pair : data)
    {
        const uint8_t& reg = pair.first;
        const uint8_t& val = pair.second;
        m_ay[chip]->Write(reg, val);
    }
    return true;
}

void Emulator::FillBuffer(unsigned char* buffer, unsigned long size)
{
    if (m_ay[0])
    {
        // buffer format must be 2 ch x 16 bit
        auto sampbuf = (int16_t*)buffer;
        auto samples = (int)(size / sizeof(int16_t));

        if (m_ay[1])
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

void Emulator::CloseDevice()
{
    WaveAudio::Close();
    m_ay[0].reset();
    m_ay[1].reset();
}
