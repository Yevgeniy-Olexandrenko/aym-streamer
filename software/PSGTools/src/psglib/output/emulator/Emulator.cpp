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

bool Emulator::InitDstChip(const Chip& srcChip, Chip& dstChip)
{
    std::lock_guard<std::mutex> lock(m_mutex);

#if AY8930_FORCE_TO_CHOOSE
    dstChip.model(Chip::Model::AY8930);
#else
    switch (srcChip.model())
    {
#if !AY8930_FORCE_TO_DISCARD
    case Chip::Model::AY8930:
#endif
    case Chip::Model::YM2149:
        dstChip.model(srcChip.model());
        break;
    default:
        dstChip.model(Chip::Model::AY8910);
        break;
    }
#endif
    dstChip.count(srcChip.count());
    dstChip.clock(srcChip.clockKnown() ? srcChip.clock() : Chip::Clock::F1750000);
    dstChip.output(srcChip.outputKnown() ? srcChip.output() : Chip::Output::Stereo);
    dstChip.stereo(srcChip.stereoKnown() ? srcChip.stereo() : Chip::Stereo::ABC);

    for (int chip = 0; chip < dstChip.countValue(); ++chip)
    {
        // create chip emulator
        switch (dstChip.model())
        {
        case Chip::Model::AY8910: m_ay[chip].reset(new ChipAY8910(dstChip.clockValue(), k_emulatorSampleRate)); break;
        case Chip::Model::YM2149: m_ay[chip].reset(new ChipYM2149(dstChip.clockValue(), k_emulatorSampleRate)); break;
        case Chip::Model::AY8930: m_ay[chip].reset(new ChipAY8930(dstChip.clockValue(), k_emulatorSampleRate)); break;
        }
        if (!m_ay[chip]) return false;
        
        // initialize chip emulator
        m_ay[chip]->Reset();
        if (dstChip.output() == Chip::Output::Mono)
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
    }
    return true;
}

bool Emulator::WriteToChip(int chip, const std::vector<uint8_t>& data)
{
    uint8_t reg, val;
    const uint8_t* dataPtr = data.data();

    while ((reg = *dataPtr++) != 0xFF)
    {
        val = *dataPtr++;
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

const std::string Emulator::GetDeviceName() const
{
    return "Emulator";
}

void Emulator::CloseDevice()
{
    WaveAudio::Close();
    m_ay[0].reset();
    m_ay[1].reset();
}
