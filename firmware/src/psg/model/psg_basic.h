#pragma once

#include "../driver/psg_driver.h"

namespace PSG
{
	enum class ChipId : uint32_t
	{
		NotFound   = 0x67E019C7, // [x] chip not found
		Compatible = 0xBD95F7C7, // [x] Variants: WF19054, JFC95101, KC89C72 etc
		AY8910     = 0xE6D5F1AE, // [x] GI AY-3-8910A, Microchip AY38910A/P
		AY8930     = 0x1728F586, // [x] Microchip AY8930
		YM2149F    = 0x4F5494D7, // [X] Yamaha YM2149F
	};

	enum class Clock : uint32_t
	{
		F_1000000 = 1000000, // Amstrad CPC, Oric
		F_1750000 = 1750000, // Pentagon and other ZX Spectrum clones
		F_1773400 = 1773400, // ZX Spectrum 128/+2/+3 (default)
		F_1789773 = 1789773, // MSX (3.579545 MHz / 2), also NES CPU clock
		F_2000000 = 2000000, // Atari ST

		F_MIN = F_1000000,
		F_MAX = F_2000000,
		F_DEF = F_1773400
	};

	using raddr_t = uint8_t;
	using rdata_t = uint8_t;

	class Basic
	{
	public:
		// init and reset operations with PSG,
		// reset also clears all registers (mute)
		void Init();
		void Reset();

		// set/get operations with PSG configuration,
		// get returns the real clock set by the divider
		void SetClock(Clock clock);
		Clock GetClock() const;
		ChipId GetChipId() const;

		// set/get operations with PSG registers
		// directly on the chip (address 0x00-0x0F)
		void SetRegister(raddr_t addr, rdata_t data);
		rdata_t GetRegister(raddr_t addr) const;

	private:
		// tests to detect the type of PSG
		void compute_chipid();
		void update_chipid(rdata_t data);
		void test_wr_rd_regs(raddr_t offset);
		void test_wr_rd_latch(raddr_t offset);
		void test_wr_rd_exp_mode(rdata_t mode_bank);

	private:
		ChipId m_chipid{ ChipId::NotFound };
	};
}
