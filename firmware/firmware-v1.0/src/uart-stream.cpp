// -----------------------------------------------------------------------------
// UART to PSG Streaming
// -----------------------------------------------------------------------------

#include "../firmware.h"
#include "uart-stream.h"
#include "uart.h"

volatile uint8_t buffer[256];
volatile uint8_t wr_ptr = 0;
volatile uint8_t rd_ptr = 0;
volatile uint8_t reg = 0xFF;

ISR(USART_RX_vect)
{
    uint8_t data = UDR0;
    if (bit_is_clear(UCSR0A, FE0))
    {
        if (reg <= 15)
        {
            buffer[wr_ptr++] = reg;
            buffer[wr_ptr++] = data;
            reg = 0xFF;
        }
        else if (data <= 15)
        {
            reg = data;
        }
    }
}

void UARTStream_Open()
{
    UART_Open(BAUD_RATE_STREAM);
    UART_EnableRXInt(true);
    sei();
}

void UARTStream_Update()
{
    while(rd_ptr != wr_ptr)
    {
        uint8_t reg  = buffer[rd_ptr++];
        uint8_t data = buffer[rd_ptr++];
        PSG_Send(psg_reg(reg), psg_data(data));
    }
}

void UARTStream_Close()
{
    // TODO
}
