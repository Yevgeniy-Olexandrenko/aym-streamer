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

		// 256 bytes: the one-byte indexes wrap by themselves, one cell
		// stays empty to tell a full buffer from an empty one
		static volatile uint8_t s_buffer[256];

		// XON/XOFF levels, measured with Tera Term (64 byte chunks):
		// up to 2 chunks still come after XOFF, 256 - 2 * 64 - 8 = 120
		static constexpr uint8_t XON  = 0x11;
		static constexpr uint8_t XOFF = 0x13;
		static constexpr uint8_t LEVEL_XOFF = 120;
		static constexpr uint8_t LEVEL_XON  = 60;

		// one byte values are read and written by one instruction,
		// so the ISR and the main loop share them without cli
		static volatile uint8_t s_head;   // written by the ISR only
		static volatile uint8_t s_tail;   // written by Read only
		static volatile uint8_t s_errors; // written by the ISR only
		static volatile bool    s_flow;   // XON/XOFF is on
		static volatile bool    s_paused; // XOFF is sent, XON is not

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
			s_tail = ++tail;

			// the ISR sends no XOFF while s_paused is set,
			// so the XON and the XOFF writes do not mix
			if (s_paused && uint8_t(s_head - tail) <= LEVEL_XON)
			{
				Write(XON);
				s_paused = false;
			}
			return true;
		}

		uint8_t GetErrors()
		{
			return s_errors;
		}

		void Write(uint8_t data)
		{
			// UDRE0 is set when the transmit buffer can take a byte. The
			// RX ISR can send XOFF between the check and the write: the
			// check is done again with interrupts off, the wait is not.
			for (;;)
			{
				while (!(UCSR0A & (1 << UDRE0)));
				uint8_t sreg = SREG; cli();
				bool ready = (UCSR0A & (1 << UDRE0));
				if (ready) UDR0 = data;
				SREG = sreg;
				if (ready) return;
			}
		}

		void SetFlowControl(bool on)
		{
			// the ISR checks s_flow first: clear it before the XON
			s_flow = on;
			if (s_paused)
			{
				Write(XON);
				s_paused = false;
			}
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
	uint8_t level = uint8_t(head - s_tail);
	if ((status & (1 << FE0 | 1 << DOR0)) || level == 255)
	{
		// frame error, a byte lost before this one,
		// or the buffer is full: drop the byte
		s_errors++;
	}
	else
	{
		s_buffer[head] = data;
		s_head = ++head;

		// XOFF at once: the bytes in flight still come after it
		if (s_flow && !s_paused && ++level >= LEVEL_XOFF)
		{
			s_paused = true;
			Write(XOFF);
		}
	}
}
