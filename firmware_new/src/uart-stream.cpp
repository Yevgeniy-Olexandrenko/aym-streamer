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
        MicroUART m_uart;
        SinglePSG* m_psg;
        uint8_t m_reg;
        uint8_t m_wrp = 0;
        uint8_t m_rdp = 0;
        uint8_t m_buf[256];
    }
}

// -----------------------------------------------------------------------------

void UART::Stream::Start(SinglePSG& psg)
{
    // prepare internal state
    m_psg = &psg;
    m_reg = 0xFF;

    // start UART communication
    // start sound chip update timer
    m_uart.begin(57600);
    Timer1.setPeriod(1000);
    Timer1.enableISR(CHANNEL_A);
    sei();
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
}

ISR(TIMER1_A) 
{
    // lock update timer and enable interrupts
    // to handle incoming data in the background
    Timer1.pause(); sei();

    // process received data in buffer
    using namespace UART::Stream;
    while (m_rdp != m_wrp)
    {
        uint8_t data = m_buf[m_rdp++];
        if (m_reg < 0x10)
        {
            // handle data for register
            m_psg->SetRegister(m_reg, data);
            m_reg = 0xFF;
        }
        else if (data < 0x10)
        {
            // handle register number
            m_reg = data;
        }
        else if (data == 0xFF)
        {
            // handle frame begining
            m_psg->Update();
        }
    }

    // unlock update timer
    Timer1.resume();
}
