#include "play-pt2.h"
#include "../psg-access.h"

#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

// externs
extern uint8_t* m_data;
extern uint8_t  m_regs[14];

namespace PT2
{
    // constants
    static const uint16_t NoteTable[] PROGMEM =
    {
        0x0ef8, 0x0e10, 0x0d60, 0x0c80, 0x0bd8, 0x0b28, 0x0a88, 0x09f0,
        0x0960, 0x08e0, 0x0858, 0x07e0, 0x077c, 0x0708, 0x06b0, 0x0640,
        0x05ec, 0x0594, 0x0544, 0x04f8, 0x04b0, 0x0470, 0x042c, 0x03fd,
        0x03be, 0x0384, 0x0358, 0x0320, 0x02f6, 0x02ca, 0x02a2, 0x027c,
        0x0258, 0x0238, 0x0216, 0x01f8, 0x01df, 0x01c2, 0x01ac, 0x0190,
        0x017b, 0x0165, 0x0151, 0x013e, 0x012c, 0x011c, 0x010a, 0x00fc,
        0x00ef, 0x00e1, 0x00d6, 0x00c8, 0x00bd, 0x00b2, 0x00a8, 0x009f,
        0x0096, 0x008e, 0x0085, 0x007e, 0x0077, 0x0070, 0x006b, 0x0064,
        0x005e, 0x0059, 0x0054, 0x004f, 0x004b, 0x0047, 0x0042, 0x003f,
        0x003b, 0x0038, 0x0035, 0x0032, 0x002f, 0x002c, 0x002a, 0x0027,
        0x0025, 0x0023, 0x0021, 0x001f, 0x001d, 0x001c, 0x001a, 0x0019,
        0x0017, 0x0016, 0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000f,
    };

    // variables
    static uint8_t m_delay;
    static uint8_t m_delayCounter;
    static uint8_t m_currentPosition;
    static Channel m_ch[3];

    // forward declarations
    static void PatternInterpreter(Channel& chan);
    static void GetRegisters(Channel& chan, uint8_t& mixer);

    void Init()
    {
        Header* header = (Header*)m_data;

        m_delay = header->delay;
        m_delayCounter = 1;
        m_currentPosition = 0;

        memset(&m_ch[0], 0, sizeof(Channel));
        memset(&m_ch[1], 0, sizeof(Channel));
        memset(&m_ch[2], 0, sizeof(Channel));

        uint16_t patternPointer = header->patternsPointer + header->positionList[0] * 6;
        m_ch[0].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 0]);
        m_ch[1].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 2]);
        m_ch[2].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 4]);

        for (int i = 0; i < 3; ++i)
        {
            m_ch[i].ornamentPointer = header->ornamentsPointers[0];
            m_ch[i].ornamentLength = m_data[m_ch[i].ornamentPointer++];
            m_ch[i].loopOrnamentPosition = m_data[m_ch[i].ornamentPointer++];
            m_ch[i].volume = 15;
        }

        memset(m_regs, 0, sizeof(m_regs));
    }

    void Play()
    {
        Header* header = (Header*)m_data;
        uint8_t mixer  = 0;

        if (--m_delayCounter == 0)
        {
            if (--m_ch[0].noteSkipCounter < 0)
            {
                if (m_data[m_ch[0].addressInPattern] == 0)
                {
                    if (++m_currentPosition == header->numberOfPositions)
                    {
                        m_currentPosition = header->loopPosition;
                    }

                    uint16_t patternPointer = header->patternsPointer + header->positionList[m_currentPosition] * 6;
                    m_ch[0].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 0]);
                    m_ch[1].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 2]);
                    m_ch[2].addressInPattern = *(uint16_t*)(&m_data[patternPointer + 4]);
                }

                PatternInterpreter(m_ch[0]);
            }

            if (--m_ch[1].noteSkipCounter < 0) PatternInterpreter(m_ch[1]);
            if (--m_ch[2].noteSkipCounter < 0) PatternInterpreter(m_ch[2]);

            m_delayCounter = m_delay;
        }

        GetRegisters(m_ch[0], mixer);
        GetRegisters(m_ch[1], mixer);
        GetRegisters(m_ch[2], mixer);

        m_regs[PSG::Mixer   ] = mixer;
        m_regs[PSG::A_Fine  ] = m_ch[0].ton & 0xff;
        m_regs[PSG::A_Coarse] = (m_ch[0].ton >> 8) & 0xf;
        m_regs[PSG::B_Fine  ] = m_ch[1].ton & 0xff;
        m_regs[PSG::B_Coarse] = (m_ch[1].ton >> 8) & 0xf;
        m_regs[PSG::C_Fine  ] = m_ch[2].ton & 0xff;
        m_regs[PSG::C_Coarse] = (m_ch[2].ton >> 8) & 0xf;
        m_regs[PSG::A_Volume] = m_ch[0].amplitude;
        m_regs[PSG::B_Volume] = m_ch[1].amplitude;
        m_regs[PSG::C_Volume] = m_ch[2].amplitude;
    }

    static void PatternInterpreter(Channel& chan)
    {
        Header* header = (Header*)m_data;
        bool quit = false;
        bool gliss = false;

        do
        {
            uint8_t val = m_data[chan.addressInPattern];
            if (val >= 0xe1)
            {
                chan.samplePointer = header->samplesPointers[val - 0xe0];
                chan.sampleLength = m_data[chan.samplePointer++];
                chan.loopSamplePosition = m_data[chan.samplePointer++];
            }
            else if (val == 0xe0)
            {
                chan.positionInSample = 0;
                chan.positionInOrnament = 0;
                chan.currentTonSliding = 0;
                chan.glissType = 0;
                chan.enabled = false;
                quit = true;
            }
            else if (val >= 0x80 && val <= 0xdf)
            {
                chan.positionInSample = 0;
                chan.positionInOrnament = 0;
                chan.currentTonSliding = 0;
                if (gliss)
                {
                    chan.slideToNote = val - 0x80;
                    if (chan.glissType == 1)
                        chan.note = chan.slideToNote;
                }
                else
                {
                    chan.note = val - 0x80;
                    chan.glissType = 0;
                }
                chan.enabled = true;
                quit = true;
            }
            else if (val == 0x7f)
            {
                chan.envelopeEnabled = false;
            }
            else if (val >= 0x71 && val <= 0x7e)
            {
                chan.envelopeEnabled = true;
                m_regs[PSG::E_Shape] = val - 0x70;
                m_regs[PSG::E_Fine] = m_data[++chan.addressInPattern];
                m_regs[PSG::E_Coarse] = m_data[++chan.addressInPattern];
            }
            else if (val == 0x70)
            {
                quit = true;
            }
            else if (val >= 0x60 && val <= 0x6f)
            {
                chan.ornamentPointer = header->ornamentsPointers[val - 0x60];
                chan.ornamentLength = m_data[chan.ornamentPointer++];
                chan.loopOrnamentPosition = m_data[chan.ornamentPointer++];
                chan.positionInOrnament = 0;
            }
            else if (val >= 0x20 && val <= 0x5f)
            {
                chan.numberOfNotesToSkip = val - 0x20;
            }
            else if (val >= 0x10 && val <= 0x1f)
            {
                chan.volume = val - 0x10;
            }
            else if (val == 0xf)
            {
                m_delay = m_data[++chan.addressInPattern];
            }
            else if (val == 0xe)
            {
                chan.glissade = m_data[++chan.addressInPattern];
                chan.glissType = 1;
                gliss = true;
            }
            else if (val == 0xd)
            {
                chan.glissade = abs((int8_t)(m_data[++chan.addressInPattern]));

                // Do not use precalculated Ton_Delta to
                // avoide error with first note of pattern
                chan.addressInPattern += 2; 
                
                chan.glissType = 2;
                gliss = true;
            }
            else if (val == 0xc)
            {
                chan.glissType = 0;
            }
            else
            {
                chan.additionToNoise = m_data[++chan.addressInPattern];
            }
            chan.addressInPattern++;
        } while (!quit);

        if (gliss && (chan.glissType == 2))
        {
            uint16_t v0 = pgm_read_word(&NoteTable[chan.slideToNote]);
            uint16_t v1 = pgm_read_word(&NoteTable[chan.note]);
            chan.tonDelta = abs(v0 - v1);
            if (chan.slideToNote > chan.note)
                chan.glissade = -chan.glissade;
        }
        chan.noteSkipCounter = chan.numberOfNotesToSkip;
    }

    static void GetRegisters(Channel& chan, uint8_t& mixer)
    {
        Header* header = (Header*)m_data;

        if (chan.enabled)
        {
            uint16_t samplePointer = chan.samplePointer + chan.positionInSample * 3;
            uint8_t b0 = m_data[samplePointer + 0];
            uint8_t b1 = m_data[samplePointer + 1];
            chan.ton = m_data[samplePointer + 2] + (uint16_t)((b1 & 15) << 8);
            if ((b0 & 4) == 0)
                chan.ton = -chan.ton;

            uint8_t note = chan.note + m_data[chan.ornamentPointer + chan.positionInOrnament];
            if (note > 95) note = 95;
            uint16_t v0 = pgm_read_word(&NoteTable[note]);
            chan.ton = (chan.ton + chan.currentTonSliding + v0) & 0xfff;

            if (chan.glissType == 2)
            {
                chan.tonDelta = chan.tonDelta - abs(chan.glissade);
                if (chan.tonDelta < 0)
                {
                    chan.note = chan.slideToNote;
                    chan.glissType = 0;
                    chan.currentTonSliding = 0;
                }
            }
            if (chan.glissType != 0)
                chan.currentTonSliding += chan.glissade;

            chan.amplitude = (chan.volume * 17 + (uint8_t)(chan.volume > 7)) * (b1 >> 4) / 256;
            if (chan.envelopeEnabled) chan.amplitude |= 16;

            if ((b0 & 1) != 0)
                mixer |= 64;
            else
                m_regs[PSG::N_Period] = ((b0 >> 3) + chan.additionToNoise) & 0x1f;

            if ((b0 & 2) != 0)
                mixer |= 8;

            if (++chan.positionInSample == chan.sampleLength)
                chan.positionInSample = chan.loopSamplePosition;

            if (++chan.positionInOrnament == chan.ornamentLength)
                chan.positionInOrnament = chan.loopOrnamentPosition;
        }
        else
        {
            chan.amplitude = 0;
        }
        mixer >>= 1;
    }
}