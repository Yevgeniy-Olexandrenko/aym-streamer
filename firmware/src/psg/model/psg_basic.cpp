#include "psg_basic.h"
#include "../driver/psg_driver.h"

namespace PSG
{
	void Basic::Init()
	{
		Driver::PowerOn();
		SetClock(Clock::F_DEF);
		detect_chipid();
		Reset();
	}

	void Basic::Reset()
	{
		Driver::Reset();
	}

	void Basic::Mute()
	{
		// volume registers of channels A, B and C
		for (raddr_t addr = 0x08; addr <= 0x0A; ++addr)
		{
			SetRegister(addr, 0x00);
		}
	}

	void Basic::SetClock(Clock clock)
	{
		// limit the clock frequency range
		if (clock < Clock::F_MIN) clock = Clock::F_MIN;
		if (clock > Clock::F_MAX) clock = Clock::F_MAX;
		Driver::SetClock(uint32_t(clock));
	}

	Clock Basic::GetClock() const
	{
		uint32_t clock;
		Driver::GetClock(clock);
		return Clock(clock);
	}

	ChipId Basic::GetChipId() const
	{
		return m_chipid;
	}

	void Basic::SetRegister(raddr_t addr, rdata_t data)
	{
		// the chip ignores addresses above 0x0F
		if (addr < 0x10)
		{
			Driver::Address(addr);
			Driver::Write(data);
		}
	}

	rdata_t Basic::GetRegister(raddr_t addr) const
	{
		// the chip ignores addresses above 0x0F,
		// the bus pull-ups give 0xFF instead
		rdata_t data = 0xFF;
		if (addr < 0x10)
		{
			Driver::Address(addr);
			Driver::Read(data);
		}
		return data;
	}

	void Basic::detect_chipid()
	{
		Driver::Reset();
		m_chipid = ChipId(7);
		test_wr_rd_regs(0x00);
		test_wr_rd_regs(0x10);
		test_wr_rd_latch(0x00);
		test_wr_rd_latch(0x10);
		test_wr_rd_exp_mode(0xA0);
		test_wr_rd_exp_mode(0xB0);
	}

	void Basic::update_chipid(rdata_t data)
	{
		m_chipid = ChipId(UINT32_C(31) * uint32_t(m_chipid) + uint32_t(data));
	}

	void Basic::test_wr_rd_regs(raddr_t offset)
	{
		rdata_t data;
		for (raddr_t addr = offset; addr < (offset + 14); ++addr)
		{
			data = 0xFF;
			if (addr == 0x07) data = 0x3F;
			if (addr == 0x0D) data = 0x0F;
			Driver::Address(addr);
			Driver::Write(data);
		}
		for (raddr_t addr = offset; addr < (offset + 14); ++addr)
		{
			Driver::Address(addr);
			Driver::Read(data);
			update_chipid(data);
		}
	}

	void Basic::test_wr_rd_latch(raddr_t offset)
	{
		rdata_t data;
		for (raddr_t addr = offset; addr < (offset + 14); ++addr)
		{
			data = 0xFF;
			if (addr == 0x07) data = 0x3F;
			if (addr == 0x0D) data = 0x0F;
			Driver::Address(addr);
			Driver::Write(data);
			Driver::Read(data);
			update_chipid(data);
		}
	}

	void Basic::test_wr_rd_exp_mode(rdata_t mode_bank)
	{
		Driver::Address(0x0D);
		Driver::Write(mode_bank | 0x0F);
		for (raddr_t addr = 0; addr < 14; ++addr)
		{
			if (addr == 0x0D) continue;
			Driver::Address(addr);
			Driver::Write(0xFF);
		}
		rdata_t data;
		for (raddr_t addr = 0; addr < 14; ++addr)
		{
			Driver::Address(addr);
			Driver::Read(data);
			update_chipid(data);
		}
	}
}
