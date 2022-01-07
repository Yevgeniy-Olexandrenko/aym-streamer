// -----------------------------------------------------------------------------
// Serial to PSG Streaming
// -----------------------------------------------------------------------------

volatile byte buffer[256];
volatile byte wr_ptr = 0;
volatile byte rd_ptr = 0;
volatile byte reg = 0xFF;

ISR(USART_RX_vect)
{
    byte data = UDR0;
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

void SerialToPSG_Init()
{
    cli();
    UBRR0H = BAUD_RATE_CALC(BAUD_RATE_STREAM) >> 8;
    UBRR0L = BAUD_RATE_CALC(BAUD_RATE_STREAM);
    UCSR0A = 0x00;
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    sei();
}

void SerialToPSG_Update()
{
    while(rd_ptr != wr_ptr)
    {
        byte reg  = buffer[rd_ptr++];
        byte data = buffer[rd_ptr++];
        PSG_ProcessAndSend(reg, data);
    }
}
