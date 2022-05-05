#include "DecodeVGM.h"
#include "zlib/zlib.h"
#include <iostream>

bool DecodeVGM::Open(Stream& stream)
{
    Header header;
    if (ReadFile(stream.file.string().c_str(), (uint8_t*)(&header), sizeof(header)))
    {
        m_isAY38910 = (header.version >= 0x151 && header.ay8910Clock > 0);
        m_isRP2A03  = (header.version >= 0x161 && header.nesApuClock > 0);

        if (header.ident == 0x206D6756 && (m_isAY38910 || m_isRP2A03))
        {
            int fileSize = 0x04 + header.eofOffset;
            m_rawData = new uint8_t[fileSize];

            if (ReadFile(stream.file.string().c_str(), m_rawData, fileSize))
            {
                uint32_t vgmDataOffset = 0x40;
                if (header.version >= 0x150 && header.vgmDataOffset)
                {
                    vgmDataOffset = header.vgmDataOffset + 0x34;
                }
                m_dataPtr = m_rawData + vgmDataOffset;

                int frameRate = 60;// DetectFrameRate();
                stream.playback.frameRate(frameRate);
                stream.info.type("VGM stream");

                if (m_isAY38910)
                {
                    bool divider = (header.ay8910Flags & 0x10);
                    bool ym_chip = (header.ay8910Type & 0xF0);
                    stream.chip.model(ym_chip ? Chip::Model::YM : Chip::Model::AY);
                    stream.chip.freqValue(header.ay8910Clock / (divider ? 2 : 1));
                }

                else if (m_isRP2A03)
                {
                    stream.chip.model(Chip::Model::YM);
                    stream.chip.freqValue(header.nesApuClock);
                    //stream.chip.count(Chip::Count::TurboSound);
                }

                m_samplesPerFrame = (44100 / frameRate);
                m_processedSamples = 0;


                if (header.loopSamples)
                {
                    m_loop = (header.totalSamples - header.loopSamples) / m_samplesPerFrame;
                }
                return true;
            }
            else
            {
                delete[] m_rawData;
            }
        }
    }
    return false;
}

bool DecodeVGM::Decode(Frame& frame)
{
    if (m_processedSamples >= m_samplesPerFrame)
    {
        VgmUpdateChips(frame, m_samplesPerFrame);
    }

    else while (m_processedSamples < m_samplesPerFrame)
    {
        int samples = VgmDecodeBlock(frame);
        if (samples)
        {
            VgmUpdateChips(frame, samples);
            m_processedSamples += samples;
        }
        else
        {
            return false;
        }
    }

    m_processedSamples -= m_samplesPerFrame;
    return true;
}

void DecodeVGM::Close(Stream& stream)
{
    //if (m_isRP2A03)
    //{
    //    RP2A03_FixVolume(stream);
    //}

    if (m_loop) stream.loop.frameId(m_loop);
    delete[] m_rawData;
}

////////////////////////////////////////////////////////////////////////////////

int DecodeVGM::VgmDecodeBlock(Frame& frame)
{
    int samples = 0;
    while (!samples)
    {
        // AY8910, write value dd to register aa
        if (m_dataPtr[0] == 0xA0)
        {
            if (m_isAY38910)
            {
                uint8_t aa = m_dataPtr[1];
                uint8_t dd = m_dataPtr[2];
                frame.Update(aa, dd);
            }
            m_dataPtr += 2;
        }

        // RP2A03, write value dd to register aa
        else if (m_dataPtr[0] == 0xB4)
        {
            if (m_isRP2A03)
            {
                uint8_t aa = m_dataPtr[1];
                uint8_t dd = m_dataPtr[2];
                m_rp2A03.Write(aa, dd);
            }
            m_dataPtr += 2;
        }

        // Data block 0x67 0x66 tt ss ss ss ss, just skip it
        else if (m_dataPtr[0] == 0x67)
        {
            uint32_t dataLength = *(uint32_t*)(&m_dataPtr[3]);
            m_dataPtr += (6 + dataLength);
        }

        // Wait n samples, n can range from 0 to 65535 (approx 1.49 seconds).
        // Longer pauses than this are represented by multiple wait commands.
        else if (m_dataPtr[0] == 0x61)
        {
            samples = *(uint16_t*)(&m_dataPtr[1]);
            m_dataPtr += 2;
        }

        // Wait 735 samples (60th of a second), a shortcut for 0x61 0xdf 0x02
        else if (m_dataPtr[0] == 0x62)
        {
            samples = 735;
        }

        // Wait 882 samples (50th of a second), a shortcut for 0x61 0x72 0x03
        else if (m_dataPtr[0] == 0x63)
        {
            samples = 882;
        }

        // End of sound data
        else if (m_dataPtr[0] == 0x66)
        {
            break;
        }

        // Wait n+1 samples, n can range from 0 to 15.
        else if ((m_dataPtr[0] & 0xF0) == 0x70)
        {
            samples = (1 + (m_dataPtr[0] & 0x0F));
        }

        // Unknown command, stop decoding
        else
        {
            break;
        }
        m_dataPtr++;
    }
    return samples;
}

void DecodeVGM::VgmUpdateChips(Frame& frame, int samples)
{
    if (m_isRP2A03)
    {
        m_rp2A03.Update(samples);
        RP2A03_Convert(frame);
    }
}

bool DecodeVGM::ReadFile(const char* path, uint8_t* dest, int size)
{
    if (path && dest && size)
    {
        int read = 0;
        if (gzFile file = gzopen(path, "rb"))
        {
            read = gzread(file, dest, size);
            gzclose(file);
        }
        return (read == size);

    }
    return false;
}

void DecodeVGM::RP2A03_Convert(Frame& frame)
{
    // process
    uint8_t  a1_volume = m_rp2A03.m_chan[0].output;
    uint16_t a1_period = m_rp2A03.m_chan[0].period >> 8;
    uint8_t  a1_enable = bool(m_rp2A03.m_chan[0].output > 0);
    uint8_t  a1_mode = m_rp2A03.m_chan[0].duty;

    uint8_t  c2_volume = m_rp2A03.m_chan[1].output;
    uint16_t c2_period = m_rp2A03.m_chan[1].period >> 8;
    uint8_t  c2_enable = bool(m_rp2A03.m_chan[1].output > 0);
    uint8_t  c2_mode   = m_rp2A03.m_chan[1].duty;

    uint16_t bt_period = m_rp2A03.m_chan[2].period >> (8 + 2);
    uint8_t  bt_enable = bool(m_rp2A03.m_chan[2].output > 0);
    if (!bt_enable || !bt_period) bt_period = 0xFFFF;

    uint8_t  bn_volume = m_rp2A03.m_chan[3].output << 0;
    uint16_t bn_period = m_rp2A03.m_chan[3].period >> (8 + 6);
    uint8_t  bn_enable = bool(m_rp2A03.m_chan[3].output > 0);
    if (bn_volume > 0x0F) bn_volume = 0x0F;

    // output
    uint8_t mixer1 = 0x3F;
    uint8_t mixer2 = 0x3F;

    if (a1_enable)
    {
        uint16_t a1_period_m = a1_period;
        if (a1_mode == 0) a1_period_m ^= 1;
        if (a1_mode == 1 || a1_mode == 3) a1_period_m >>= 1;
        uint8_t  a1_volume_m = (a1_volume ? a1_volume - 1 : a1_volume);

        // chip 0 ch A
        frame.Update(0, TonA_PeriodL, a1_period & 0xFF);
        frame.Update(0, TonA_PeriodH, a1_period >> 8 & 0x0F);
        frame.Update(0, VolA_EnvFlg,  a1_volume);

        // chip 1 ch A
        frame.Update(1, TonA_PeriodL, a1_period_m & 0xFF);
        frame.Update(1, TonA_PeriodH, a1_period_m >> 8 & 0x0F);
        frame.Update(1, VolA_EnvFlg,  a1_volume_m);

        if (a1_volume > m_maxVol[0]) m_maxVol[0] = a1_volume;
    }
    mixer1 &= ~(a1_enable << 0);
    mixer2 &= ~(a1_enable << 0);

    if (c2_enable)
    {
        uint16_t c2_period_m = c2_period;
        if (c2_mode == 0) c2_period_m ^= 1;
        if (c2_mode == 1 || c2_mode == 3) c2_period_m >>= 1;
        uint8_t  c2_volume_m = (c2_volume ? c2_volume - 1 : c2_volume);

        // chip 0 ch C
        frame.Update(0, TonC_PeriodL, c2_period & 0xFF);
        frame.Update(0, TonC_PeriodH, c2_period >> 8 & 0x0F);
        frame.Update(0, VolC_EnvFlg,  c2_volume);
        
        // chip 1 ch C
        frame.Update(1, TonC_PeriodL, c2_period_m & 0xFF);
        frame.Update(1, TonC_PeriodH, c2_period_m >> 8 & 0x0F);
        frame.Update(1, VolC_EnvFlg,  c2_volume_m);

        if (c2_volume > m_maxVol[1]) m_maxVol[1] = c2_volume;
    }
    mixer1 &= ~(c2_enable << 2);
    mixer2 &= ~(c2_enable << 2);

    // chip 0 ch B
    frame.Update(0, Env_PeriodL, bt_period & 0xFF);
    frame.Update(0, Env_PeriodH, bt_period >> 8 & 0xFF);
    frame.Update(0, VolB_EnvFlg, 0x10);
    if (frame.Read(0, Env_Shape) != 0x0E)
    {
        frame.Write(0, Env_Shape, 0x0E);
    }

    if (bn_enable)
    {
        // chip 1 ch B
        frame.Update(1, Noise_Period, bn_period & 0x1F);
        frame.Update(1, VolB_EnvFlg,  bn_volume);

        if (bn_volume > m_maxVol[2]) m_maxVol[2] = bn_volume;
    }
    mixer2 &= ~(bn_enable << 4);
    
    // mixers
    frame.Update(0, Mixer_Flags, mixer1);
    frame.Update(1, Mixer_Flags, mixer2);
}

void DecodeVGM::RP2A03_FixVolume(Stream& stream)
{
#if 1
    float pulseVolFactor = 15.0f / std::max(m_maxVol[0], m_maxVol[1]);
    float noiseVolFactor = 15.0f / m_maxVol[2];

    for (int i = 0, c = stream.frames.count(); i < c; ++i)
    {
        Frame& frame = const_cast<Frame&>(stream.frames.get(i));

        // chip 0
        frame.data(0, VolA_EnvFlg) *= pulseVolFactor;
        frame.data(0, VolC_EnvFlg) *= pulseVolFactor;

        // chip 1
        frame.data(1, VolA_EnvFlg) *= pulseVolFactor;
        frame.data(1, VolB_EnvFlg) *= noiseVolFactor;
        frame.data(1, VolC_EnvFlg) *= pulseVolFactor;
    }
#else
    int pulseVolDelta = (0x0F - std::max(m_maxVol[0], m_maxVol[1]));
    int noiseVolDelta = (0x0F - m_maxVol[2]);

    for (int i = 0, c = stream.frames.count(); i < c; ++i)
    {
        Frame& frame = const_cast<Frame&>(stream.frames.get(i));

        // chip 0
        if (frame.data(0, VolA_EnvFlg) < 0x0F)
            frame.data(0, VolA_EnvFlg) += pulseVolDelta;

        if (frame.data(0, VolC_EnvFlg) < 0x0F)
            frame.data(0, VolC_EnvFlg) += pulseVolDelta;

        // chip 1
        if (frame.data(1, VolA_EnvFlg) < 0x0F)
            frame.data(1, VolA_EnvFlg) += pulseVolDelta;

        if (frame.data(1, VolB_EnvFlg) < 0x0F)
            frame.data(1, VolB_EnvFlg) += noiseVolDelta;

        if (frame.data(1, VolC_EnvFlg) < 0x0F)
            frame.data(1, VolC_EnvFlg) += pulseVolDelta;
    }
#endif
}
