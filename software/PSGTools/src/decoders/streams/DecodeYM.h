#pragma once

#include "decoders/Decoder.h"

class DecodeYM : public Decoder
{
    #pragma pack(push, 1)
    struct HeaderLZH
    {
        uint8_t  hdrSize;
        uint8_t  chkSum;
        uint8_t  method[5];
        uint32_t compSize;
        uint32_t unCompSize;
        uint32_t dosDateTime;
        uint16_t attr;
        uint8_t  fileNameLen;
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct HeaderYM5
    {
        uint8_t id[4];
        uint8_t leo[8];
        uint8_t numOfFrames[4];
        uint8_t songAttr[4];
        uint8_t numOfDig[2];
        uint8_t chipFreq[4];
        uint8_t playFreq[2];
        uint8_t loopFrame[4];
        uint8_t addSize[2];
    };
    #pragma pack(pop)

public:
    bool Open(Stream& stream) override;
    bool Decode(Frame& frame) override;
    void Close(Stream& stream) override;

private:
    uint8_t* m_data;
    size_t   m_offset;
    bool     m_interleaved;
    size_t   m_frames;
    FrameId  m_frame;
    FrameId  m_loop;
};
