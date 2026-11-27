#pragma once

#include <stdint.h>

namespace UART
{
	// USART0: the RX interrupt puts the bytes into a ring buffer, the main
	// loop takes them with Read while it works with the chips. Write waits
	// for the transmitter, it is used only for short replies.
	namespace Driver
	{
		// start the receiver and the transmitter at 57600 baud 8N1
		void Open();

		// take the next received byte, false when the buffer is empty
		bool Read(uint8_t &data);

		// count of the lost bytes (frame error, overrun, buffer full),
		// it wraps after 255: compare it with the last value
		uint8_t GetErrors();

		// send one byte, wait while the transmit buffer is full
		void Write(uint8_t data);
	}
}
