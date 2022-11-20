#include "uart.h"
#include <stdlib.h>

void UART_Open(uint32_t baud)
{
    UBRR0H = ((F_CPU / 16 / baud) - 1) >> 8;
    UBRR0L = ((F_CPU / 16 / baud) - 1);
    UCSR0A = 0x00;
    UART_EnableRX(true);
    UART_EnableTX(true);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

void UART_EnableRX(bool yes)
{
    if (yes) UCSR0B |= _BV(RXEN0); else UCSR0B &= ~_BV(RXEN0);
}

void UART_EnableRXInt(bool yes)
{
    if (yes) UCSR0B |= _BV(RXCIE0); else UCSR0B &= ~_BV(RXCIE0);
}

void UART_EnableTX(bool yes)
{
    if (yes) UCSR0B |= _BV(TXEN0); else UCSR0B &= ~_BV(TXEN0);
}

void UART_EnableTXInt(bool yes)
{
    if (yes) UCSR0B |= _BV(TXCIE0); else UCSR0B &= ~_BV(TXCIE0);
}

void UART_Close()
{
    UART_EnableRXInt(false);
    UART_EnableTXInt(false);
    UART_EnableRX(false);
    UART_EnableTX(false);
}

void UART_PutByte(uint8_t data)
{
    // Stay here until data buffer is empty
    while (!(UCSR0A & _BV(UDRE0)));
    UDR0 = (unsigned char) data;
}

uint8_t UART_GetByte()
{
    // Check to see if something was received
    while (!(UCSR0A & _BV(RXC0)));
    return (uint8_t) UDR0;
}

void UART_PutNibbleHex(uint8_t nib)
{
    nib &= 0x0F;
    UART_PutByte((nib >= 0x0A ? 'A' - 0x0A : '0') + nib);
}

void UART_PutByteHex(uint8_t data)
{
    UART_PutNibbleHex(data >> 4);
    UART_PutNibbleHex(data);
}

void UART_PutWordHex(uint16_t data)
{
    UART_PutByteHex(data >> 8);
    UART_PutByteHex(data);
}

void UART_PutDWordHex(uint32_t data)
{
    UART_PutWordHex(data >> 16);
    UART_PutWordHex(data);
}

void UART_PutSP()
{
    UART_PutByte(' ');
}

void UART_PutLN()
{
    UART_PutByte('\n');
}

void UART_PutNumber(int32_t data)
{
    char s[12];
    ultoa(data, s, 10);
    UART_PutString(s);
}

void UART_PutString(const char* str)
{
    while (*str != '\0') UART_PutByte(*str++);
}

void UART_PutString(const __FlashStringHelper* str)
{
    const char* p = (const char *)str;
    while (char c = pgm_read_byte(p++)) UART_PutByte(c);
}

void UART_GetString(char* str, uint8_t len)
{
    while (len && (*str = UART_GetByte()) != '\n') { str++; len--; }
}
