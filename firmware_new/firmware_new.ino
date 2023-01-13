#include "src/psg-access.h"
#include "src/uart.h"

#if 1
volatile PSG psg;
volatile uint8_t reg;

void setup()
{
    PSG::SetClock(PSG::F1_75MHZ);
    psg.Init();
    //psg.SetStereo(PSG::Stereo::ACB);
    
    reg = 0xFF;
    UART_Open(57000);
    UART_EnableRXInt(true);
    sei();
}

void loop()
{
    //
}

ISR(USART_RX_vect)
{
    uint8_t data = UDR0;
    if (bit_is_clear(UCSR0A, FE0))
    {
        if (reg < 0x10)
        {
            // received data for register
            psg.SetRegister(reg, data);
            reg = 0xFF;
        }
        else if (data < 0x10)
        {
            // received register number
            reg = data;
        }        
        else if (data == 0xFF)
        {
            // expected register number, but
            // received end-of-frame marker
            psg.Update();
        }
    }
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
