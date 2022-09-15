#include "DecodeVGM.h"
#include "zlib.h"
#include "decoders/chipsims/SimAY8910.h"
#include "decoders/chipsims/SimRP2A03.h"
#include "decoders/chipsims/SimSN76489.h"
#include <sstream>

////////////////////////////////////////////////////////////////////////////////

// debug output
#if DBG_DECODE_VGM
std::ofstream debug_out;
#define DebugOpen() \
    debug_out.open("dbg_decode_vgm.txt");
#define DebugPrintWrite(aa, dd) \
    debug_out << 'r' << std::hex << std::setw(2) << std::setfill('0') << int(aa); \
    debug_out << ':' << std::hex << std::setw(2) << std::setfill('0') << int(dd); \
    debug_out << ' ';
#define DebugPrintNewLine() \
    debug_out << std::endl;
#define DebugClose() \
    debug_out.close();
#else
#define DebugOpen()
#define DebugPrintWrite(aa, bb)
#define DebugPrintNewLine()
#define DebugClose()
#endif

////////////////////////////////////////////////////////////////////////////////

namespace
{
    const uint32_t VGMSignature = 0x206D6756;
    const uint32_t GD3Signature = 0x20336447;

    bool ReadFile(const char* path, uint8_t* dest, int size)
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
}

////////////////////////////////////////////////////////////////////////////////

bool DecodeVGM::Open(Stream& stream)
{
    Header header;
    if (ReadFile(stream.file.string().c_str(), (uint8_t*)(&header), sizeof(header)))
    {
        if (header.sn76489Clock) m_simulator.reset(new SimSN76489());
        if (header.version >= 0x151 && header.ay8910Clock) m_simulator.reset(new SimAY8910());
        if (header.version >= 0x161 && header.nesApuClock) m_simulator.reset(new SimRP2A03());

        if (header.ident == VGMSignature && m_simulator)
        {
            int fileSize = 0x04 + header.eofOffset;
            m_data = new uint8_t[fileSize];

            if (ReadFile(stream.file.string().c_str(), m_data, fileSize))
            {
                uint32_t dataOffset = 0x40;
                if (header.version >= 0x150 && header.vgmDataOffset)
                {
                    dataOffset = header.vgmDataOffset + 0x34;
                }
                m_dataPtr = m_data + dataOffset;

                int clockRate = 0;
                int frameRate = 60;

                if (m_simulator->type() == ChipSim::Type::SN76489)
                {
                    clockRate = (header.sn76489Clock & 0x3FFFFFFF);
                    auto twoChips = bool(header.sn76489Clock & 0x40000000);

                    Chip::Model model = Chip::Model::AY8910;
                    stream.schip.first.model(model);
                    stream.schip.clockValue(clockRate / 2);
                    if (twoChips) stream.schip.second.model(model);
                    //stream.schip.output(Chip::Output::Mono);

                    // TODO
                }
                
                else if (m_simulator->type() == ChipSim::Type::AY8910)
                {
                    clockRate = (header.ay8910Clock & 0x3FFFFFFF);
                    if (header.ay8910Flags & 0x10) clockRate /= 2;
                    // TODO: detect frame rate

                    // detect chip model and count
                    Chip::Model model = Chip::Model::AY8910;
                    if (header.ay8910Type == 0x03) model = Chip::Model::AY8930;
                    if (header.ay8910Type >= 0x10) model = Chip::Model::YM2149;
                    auto twoChips = bool(header.ay8910Clock & 0x40000000);
          
                    // setup chip model, clock rate and 2nd chip if needed
                    stream.schip.first.model(model);
                    stream.schip.clockValue(clockRate);
                    if (twoChips) stream.schip.second.model(model);
                }

                else if (m_simulator->type() == ChipSim::Type::RP2A03)
                {
                    clockRate = (header.nesApuClock & 0x3FFFFFFF);
                    if (clockRate / 1000 == 1662) frameRate = 50;

                    // setup chip clock rate
                    if (stream.dchip.clockKnown())
                        stream.schip.clock(stream.dchip.clock());
                    else
                        stream.schip.clockValue(clockRate);

                    // setup first chip (YM2149 used by default)
                    if (stream.dchip.first.model() != Chip::Model::Compatible)
                        stream.schip.first.model(stream.dchip.first.model());
                    else
                        stream.schip.first.model(Chip::Model::YM2149);

                    // setup output Strereo or Mono (Mono used by default)
                    if (stream.dchip.outputKnown())
                        stream.schip.output(stream.dchip.output());
                    else
                        stream.schip.output(Chip::Output::Mono);

                    // configure simulator output type and update chip if required
                    auto outputType = SimRP2A03::OutputType::SingleChip;
                    if (stream.dchip.first.model() == Chip::Model::AY8930)
                    {
                        outputType = SimRP2A03::OutputType::AY8930Chip;
                        stream.schip.first.model(Chip::Model::AY8930);
                    }
                    else if (stream.dchip.count() == 2)
                    {
                        outputType = SimRP2A03::OutputType::DoubleChip;
                        stream.schip.second.model(stream.schip.first.model());
                    }
                    static_cast<SimRP2A03*>(m_simulator.get())->ConfigureOutput(outputType);
                }

                stream.info.type("VGM stream");
                stream.play.frameRate(frameRate);

                m_simulator->ConfigureClock(clockRate, stream.schip.clockValue());
                m_samplesPerFrame = (44100 / frameRate);
                m_processedSamples = 0;

                if (header.loopSamples)
                {
                    m_loop = (header.totalSamples - header.loopSamples) / m_samplesPerFrame;
                }

                // GD3 tags reading
                if (uint8_t* gd3Offset = m_data + header.gd3Offset)
                {
                    uint32_t gd3 = *(uint32_t*)(gd3Offset + 0x14);
                    uint32_t ver = *(uint32_t*)(gd3Offset + 0x18);

                    if (gd3 == GD3Signature && ver == 0x00000100)
                    {
                        gd3Offset += 0x20;

                        auto ReadString = [&]() -> std::string
                        {
                            std::stringstream ss;
                            while (true)
                            {
                                wchar_t ch = *(wchar_t*)gd3Offset;
                                gd3Offset += sizeof(wchar_t);

                                if (!ch) break;
                                ss << (ch < 0x20 || ch > 0x7F ? '?' : char(ch));
                            }
                            return ss.str();
                        };

                        std::string GD3[]{
                            ReadString(), ReadString(), // Track name
                            ReadString(), ReadString(), // Game name
                            ReadString(), ReadString(), // System name
                            ReadString(), ReadString(), // Name of Original Track Author
                            ReadString(),               // Date of game's release written in the form yyyy/mm/dd, or just yyyy/mm or yyyy
                            ReadString(),               // Name of person who converted it to a VGM file
                            ReadString()                // Notes
                        };

                        stream.info.title(GD3[0]);
                        stream.info.artist(GD3[6]);
                        stream.info.comment(GD3[2]);

                        if (!GD3[4].empty())
                            stream.info.type(stream.info.type() + " (" + GD3[4] + ")");
                    }
                }
                DebugOpen();
                return true;
            }
            else
            {
                delete[] m_data;
            }
        }
    }
    return false;
}

bool DecodeVGM::Decode(Frame& frame)
{
    if (m_processedSamples >= m_samplesPerFrame)
    {
        m_simulator->Simulate(m_samplesPerFrame);
        m_simulator->Convert(frame);
    }

    else while (m_processedSamples < m_samplesPerFrame)
    {
        if (int samples = DecodeBlock())
        {
            m_simulator->Simulate(samples);
            m_simulator->Convert(frame);
            m_processedSamples += samples;
        }
        else
        {
            return false;
        }
    }

    m_processedSamples -= m_samplesPerFrame;
    DebugPrintNewLine();
    return true;
}

void DecodeVGM::Close(Stream& stream)
{
    stream.loop.frameId(m_loop);
    delete[] m_data;

    DebugClose();
}

////////////////////////////////////////////////////////////////////////////////

int DecodeVGM::DecodeBlock()
{
    int samples = 0;
    while (!samples)
    {
        // YM2203 or AY8910 stereo mask
        if (m_dataPtr[0] == 0x31)
        {
            uint8_t dd = m_dataPtr[1];

            // b7.b6.b5.b4.b3.b2.b1.b0
            // II.YY.R3.L3.R2.L2.R1.L1
            // -----------------------
            // II       chip instance(0 or 1)
            // YY       set stereo mask for YM2203 (1) or AY8910 (0)
            // L1/L2/L3 enable channel 1/2/3 on left speaker
            // R1/R2/R3 enable channel 1/2/3 on right speaker

            m_dataPtr += 1;
        }

        // SN76489, write value dd
        else if (m_dataPtr[0] == 0x50)
        {
            if (m_simulator->type() == ChipSim::Type::SN76489)
            {
                uint8_t dd = m_dataPtr[1];
                m_simulator->Write(0, 0, dd);
            }
            m_dataPtr += 1;
        }

        // AY8910, write value dd to register aa
        else if (m_dataPtr[0] == 0xA0)
        {
            if (m_simulator->type() == ChipSim::Type::AY8910)
            {
                uint8_t aa = m_dataPtr[1];
                uint8_t dd = m_dataPtr[2];
                m_simulator->Write((aa & 0x80) >> 7, aa & 0x7F, dd);
                DebugPrintWrite(aa, dd);
            }
            m_dataPtr += 2;
        }

        // RP2A03, write value dd to register aa
        else if (m_dataPtr[0] == 0xB4)
        {
            if (m_simulator->type() == ChipSim::Type::RP2A03)
            {
                uint8_t aa = m_dataPtr[1];
                uint8_t dd = m_dataPtr[2];
                m_simulator->Write(0, aa, dd);
                DebugPrintWrite(aa, dd);
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
