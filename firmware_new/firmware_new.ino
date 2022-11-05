#include "src/psg-access.h"
#include "src/psg-detect.h"
#include "src/play-pt2.h"
#include "src/demo-music4.h"

uint8_t* m_data = demo_music;
uint8_t  m_regs[14];

void setup()
{
    PSG::Init();
    PSG::Detect();
    PSG::Reset();

    // TODO
}

void loop()
{
    PT2::Init();
    while(true)
    {
        m_regs[PSG::E_Shape] = 0xFF;

        PT2::Play();
        
        for (uint8_t reg = 0; reg < 14; ++reg)
        {
            if (reg == PSG::E_Shape && m_regs[reg] == 0xFF) continue;
            PSG::Send(reg, m_regs[reg]);
        }
        _delay_ms(20);
    }
}
