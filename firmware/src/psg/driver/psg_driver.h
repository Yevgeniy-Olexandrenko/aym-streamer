#pragma once

#include <stdint.h>

#define set_bits(reg, bits) reg |=  (bits)
#define res_bits(reg, bits) reg &= ~(bits)

#define set_bit(reg, bit) reg |=  (1 << (bit))
#define res_bit(reg, bit) reg &= ~(1 << (bit))
#define isb_set(reg, bit) bool((reg) & (1 << (bit)))

namespace PSG
{
	namespace Driver
	{
		void PowerOn();
		void SetClock(uint32_t clock);
		void GetClock(uint32_t &clock);

		void Reset();
		void Address(uint8_t addr);
		void Write(uint8_t data);
		void Read(uint8_t &data);

		// TurboSound: a latch of 0xF8-0xFF selects the chip by DA0,
		// 0xFF chip 0, 0xFE chip 1. Reset latches the selected chip again.
		void Select(uint8_t chip);
	}
}
