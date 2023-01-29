#include "uart-stream.h"
#include "psg-access.h"

#include <avr/interrupt.h>
#define  MU_TX_BUF 0
#define  MU_RX_BUF 0
#include <MicroUART.h>
#include <GyverTimers.h>

namespace UART
{
    namespace Stream
    {
        MicroUART  m_uart;
        SoundChip* m_soundChip;
        volatile uint8_t m_reg;
        volatile uint8_t m_wrp = 0;
        volatile uint8_t m_rdp = 0;
        volatile uint8_t m_buf[256];
    }
}

// -----------------------------------------------------------------------------

void UART::Stream::Start(SoundChip& soundChip)
{
    // prepare internal state
    m_soundChip = &soundChip;
    m_reg = 0xFF;

    // start UART communication
    // start sound chip update timer
    m_uart.begin(57600);
    Timer1.setPeriod(1000);
    Timer1.enableISR(CHANNEL_A);
}

void UART::Stream::Stop()
{
    // stop sound chip update timer
    // stop UART communication
    Timer1.stop();
    m_uart.end();
}

// -----------------------------------------------------------------------------

void MU_serialEvent()
{
    // store data in circular buffer and
    // delay sound chip update procedure
    using namespace UART::Stream;
    m_buf[m_wrp++] = m_uart.read();
    Timer1.restart();
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
