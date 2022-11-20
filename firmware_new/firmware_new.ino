#include "src/psg-access.h"

PSG psg;

#if 0
void setup()
{
    psg.Init();
}

void loop()
{
    //
}
#else
#include "src/research/play-pt2.h"
#include "src/research/demo-music4.h"

uint8_t* m_data = demo_music;
uint8_t  m_regs[14];

void setup()
{
    psg.Init();

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
            psg.SetRegister(reg, m_regs[reg]);
        }
        _delay_ms(20);
    }
}
#endif
