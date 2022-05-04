#include "DecodeYM.h"
#include "lh5_decode/lh5_decode.h"

uint16_t GetU16(uint8_t* p) { return (p[0] << 8  | p[1]); }
uint32_t GetU32(uint8_t* p) { return (p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]); }

bool DecodeYM::Open(Stream& stream)
{
    bool isDetected = false;
    std::ifstream fileStream;
    fileStream.open(stream.file, std::fstream::binary);

    if (fileStream)
    {
        fileStream.seekg(0, fileStream.end);
        uint32_t fileSize = (uint32_t)fileStream.tellg();

        if (fileSize > sizeof(HeaderLZH))
        {
            HeaderLZH lzh;
            fileStream.seekg(0, fileStream.beg);
            fileStream.read((char*)(&lzh), sizeof(lzh));

            if (!memcmp(lzh.method, "-lh5-", 5))
            {
                uint32_t headerSize = lzh.hdrSize + 2;
                uint32_t packedSize = fileSize - headerSize;
                uint8_t* packedData = new uint8_t[packedSize];
                fileStream.seekg(headerSize, fileStream.beg);
                fileStream.read((char*)packedData, packedSize);

                if (fileStream)
                {
                    uint32_t depackedSize = lzh.unCompSize;
                    m_data = new uint8_t[depackedSize];

                    if (lh5_decode(packedData, m_data, depackedSize, packedSize))
                    {
                        HeaderYM5* ym5 = (HeaderYM5*)m_data;
                        std::string id((char*)ym5->id, 4);

                        if (id == "YM5!" || id == "YM6!")
                        {
                            uint32_t offset = sizeof(HeaderYM5);
                            offset += GetU16(ym5->addSize);
                            
                            for (int i = 0, c = GetU16(ym5->numOfDig); i < c; i++)
                            {
                                uint32_t count = GetU32(&m_data[offset]);
                                offset += (4 + count);
                            }

                            auto p = (char* )(m_data + offset);
                            std::string title(p); p += title.size() + 1;
                            std::string author(p); p += author.size() + 1;
                            std::string comment(p); p += comment.size() + 1;

                            stream.chip.freqValue(GetU32(ym5->chipFreq));
                            stream.playback.frameRate(GetU16(ym5->playFreq));
                            stream.info.title(title);
                            stream.info.artist(author);
                            stream.info.comment(comment);
                            stream.info.type(id.substr(0, 3) + " stream");

                            m_offset = ((uint8_t*)p - m_data);
                            m_frames = GetU32(ym5->numOfFrames);
                            m_loop   = GetU32(ym5->loopFrame);
                            m_frame  = 0;
                            
                            m_interleaved = ((GetU32(ym5->songAttr) & 1) != 0);
                            isDetected = true;
                        }

                        else if (id == "YM2!" || id == "YM3!" || id == "YM3b")
                        {
                            stream.playback.frameRate(50);
                            stream.info.type(id.substr(0, 3) + " stream");

                            m_offset = id.size();
                            m_frames = (depackedSize - id.size()) / 14;
                            m_loop   = 0;
                            m_frame  = 0;

                            if (id[3] == 'b')
                            {
                                // last four bytes (DWORD data) are frame number for looping
                                m_loop = *(uint32_t*)(&m_data[depackedSize - 4]);
                            }

                            m_interleaved = true;
                            isDetected = true;
                        }
                    }
                    else delete[] m_data;
                }
                delete[] packedData;
            }
        }
        fileStream.close();
    }
    return isDetected;
}

bool DecodeYM::Decode(Frame& frame)
{
    uint32_t frameSize = m_interleaved ? 1 : 16;
    uint32_t valueSize = m_interleaved ? m_frames : 1;
    uint8_t* dataPtr = (m_data + m_offset) + (m_frame * frameSize);
    m_frame++;

    for (uint8_t reg = 0; reg < 14; reg++)
    {
        frame.Update(reg, *dataPtr);
        dataPtr += valueSize;
    }

    return (m_frame < m_frames);
}

void DecodeYM::Close(Stream& stream)
{
    if (m_loop > 0)
        stream.loop.frameId(m_loop);

    delete[] m_data;
}
