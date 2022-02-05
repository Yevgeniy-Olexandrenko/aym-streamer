#include "AY38910.h"
#include "../../module/Frame.h"

static const int is_ym = 0;
static const double clock_rate = 1750000;
static const int sample_rate = 48000;
static const bool dc_filter_on = !false;

//
//t->sample_rate = 44100;
//t->eqp_stereo_on = 0;
//t->dc_filter_on = 1;
//t->is_ym = 1;
//t->clock_rate = 2000000;
//t->frame_rate = 50;


AY38910::AY38910()
    : WaveAudio()
    , m_isOpened(false)
{
}

AY38910::~AY38910()
{
    Close();
}

void AY38910::Open()
{
    m_isOpened = ayumi_configure(&m_ay, is_ym, clock_rate, sample_rate);
    if (m_isOpened)
    {
        ayumi_set_pan(&m_ay, 0, 0.1, false);
        ayumi_set_pan(&m_ay, 1, 0.5, false);
        ayumi_set_pan(&m_ay, 2, 0.9, false);

        WaveAudio::Start();
        m_isOpened &= m_working;
    }
}

bool AY38910::IsOpened() const
{
    return m_isOpened;
}

bool AY38910::OutFrame(const Frame& frame, bool force)
{
    if (m_isOpened)
    {
        if (force || frame[TonA_PeriodL].IsChanged() || frame[TonA_PeriodH].IsChanged())
        {
            uint16_t period = frame[TonA_PeriodL].GetData() | frame[TonA_PeriodH].GetData() << 8;
            ayumi_set_tone(&m_ay, 0, period);
        }

        if (force || frame[TonB_PeriodL].IsChanged() || frame[TonB_PeriodH].IsChanged())
        {
            uint16_t period = frame[TonB_PeriodL].GetData() | frame[TonB_PeriodH].GetData() << 8;
            ayumi_set_tone(&m_ay, 1, period);
        }

        if (force || frame[TonC_PeriodL].IsChanged() || frame[TonC_PeriodH].IsChanged())
        {
            uint16_t period = frame[TonC_PeriodL].GetData() | frame[TonC_PeriodH].GetData() << 8;
            ayumi_set_tone(&m_ay, 2, period);
        }

        if (force || frame[Noise_Period].IsChanged())
        {
            ayumi_set_noise(&m_ay, frame[Noise_Period].GetData());
        }

        if (force || frame[Mixer_Flags].IsChanged() || frame[VolA_EnvFlg].IsChanged())
        {
            uint8_t r7 = frame[Mixer_Flags].GetData();
            uint8_t r8 = frame[VolA_EnvFlg].GetData();
            ayumi_set_mixer(&m_ay, 0, r7 & 0x01, r7 >> 3 & 0x01, r8 >> 4);
            ayumi_set_volume(&m_ay, 0, r8 & 0x0F);
        }

        if (force || frame[Mixer_Flags].IsChanged() || frame[VolB_EnvFlg].IsChanged())
        {
            uint8_t r7 = frame[Mixer_Flags].GetData();
            uint8_t r9 = frame[VolB_EnvFlg].GetData();
            ayumi_set_mixer(&m_ay, 1, r7 >> 1 & 0x01, r7 >> 4 & 0x01, r9 >> 4);
            ayumi_set_volume(&m_ay, 1, r9 & 0x0F);
        }

        if (force || frame[Mixer_Flags].IsChanged() || frame[VolC_EnvFlg].IsChanged())
        {
            uint8_t r7 = frame[Mixer_Flags].GetData();
            uint8_t rA = frame[VolC_EnvFlg].GetData();
            ayumi_set_mixer(&m_ay, 2, r7 >> 2 & 0x01, r7 >> 5 & 0x01, rA >> 4);
            ayumi_set_volume(&m_ay, 2, rA & 0x0F);
        }

        if (force || frame[Env_PeriodL].IsChanged() || frame[Env_PeriodH].IsChanged())
        {
            uint16_t period = frame[Env_PeriodL].GetData() | frame[Env_PeriodH].GetData() << 8;
            ayumi_set_envelope(&m_ay, period);
        }

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
    uint16_t* buf = (uint16_t*)buffer;
    uint32_t sz = size / sizeof(uint16_t);

    for (uint32_t i = 0; i < sz; ++i)
    {
        ayumi_process(&m_ay);
        if (dc_filter_on) {
            ayumi_remove_dc(&m_ay);
        }

        float volume = 32767;
        float out_0 = (float)(m_ay.left * volume);
        float out_1 = (float)(m_ay.right * volume);

        buf[i] = uint16_t(0.5f * (out_0 + out_1));

        //std::cout << int(out_0) << "\t";
    }
}
