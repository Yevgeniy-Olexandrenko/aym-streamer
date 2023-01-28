#include "src/psg-access.h"
#include "src/uart.h"

volatile SoundChip psg;
volatile uint8_t reg;

void setup()
{
    psg.Init();
    psg.SetClock(SoundChip::Clock::F1_75MHZ);
    psg.SetStereo(SoundChip::Stereo::CBA);
    // PSG::Init();
    
    reg = 0xFF;
    UART_Open(57600);
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
        // if (reg < 0x10)
        // {
        //     // received data for register
        //     PSG::Address(reg);
        //     PSG::Write(data);
        //     reg = 0xFF;
        // }
        // else if (data < 0x10)
        // {
        //     // received register number
        //     reg = data;
        // }
    }
}
