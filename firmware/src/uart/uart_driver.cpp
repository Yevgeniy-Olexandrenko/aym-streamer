#include "uart_driver.h"
#include <avr/io.h>
#include <avr/interrupt.h>

namespace UART
{
	namespace Driver
	{
		// double speed mode (U2X0): baud = F_CPU / 8 / (UBRR0 + 1),
		// at 16 MHz UBRR0 = 34 gives 57143 baud (-0.8%), the normal
		// mode gives 58824 baud (+2.1%)
		static constexpr uint32_t BAUD = 57600;
		static constexpr uint16_t UBRR = uint16_t((uint32_t(F_CPU) + 4 * BAUD) / (8 * BAUD) - 1);

		// the size is a power of two: the indexes wrap with the mask,
		// one cell stays empty to tell a full buffer from an empty one
		static constexpr uint8_t BUF_SIZE = 128;
		static constexpr uint8_t BUF_MASK = BUF_SIZE - 1;

		// one byte values are read and written by one instruction,
		// so the ISR and the main loop share them without cli
		static volatile uint8_t s_buffer[BUF_SIZE];
		static volatile uint8_t s_head;   // written by the ISR only
		static volatile uint8_t s_tail;   // written by Read only
		static volatile uint8_t s_errors; // written by the ISR only

		void Open()
		{
			s_head = s_tail = 0;
			UBRR0  = UBRR;
			UCSR0A = (1 << U2X0);                  // double speed mode
			UCSR0C = (1 << UCSZ01 | 1 << UCSZ00);  // async, 8 data bits, no parity, 1 stop bit
			UCSR0B = (1 << RXCIE0 | 1 << RXEN0 | 1 << TXEN0); // receiver, its interrupt and transmitter on
		}

		bool Read(uint8_t &data)
		{
			uint8_t tail = s_tail;
			if (tail == s_head) return false;
			data = s_buffer[tail];
			s_tail = (tail + 1) & BUF_MASK;
			return true;
		}

		uint8_t GetErrors()
		{
			return s_errors;
		}

		void Write(uint8_t data)
		{
			// UDRE0 is set when the transmit buffer can take a byte
			while (!(UCSR0A & (1 << UDRE0)));
			UDR0 = data;
		}
	}
}

using namespace UART::Driver;

// The USART keeps 2 received bytes plus the one in the shift register,
// so this ISR can wait for about 2 byte times (350 us at 57600 baud).
ISR(USART_RX_vect)
{
	// the error flags are valid only until UDR0 is read
	uint8_t status = UCSR0A;
	uint8_t data = UDR0;

	uint8_t head = s_head;
	uint8_t next = (head + 1) & BUF_MASK;
	if ((status & (1 << FE0 | 1 << DOR0)) || next == s_tail)
	{
		// frame error, a byte lost before this one,
		// or the buffer is full: drop the byte
		s_errors++;
	}
	else
	{
		s_buffer[head] = data;
		s_head = next;
	}
}
