#include "uart-stream.h"
#include "psg/AdvancedAccess.h"
#include <stdlib.h>

#include <avr/interrupt.h>
#define  MU_TX_BUF 0
#define  MU_RX_BUF 0
#include <MicroUART.h>
#include <GyverTimers.h>

// shared class instance
uart_stream UARTStream;

// shared references and objects
static volatile psg::AdvancedAccess* m_psg;
static volatile MicroUART m_uart;

// handling input binary data
static volatile uint8_t m_buf[64];
static volatile uint8_t m_wrp;
static volatile uint8_t m_rdp;
static volatile uint8_t m_reg;

// handling input strings
static volatile uart_stream::Handler m_handler = nullptr;
static volatile uint8_t m_str[64];
static volatile uint8_t m_stp;

// -----------------------------------------------------------------------------

void uart_stream::Start(psg::AdvancedAccess& psg)
{
    // prepare internal state
    m_psg = &psg; m_reg = 0xFF;
    m_wrp = 0x00; m_rdp = 0x00;
    m_stp = 0x00;

    // start UART for communication
    // and timer for PSG update
    m_uart.begin(57600);
    Timer1.setPeriod(1000);
    Timer1.enableISR(CHANNEL_A);
    sei();
}

void uart_stream::SetInputHandler(Handler handler)
{
    m_handler = handler;
}

void uart_stream::Stop()
{
    // stop PSG update timer and
    // communication via UART
    Timer1.stop();
    m_uart.end();
}

void uart_stream::Print(char chr, int num)
{
    while (num-- > 0) m_uart.write(chr);
}

void uart_stream::Print(const char* str)
{
    while (*str != '\0') m_uart.write(*str++);
}

void uart_stream::Print(const __FlashStringHelper* str)
{
    const char* p = (const char *)str;
    while (char c = pgm_read_byte(p++)) m_uart.write(c);
}

void uart_stream::Print(uint8_t val, bool hex)
{
    if (hex)
    {
        PrintNibble(val >> 4);
        PrintNibble(val);
    }
    else
    {
        PrintNumber(val);
    }
}

void uart_stream::Print(uint16_t val, bool hex)
{
    if (hex)
    {
        Print(uint8_t(val >> 8), true);
        Print(uint8_t(val), true);
    }
    else
    {
        PrintNumber(val);
    }
}

void uart_stream::Print(uint32_t val, bool hex)
{
    if (hex)
    {
        Print(uint16_t(val >> 16), true);
        Print(uint16_t(val), true);
    }
    else
    {
        PrintNumber(val);
    }
}

void uart_stream::Println(char chr, int num)
{
    Print(chr, num);
    Println();
}

void uart_stream::Println(const char* str)
{
    Print(str);
    Println();
}

void uart_stream::Println(const __FlashStringHelper* str)
{
    Print(str);
    Println();
}

void uart_stream::Println(uint8_t val, bool hex)
{
    Print(val, hex);
    Println();
}

void uart_stream::Println(uint16_t val, bool hex)
{
    Print(val, hex);
    Println();
}

void uart_stream::Println(uint32_t val, bool hex)
{
    Print(val, hex);
    Println();
}

void uart_stream::Println()
{
    m_uart.write('\r');
    m_uart.write('\n');
}

void uart_stream::PrintNibble(uint8_t nibble)
{
    nibble &= 0x0F;
    m_uart.write((nibble >= 0x0A ? 'A' - 0x0A : '0') + nibble);
}

void uart_stream::PrintNumber(int32_t number)
{
    char str[12];
    ultoa(number, str, 10);
    Print(str);
}

// -----------------------------------------------------------------------------

void MU_serialEvent()
{
    // store data in circular buffer and
    // delay sound chip update procedure
    m_buf[m_wrp++] = m_uart.read();
    m_wrp &= sizeof(m_buf) - 1;
}

ISR(TIMER1_A) 
{
    // lock update timer and enable interrupts
    // to handle incoming data in the background
    Timer1.pause(); sei();

    // process received data in buffer
    while (m_rdp != m_wrp)
    {
        uint8_t data = m_buf[m_rdp++];
        m_rdp &= sizeof(m_buf) - 1;

        if (m_reg < 0x10)
        {
            // handle data for register
            m_psg->SetRegister(m_reg, data);
            m_reg = 0xFF;
        }
        else
        {
            // waiting for register number
            if (data == 0xFF)
            {
                // handle frame begining
                m_psg->Update();
                m_stp = 0x00;
            }
            else if (data >= 0x20 && data <= 0x7F)
            {
                // handle new char for input string
                if (m_stp < sizeof(m_str) - 2)
                {
                    m_str[m_stp++] = data;
                }
            }
            else if (data < 0x10)
            {
                if (m_stp)
                {
                    // handle input string processing
                    if (m_handler)
                    {
                        m_str[m_stp] = 0;
                        m_handler(m_str);
                    }
                }
                else
                {
                    // handle register number
                    m_reg = data;
                }
                m_stp = 0x00;
            }
        }
    }

    // unlock update timer
    Timer1.resume();
}
