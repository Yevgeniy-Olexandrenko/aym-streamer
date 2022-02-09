#include <thread>
#include "AY38910.h"
#include "module/Module.h"

namespace
{
    const int k_is_ym = true;
    const int k_clock_rate = 1750000;
    const int k_sample_rate = 44100;
}

AY38910::AY38910(const Module& module)
    : Output(module)
    , WaveAudio(k_sample_rate, 100, 2, 2)
    , m_ay{0}
{
    uint32_t clockRate = module.GetChipFreqValue(k_clock_rate);
    m_isOpened = ayumi_configure(&m_ay, k_is_ym, clockRate, k_sample_rate);
}

AY38910::~AY38910()
{
    Close();
}

void AY38910::Open()
{
    if (m_isOpened)
    {
        ayumi_set_pan(&m_ay, 0, 0.1, false);
        ayumi_set_pan(&m_ay, 1, 0.5, false);
        ayumi_set_pan(&m_ay, 2, 0.9, false);

        WaveAudio::Start();
        if (m_isOpened &= m_working)
        {
            // make some delay for wave audio warm up
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool AY38910::OutFrame(const Frame& frame, bool force)
{
    if (m_isOpened)
    {
        uint8_t r7 = frame[Mixer_Flags].GetData();
        uint8_t r8 = frame[VolA_EnvFlg].GetData();
        uint8_t r9 = frame[VolB_EnvFlg].GetData();
        uint8_t rA = frame[VolC_EnvFlg].GetData();

        ayumi_set_tone(&m_ay, 0, frame[TonA_PeriodL].GetData() | frame[TonA_PeriodH].GetData() << 8);
        ayumi_set_tone(&m_ay, 1, frame[TonB_PeriodL].GetData() | frame[TonB_PeriodH].GetData() << 8);
        ayumi_set_tone(&m_ay, 2, frame[TonC_PeriodL].GetData() | frame[TonC_PeriodH].GetData() << 8);

        ayumi_set_noise(&m_ay, frame[Noise_Period].GetData());

        ayumi_set_mixer(&m_ay, 0, r7 >> 0 & 0x01, r7 >> 3 & 0x01, r8 >> 4);
        ayumi_set_mixer(&m_ay, 1, r7 >> 1 & 0x01, r7 >> 4 & 0x01, r9 >> 4);
        ayumi_set_mixer(&m_ay, 2, r7 >> 2 & 0x01, r7 >> 5 & 0x01, rA >> 4);

        ayumi_set_volume(&m_ay, 0, r8 & 0x0F);
        ayumi_set_volume(&m_ay, 1, r9 & 0x0F);
        ayumi_set_volume(&m_ay, 2, rA & 0x0F);

        ayumi_set_envelope(&m_ay, frame[Env_PeriodL].GetData() | frame[Env_PeriodH].GetData() << 8);

        if (force || frame[Env_Shape].IsChanged())
        {
            ayumi_set_envelope_shape(&m_ay, frame[Env_Shape].GetData());
        }
    }
    return m_isOpened;
}

void AY38910::Close()
{
    WaveAudio::Stop();
}

void AY38910::FillBuffer(unsigned char* buffer, unsigned long size)
{
    // buffer format must be 2 ch x 16 bit
    auto sampbuf = (int16_t*)buffer;
    auto samples = (int)(size / sizeof(int16_t));

    for (int i = 0; i < samples;)
    {
        ayumi_process(&m_ay);
        ayumi_remove_dc(&m_ay);

        sampbuf[i++] = (int16_t)(INT16_MAX * m_ay.left);
        sampbuf[i++] = (int16_t)(INT16_MAX * m_ay.right);
    }
}
