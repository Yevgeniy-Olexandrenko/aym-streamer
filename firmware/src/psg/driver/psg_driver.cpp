#include "psg_driver.h"
#include "psg_wiring.h"
#include <util/delay.h>
#include <avr/interrupt.h>

#define wait_for_delay(ns) _delay_us(0.001f * (ns))
#define BUS_MASK (1 << BUS_BDIR | 1 << BUS_BC1)

namespace PSG
{
	namespace Driver
	{
		static uint32_t s_clock;

		// timing delays (nano seconds), the strictest values of
		// AY-3-8910/8912 (GI, Microchip), YM2149, YM3439 and AY8930
		static constexpr uint16_t tAS = 400;  // Address Setup Time (GI)
		static constexpr uint16_t tAH = 100;  // Address Hold Time (GI)
		static constexpr uint16_t tDW = 500;  // Write Data Pulse Width (GI, Microchip)
		static constexpr uint16_t tDH = 100;  // Write Data Hold Time (GI)
		static constexpr uint16_t tDA = 500;  // Read Data Access Time, max (GI)
		static constexpr uint16_t tTS = 200;  // Tristate Delay Time, max (GI)
		static constexpr uint16_t tRW = 3000; // Reset Pulse Width (AY8930: 2 clock periods, 2 us at 1 MHz)
		static constexpr uint16_t tRB = 100;  // Reset to Bus Control Delay Time (GI, YM2149)

		// AVR input synchronizer delays the pin value
		// in PINx register up to 1.5 CPU clock cycles
		static constexpr uint16_t tSY = uint16_t(2 * 1000000000UL / F_CPU);

		inline void set_data_bus(uint8_t data)
		{
			// set ports to output
			set_bits(LSB_DDR, LSB_MASK);
			set_bits(MSB_DDR, MSB_MASK);

			// set data bits to output ports
			LSB_PORT = (LSB_PORT & ~LSB_MASK) | (data & LSB_MASK);
			MSB_PORT = (MSB_PORT & ~MSB_MASK) | (data & MSB_MASK);
		}

		inline void get_data_bus(uint8_t& data)
		{
			// get data bits from input ports
			data = (LSB_PIN & LSB_MASK) | (MSB_PIN & MSB_MASK);
		}

		inline void release_data_bus()
		{
			// setup ports to input
			res_bits(LSB_DDR, LSB_MASK);
			res_bits(MSB_DDR, MSB_MASK);

			// enable pull-up resistors
			set_bits(LSB_PORT, LSB_MASK);
			set_bits(MSB_PORT, MSB_MASK);
		}

		inline void set_ctrl_bus_addr()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR | 1 << BUS_BC1; }
		inline void set_ctrl_bus_write() { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BDIR; }
		inline void set_ctrl_bus_read()  { BUS_PORT = (BUS_PORT & ~BUS_MASK) | 1 << BUS_BC1;  }
		inline void set_ctrl_bus_inact() { BUS_PORT = (BUS_PORT & ~BUS_MASK); }

		void PowerOn()
		{
			// setup hardware pins
			set_bit(BUS_DDR, BUS_BDIR); // output
			set_bit(BUS_DDR, BUS_BC1 ); // output
			set_bit(SIG_DDR, SIG_RES ); // output
			set_bit(SIG_DDR, SIG_CLK ); // output
			set_ctrl_bus_inact();
			release_data_bus();
		}

		void SetClock(uint32_t clock)
		{
			// choosing a divider for the closest clock frequency
			uint8_t  clockDiv = uint8_t(F_CPU / clock);
			uint32_t minClock = (uint32_t(F_CPU) / (clockDiv + 1));
			uint32_t maxClock = (uint32_t(F_CPU) / (clockDiv + 0));
			if (clock - minClock < maxClock - clock) clockDiv++;
			s_clock = (uint32_t(F_CPU) / clockDiv);

			// configure Timer2 as a clock source for PSG
			uint8_t compare = uint8_t(clockDiv - 1);
			TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
			TCCR2B = (1 << WGM22 ) | (1 << CS20 );
			OCR2A  = compare;
			OCR2B  = compare / 2; // duty 50-56% (40-60% allowed)
		}

		void GetClock(uint32_t &clock)
		{
			// real PSG clock based on Timer2 divider
			clock = s_clock;
		}

		void Reset()
		{
			// 'Reset PSG' sequence
			res_bit(SIG_PORT, SIG_RES);
			wait_for_delay(tRW);
			set_bit(SIG_PORT, SIG_RES);
			wait_for_delay(tRB);
		}

		void Address(uint8_t addr)
		{
			// 'Latch Address' sequence, the address goes on the bus
			// before BDIR/BC1: else the bus pull-ups give 0xFF for a
			// short time, and TurboSound logic selects a wrong chip
			set_data_bus(addr);
			set_ctrl_bus_addr();
			wait_for_delay(tAS);
			set_ctrl_bus_inact();
			wait_for_delay(tAH);
			release_data_bus();
		}

		void Write(uint8_t data)
		{
			// 'Write to PSG' sequence
			set_data_bus(data);

			// the write pulse is limited to 10 us (GI, Microchip,
			// YM2149, YM3439), so interrupts must not stretch it
			uint8_t sreg = SREG; cli();
			set_ctrl_bus_write();
			wait_for_delay(tDW);
			set_ctrl_bus_inact();
			SREG = sreg;

			wait_for_delay(tDH);
			release_data_bus();
		}

		void Read(uint8_t &data)
		{
			// 'Read from PSG' sequence
			set_ctrl_bus_read();
			wait_for_delay(tDA + tSY);
			get_data_bus(data);
			set_ctrl_bus_inact();
			wait_for_delay(tTS);
		}
	}
}
