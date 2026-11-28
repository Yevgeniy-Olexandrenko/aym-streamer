#pragma once

#include <stdint.h>

// register bit mask
#define set_bits(reg, bits) reg |=  (bits)
#define res_bits(reg, bits) reg &= ~(bits)

// register bit
#define set_bit(reg, bit) reg |=  (1 << (bit))
#define res_bit(reg, bit) reg &= ~(1 << (bit))
#define isb_set(reg, bit) bool((reg) & (1 << (bit)))

// interrupts
#define disable_interrupts() uint8_t sreg = SREG; cli();
#define restore_interrupts() SREG = sreg;

namespace PSG
{
	namespace Driver
	{
		void PowerOn();
		void SetClock(uint32_t clock);
		void GetClock(uint32_t &clock);
		void Select(uint8_t chip);

		void Reset();
		void Address(uint8_t addr);
		void Write(uint8_t data);
		void Read(uint8_t &data);
	}
}
