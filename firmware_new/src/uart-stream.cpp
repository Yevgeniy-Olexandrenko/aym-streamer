#include <GyverTimers.h>
#include "uart-stream.h"
#include "psg-access.h"
#include "uart.h"

//#define DEBUG_DIRRECT_WRITE

namespace UART
{
    namespace Stream
    {
        volatile SoundChip* m_soundChip;
        volatile uint8_t m_buf[256];
        volatile uint8_t m_wrp = 0;
        volatile uint8_t m_rdp = 0;
        volatile uint8_t m_reg;
    }
}

// -----------------------------------------------------------------------------

void UART::Stream::Start(SoundChip& soundChip)
{
    // prepare internal state
    m_soundChip = &soundChip;
    m_reg = 0xFF;

    // start UART communication
    UART_Open(57600);
    UART_EnableRXInt(true);

    // start sound chip update timer
    Timer1.setPeriod(1000);
    Timer1.enableISR(CHANNEL_A);
}

void UART::Stream::Stop()
{
    // stop sound chip update timer
    Timer1.stop();

    // stop UART communication
    UART_Close();
}

// -----------------------------------------------------------------------------

ISR(USART_RX_vect)
{
    using namespace UART::Stream;
    if (bit_is_clear(UCSR0A, FE0))
    {
        // store data in circular buffer and
        // delay sound chip update procedure
        m_buf[m_wrp++] = UDR0;
        Timer1.restart();
    }
}

ISR(TIMER1_A) 
{
    // enable interrupts for data receiving
    // from UART stream in background
    Timer1.pause();
    sei();

    // process received data in buffer
    using namespace UART::Stream;
    while (m_rdp != m_wrp)
    {
        uint8_t data = m_buf[m_rdp++];
        if (m_reg < 0x10)
        {
            // handle data for register
            m_soundChip->SetRegister(m_reg, data);
            m_reg = 0xFF;
        }
        else if (data < 0x10)
        {
            // handle register number
            m_reg = data;
        }
    }
    m_soundChip->Update();
}
