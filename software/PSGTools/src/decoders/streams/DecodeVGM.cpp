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

                int frameRate = DetectFrameRate();
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
                m_minSamplesPerFrame = (44100 / (frameRate + 1));
                m_maxSamplesPerFrame = (44100 / (frameRate - 1));
                m_processedSamples = 0;
                m_firstFrame = true;

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
    bool result = false;
    if (m_isAY38910)
    {
        result = VgmDecode(frame);
    }
    else if (m_isRP2A03)
    {
        result = VgmDecode(frame);
        RP2A03Update(frame);
    }

    //if (!result)
    //{
    //    std::cout << "tone1 - 0x" << std::hex << m_maxVol[0] << std::endl;
    //    std::cout << "tone2 - 0x" << std::hex << m_maxVol[1] << std::endl;
    //    std::cout << "noise - 0x" << std::hex << m_maxVol[2] << std::endl;
    //}

    return result;
}

void DecodeVGM::Close(Stream& stream)
{
    if (m_isRP2A03)
    {
        //RP2A03FixVolume(stream);
    }

    if (m_loop) stream.loop.frameId(m_loop);
    delete[] m_rawData;
}

////////////////////////////////////////////////////////////////////////////////

bool DecodeVGM::VgmDecode(Frame& frame)
{
    while (m_processedSamples < m_samplesPerFrame)
    {
        uint16_t waitSamples = 0;

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
                if (aa < 0x20)
                {
                    RP2A03Write(aa, dd);
                }
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
            waitSamples = *(uint16_t*)(&m_dataPtr[1]);
            m_processedSamples += waitSamples;

            if (m_firstFrame || (m_processedSamples >= m_minSamplesPerFrame && m_processedSamples <= m_maxSamplesPerFrame))
            {
                m_samplesPerFrame = m_processedSamples;
                m_firstFrame = false;
            }

            //std::cout << m_processedSamples << ' ';
            m_dataPtr += 2;
        }

        // Wait 735 samples (60th of a second), a shortcut for 0x61 0xdf 0x02
        else if (m_dataPtr[0] == 0x62)
        {
            waitSamples = m_samplesPerFrame = 735;
            m_processedSamples += waitSamples;
            //std::cout << m_processedSamples << ' ';
        }

        // Wait 882 samples (50th of a second), a shortcut for 0x61 0x72 0x03
        else if (m_dataPtr[0] == 0x63)
        {
            waitSamples = m_samplesPerFrame = 882;
            m_processedSamples += waitSamples;
            //std::cout << m_processedSamples << ' ';
        }

        // End of sound data
        else if (m_dataPtr[0] == 0x66)
        {
            return false;
        }

        // Wait n+1 samples, n can range from 0 to 15.
        else if ((m_dataPtr[0] & 0xF0) == 0x70)
        {
            waitSamples = (1 + (m_dataPtr[0] & 0x0F));
            m_processedSamples += waitSamples;
            //std::cout << m_processedSamples << ' ';
        }

        // Unknown command, stop decoding
        else
        {
            return false;
        }
        m_dataPtr++;

        //if (waitSamples)
        //{
        //    if (m_isRP2A03)
        //    {
        //        m_rp2A03.Update(waitSamples);
        //    }
        //    waitSamples = 0;
        //}
    }

   
    
  

    m_processedSamples -= m_samplesPerFrame;
    return true;
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

int DecodeVGM::DetectFrameRate()
{
    uint8_t* dataPtrSaved = m_dataPtr;
    int frameRate = 0;

    m_samplesPerFrame = INT32_MAX;
    m_minSamplesPerFrame = int(44100 / 60 * 0.95f);
    m_maxSamplesPerFrame = int(44100 / 50 * 1.05f);
    m_processedSamples = 0;
    m_firstFrame = false;

    Frame frame;
    float accumulator = 0;
    float counter = 0;

    for (int i = 0; i < 100 && VgmDecode(frame); ++i)
    {
        if (m_samplesPerFrame != INT32_MAX)
        {
            accumulator += m_samplesPerFrame;
            counter++;

            std::cout << int(44100 * counter / accumulator + 0.5f) << ' ';
        }
    }
    std::cout << std::endl;

    if (counter)
    {
        frameRate = int(44100 * counter / accumulator + 0.5f);
        if (frameRate >= 59 && frameRate <= 61) frameRate = 60;
        if (frameRate >= 49 && frameRate <= 51) frameRate = 50;
    }
    else
    {
        // TODO: detect in some another way,
        // set some default value for now
        frameRate = 50;
    }

    m_dataPtr = dataPtrSaved;
    return frameRate;
}

void DecodeVGM::RP2A03Write(uint8_t reg, uint8_t data)
{
    m_rp2A03.Write(reg, data);
}

void DecodeVGM::RP2A03Update(Frame& frame)
{
    // process
    uint8_t  a1_volume = m_rp2A03.m_chan[0].output;
    uint16_t a1_period = m_rp2A03.m_chan[0].period >> 8;
    uint8_t  a1_enable = bool(m_rp2A03.m_chan[0].output > 0);

    uint8_t  c2_volume = m_rp2A03.m_chan[1].output;
    uint16_t c2_period = m_rp2A03.m_chan[1].period >> 8;
    uint8_t  c2_enable = bool(m_rp2A03.m_chan[1].output > 0);

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
        uint16_t a1_period_m = (a1_period + 1);
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
        uint16_t c2_period_m = (c2_period ? c2_period - 1 : c2_period);
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
    frame.Update(0, Env_Shape,   0x0E);

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

    // update
    int samples = 44100 / 60;// m_samplesPerFrame;
    m_rp2A03.Update(samples);
}

void DecodeVGM::RP2A03FixVolume(Stream& stream)
{
    int pulseVolDelta = 0x0F - std::max(m_maxVol[0], m_maxVol[1]);
    int noiseVolDelta = 0x0F - m_maxVol[2];

    for (int i = 0, c = stream.frames.count(); i < c; ++i)
    {
        Frame& frame = const_cast<Frame&>(stream.frames.get(i));

        //if (frame[VolA_EnvFlg].first.changed() && frame[VolA_EnvFlg].first.data() < 0x0F)
        //    frame[VolA_EnvFlg].first.override(frame[VolA_EnvFlg].first.data() + pulseVolDelta);
        //else
        //    frame[VolA_EnvFlg].first = Register(frame[VolA_EnvFlg].first.data() + pulseVolDelta);
        //
        //if (frame[VolA_EnvFlg].second.changed() && frame[VolA_EnvFlg].second.data() < 0x0F)
        //    frame[VolA_EnvFlg].second.override(frame[VolA_EnvFlg].second.data() + pulseVolDelta);
        //else
        //    frame[VolA_EnvFlg].second = Register(frame[VolA_EnvFlg].second.data() + pulseVolDelta);

        //if (frame[VolC_EnvFlg].first.changed() && frame[VolC_EnvFlg].first.data() < 0x0F)
        //    frame[VolC_EnvFlg].first.override(frame[VolC_EnvFlg].first.data() + pulseVolDelta);
        //else
        //    frame[VolC_EnvFlg].first = Register(frame[VolC_EnvFlg].first.data() + pulseVolDelta);

        //if (frame[VolC_EnvFlg].second.changed() && frame[VolC_EnvFlg].second.data() < 0x0F)
        //    frame[VolC_EnvFlg].second.override(frame[VolC_EnvFlg].second.data() + pulseVolDelta);
        //else
        //    frame[VolC_EnvFlg].second = Register(frame[VolC_EnvFlg].second.data() + pulseVolDelta);

        //if (frame[VolB_EnvFlg].second.changed() && frame[VolB_EnvFlg].second.data() < 0x0F)
        //    frame[VolB_EnvFlg].second.override(frame[VolB_EnvFlg].second.data() + noiseVolDelta);
        //else
        //    frame[VolB_EnvFlg].second = Register(frame[VolB_EnvFlg].second.data() + noiseVolDelta);
    }
}
