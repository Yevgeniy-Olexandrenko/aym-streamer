#pragma once

#include <memory>
#include "decoders/Decoder.h"

#define DBG_DECODE_VGM 0

class ChipSim;

class DecodeVGM : public Decoder
{
    #pragma pack(push, 1)
    struct Header
    {
        uint32_t ident; // "Vgm "
        uint32_t eofOffset;
        uint32_t version;
        uint32_t sn76489Clock;
        uint32_t ym2413Clock;
        uint32_t gd3Offset;
        uint32_t totalSamples;
        uint32_t loopOffset;
        uint32_t loopSamples;
        uint32_t rate;
        uint16_t sn76489Feedback;
        uint16_t sn76489ShiftReg;
        uint32_t ym2612Clock;
        uint32_t ym2151Clock;
        uint32_t vgmDataOffset;
        uint32_t segaPcmClock;
        uint32_t spcmInterface;
        uint32_t rf5c68Clock;
        uint32_t ym2203Clock;
        uint32_t ym2608Clock;
        uint32_t ym2610bClock;
        uint32_t ym3812Clock;
        uint32_t ym3526Clock;
        uint32_t y8950Clock;
        uint32_t ymf262Clock;
        uint32_t ymf278bClock;
        uint32_t ymf271Clock;
        uint32_t ymz280bClock;
        uint32_t rf5c164Clock;
        uint32_t pwmClock;
        uint32_t ay8910Clock;
        uint8_t  ay8910Type;
        uint8_t  ay8910Flags;
        uint8_t  ym2203ay8910Flags;
        uint8_t  ym2608ay8910Flags;
        uint8_t  volumeModifier;
        uint8_t  reserved1;
        uint8_t  loopBase;
        uint8_t  loopModifier;
        uint32_t gbDmgClock;
        uint32_t nesApuClock;
        uint32_t multiPcmClock;
        uint32_t upd7759Clock;
        uint32_t okim6258Clock;
        uint8_t  okim6258Flags;
        uint8_t  k054539Flags;
        uint8_t  c140Flags;
        uint8_t  reserved2;
        uint32_t oki6295Clock;
        uint32_t k051649Clock;
        uint32_t k054539Clock;
        uint32_t huc6280Clock;
        uint32_t c140Clock;
        uint32_t k053260Clock;
        uint32_t pokeyClock;
        uint32_t qsoundClock;
        uint32_t scspClock;
        uint32_t extraHeaderOffset;
        uint32_t wswanClock;
        uint32_t vsuClock;
        uint32_t saa1090Clock;
        uint32_t es5503Clock;
        uint32_t es5506Clock;
        uint8_t  es5503Channels;
        uint8_t  es5506Channels;
        uint8_t  c352ClockDivider;
        uint8_t  reserved3;
        uint32_t x1010Clock;
        uint32_t c352Clock;
        uint32_t ga20Clock;
        uint8_t  reserved4[7 * 4];
    };
    #pragma pack(pop)

public:
	bool Open(Stream& stream) override;
	bool Decode(Frame& frame) override;
	void Close(Stream& stream) override;

private:
    int  DecodeBlock();
   

private:
    uint8_t* m_data;
    uint8_t* m_dataPtr;

    int m_loop;
    int m_samplesPerFrame;
    int m_processedSamples;

    std::shared_ptr<ChipSim> m_simulator;
};
