#pragma once

#include "psg_advanced.h"
#include "../driver/psg_driver.h"

namespace PSG
{
	enum class Board : uint8_t
	{
		SingleChip, // one PSG
		TurboSound  // two PSG, the chip is selected
	};

	// Two PSG of Basic or of Advanced model. The chip is selected on the bus
	// by Driver::Select. Both chips share the clock and the RESET# signal.
	// On a single chip board the calls for chip 1 are ignored, a chip that
	// is not found still gets the data.
	template<class PSGModel>
	class TurboSound
	{
	public:
		// init the chips, detect the board and the chips
		void Init()
		{
			// chip 0 is selected from the start, Init latches it
			m_psg[0].Init();

			// with TurboSound each chip keeps its own value, a single
			// chip keeps the last value written, no chip gives 0xFF
			rdata_t data0, data1;
			Driver::Select(0); Driver::Address(0x00); Driver::Write(0x55);
			Driver::Select(1); Driver::Address(0x00); Driver::Write(0xAA);
			Driver::Select(0); Driver::Address(0x00); Driver::Read(data0);
			Driver::Select(1); Driver::Address(0x00); Driver::Read(data1);
			m_chips = (data0 != data1 ? 2 : 1);

			// clear the test values: the registers of the chips
			// must be zero as the state of Advanced expects
			Driver::Select(1); Driver::Address(0x00); Driver::Write(0x00);
			Driver::Select(0); Driver::Address(0x00); Driver::Write(0x00);

			// chip 1 only with TurboSound
			if (m_chips == 2) on_bus(1).Init();
			m_current = 0;
		}

		// RESET# is common for both chips, no selection
		void Reset()
		{
			for (uint8_t chip = 0; chip < m_chips; ++chip) m_psg[chip].Reset();
		}

		// set the volumes of both chips to zero
		void Mute()
		{
			for (uint8_t chip = 0; chip < m_chips; ++chip) on_bus(chip).Mute();
		}

		// the clock is common for both chips, Advanced
		// writes the converted periods to each chip
		void SetClock(Clock clock)
		{
			for (uint8_t chip = 0; chip < m_chips; ++chip) on_bus(chip).SetClock(clock);
		}

		// Advanced only, the same stereo for both chips
		void SetStereo(Stereo stereo)
		{
			for (uint8_t chip = 0; chip < m_chips; ++chip) on_bus(chip).SetStereo(stereo);
		}

		Board GetBoard() const
		{
			return (m_chips == 2 ? Board::TurboSound : Board::SingleChip);
		}

		// select the current chip for the calls below
		void Select(uint8_t chip)
		{
			m_current = bool(chip); // 0 or 1
		}

		uint8_t GetSelected() const
		{
			return m_current;
		}

		// the same clock for both chips
		Clock GetClock() const
		{
			return m_psg[0].GetClock();
		}

		ChipId GetChipId() const
		{
			return m_psg[m_current].GetChipId();
		}

		// Advanced only, the same stereo for both chips
		Stereo GetStereo() const
		{
			return m_psg[0].GetStereo();
		}

		void SetRegister(raddr_t addr, rdata_t data)
		{
			if (m_current < m_chips) on_bus(m_current).SetRegister(addr, data);
		}

		rdata_t GetRegister(raddr_t addr)
		{
			return (m_current < m_chips ? on_bus(m_current).GetRegister(addr) : 0xFF);
		}

		// Advanced only, write the changes of both chips
		void Update()
		{
			for (uint8_t chip = 0; chip < m_chips; ++chip) on_bus(chip).Update();
		}

	private:
		// select the chip on the bus for the next call of its model
		PSGModel& on_bus(uint8_t chip)
		{
			Driver::Select(chip);
			return m_psg[chip];
		}

	private:
		PSGModel m_psg[2];
		uint8_t  m_chips{ 1 };   // 1, or 2 with TurboSound
		uint8_t  m_current{ 0 }; // chip selected by Select
	};
}
