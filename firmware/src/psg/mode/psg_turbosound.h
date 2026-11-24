#pragma once

#include "psg_advanced.h"

namespace PSG
{
	enum class Board : uint8_t
	{
		SingleChip, // one PSG in the socket of the streamer
		TurboSound  // TurboSound adapter with two PSG sockets
	};

	// Two PSG on the TurboSound adapter, both of Basic or of Advanced mode.
	// The adapter selects the chip by a latch address cycle with 0xFF (chip 0)
	// or 0xFE (chip 1). Both chips share the clock and the RESET# signal.
	// The calls for a chip that is not found (ChipId::NotFound) are ignored.
	template<class PSGMode>
	class TurboSound
	{
	public:
		// power on the chips, detect the board and the chips
		void PowerOn()
		{
			// the bus pins must be set before the first selection,
			// the selection of the adapter is unknown after power on
			Driver::PowerOn();
			m_on_bus = 0xFF;
			select_on_bus(0);
			m_psg[0].PowerOn();

			// with the adapter each chip keeps its own value, a single
			// chip keeps the last value written, no chip gives 0xFF
			rdata_t data0, data1;
			select_on_bus(0); Driver::Address(0x00); Driver::Write(0x55);
			select_on_bus(1); Driver::Address(0x00); Driver::Write(0xAA);
			select_on_bus(0); Driver::Address(0x00); Driver::Read(data0);
			select_on_bus(1); Driver::Address(0x00); Driver::Read(data1);
			m_board = (data0 != data1 ? Board::TurboSound : Board::SingleChip);

			// clear the test values: the registers of the chips
			// must be zero as the state of Advanced expects
			select_on_bus(1); Driver::Address(0x00); Driver::Write(0x00);
			select_on_bus(0); Driver::Address(0x00); Driver::Write(0x00);

			// chip 1 only on the adapter
			if (m_board == Board::TurboSound)
			{
				select_on_bus(1);
				m_psg[1].PowerOn();
			}
			m_current = 0;
		}

		// RESET# is common for both chips
		void Reset()
		{
			m_psg[0].Reset();
			m_psg[1].Reset();
		}

		// the clock is common for both chips
		void SetClock(Clock clock)
		{
			m_psg[0].SetClock(clock);
			m_psg[1].SetClock(clock);
		}

		// Advanced only, the same stereo for both chips
		void SetStereo(Stereo stereo)
		{
			m_psg[0].SetStereo(stereo);
			m_psg[1].SetStereo(stereo);
		}

		Board GetBoard() const
		{
			return m_board;
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

		Clock GetClock() const
		{
			return m_psg[0].GetClock();
		}

		ChipId GetChipId() const
		{
			return m_psg[m_current].GetChipId();
		}

		void SetRegister(raddr_t addr, rdata_t data)
		{
			if (is_found(m_current))
			{
				select_on_bus(m_current);
				m_psg[m_current].SetRegister(addr, data);
			}
		}

		void GetRegister(raddr_t addr, rdata_t &data)
		{
			if (is_found(m_current))
			{
				select_on_bus(m_current);
				m_psg[m_current].GetRegister(addr, data);
			}
			else data = 0xFF;
		}

		// Advanced only, write the changes of both chips
		void Commit()
		{
			for (uint8_t chip = 0; chip < 2; ++chip)
			{
				if (is_found(chip))
				{
					select_on_bus(chip);
					m_psg[chip].Commit();
				}
			}
		}

	private:
		bool is_found(uint8_t chip) const
		{
			// a chip is NotFound until its PowerOn finds it
			return (m_psg[chip].GetChipId() != ChipId::NotFound);
		}

		// latch 0xFF or 0xFE only when the selection changes
		void select_on_bus(uint8_t chip)
		{
			if (m_on_bus != chip)
			{
				Driver::Address(0xFF - chip);
				m_on_bus = chip;
			}
		}

	private:
		PSGMode m_psg[2];
		Board   m_board{ Board::SingleChip };
		uint8_t m_current{ 0 };    // chip selected by Select
		uint8_t m_on_bus{ 0xFF };  // chip selected on the adapter, 0xFF unknown
	};
}
