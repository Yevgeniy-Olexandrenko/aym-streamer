#pragma once

#include <stdint.h>

namespace UART
{
	// USART0 receiver: the RX interrupt puts the bytes into a ring buffer,
	// the main loop takes them with Read while it works with the chips.
	namespace Driver
	{
		// start the receiver at 57600 baud 8N1, the transmitter is off
		void Open();

		// take the next received byte, false when the buffer is empty
		bool Read(uint8_t &data);

		// count of the lost bytes (frame error, overrun, buffer full),
		// it wraps after 255: compare it with the last value
		uint8_t GetErrors();
	};
}
