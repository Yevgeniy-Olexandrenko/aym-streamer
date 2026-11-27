
#include "psg_advanced.h"
#include <avr/pgmspace.h>
#include <string.h>

namespace PSG
{
	enum Reg : uint8_t
	{
		// bank A registers
		A_Fine    = 0x00, // Tone A period (fine tune)
		A_Coarse  = 0x01, // Tone A period (coarse tune)
		B_Fine    = 0x02, // Tone B period (fine tune)
		B_Coarse  = 0x03, // Tone B period (coarse tune)
		C_Fine    = 0x04, // Tone C period (fine tune)
		C_Coarse  = 0x05, // Tone C period (coarse tune)
		N_Period  = 0x06, // Noise period
		Mixer     = 0x07, // Mixer control + I/O enable
		A_Volume  = 0x08, // Amplitude A control
		B_Volume  = 0x09, // Amplitude B control
		C_Volume  = 0x0A, // Amplitude C control
		EA_Fine   = 0x0B, // Envelope/AY8930 Envelope A period (fine tune)
		EA_Coarse = 0x0C, // Envelope/AY8930 Envelope A period (coarse tune)
		EA_Shape  = 0x0D, // Envelope/AY8930 Envelope A shape + AY8930 mode/bank

		// bank B registers
		EB_Fine   = 0x10, // AY8930 Envelope B period (fine tune)
		EB_Coarse = 0x11, // AY8930 Envelope B period (coarse tune)
		EC_Fine   = 0x12, // AY8930 Envelope C period (fine tune)
		EC_Coarse = 0x13, // AY8930 Envelope C period (coarse tune)
		EB_Shape  = 0x14, // AY8930 Envelope B shape
		EC_Shape  = 0x15, // AY8930 Envelope C shape
		A_Duty    = 0x16, // AY8930 Duty A cycle
		B_Duty    = 0x17, // AY8930 Duty B cycle
		C_Duty    = 0x18, // AY8930 Duty C cycle
		N_AndMask = 0x19, // AY8930 Noise "AND" mask
		N_OrMask  = 0x1A, // AY8930 Noise "OR" mask

		// aliases
		E_Fine    = EA_Fine,   // comp mode alias for Envelope period (fine tune) 
		E_Coarse  = EA_Coarse, // comp mode alias for Envelope period (coarse tune) 
		E_Shape   = EA_Shape,  // comp mode alias for Envelope shape
		Mode_Bank = EA_Shape,  // alias for AY8930 mode/bank register

		// bank bounds
		BankA_Fst = A_Fine,    // first register in bank A
		BankA_Lst = EA_Shape,  // last register in bank A
		BankB_Fst = EB_Fine,   // first register in bank B
		BankB_Lst = N_OrMask,  // last register in bank B
	};

	const raddr_t e_shape[] PROGMEM = { EA_Shape, EB_Shape, EC_Shape };

	constexpr uint32_t to_mask(raddr_t reg)
	{
		return (UINT32_C(1) << reg);
	}

	template<typename T>
	inline void swap_values(T& a, T& b)
	{
		T t = a; a = b; b = t;
	}

	// swap the values of two bits given by masks
	template<typename T>
	inline void swap_bits(T& value, T mask_a, T mask_b)
	{
		if (bool(value & mask_a) != bool(value & mask_b)) value ^= (mask_a | mask_b);
	}

	// masks of all registers and of envelope shape registers
	constexpr uint32_t all_regs_mask =
		(to_mask(BankA_Lst + 1) - to_mask(BankA_Fst)) |
		(to_mask(BankB_Lst + 1) - to_mask(BankB_Fst));
	constexpr uint32_t e_shape_mask =
		to_mask(EA_Shape) | to_mask(EB_Shape) | to_mask(EC_Shape);

	void Advanced::Init()
	{
		// the chip is reset with the default clock
		Basic::Init();
		m_clock = Clock::F_DEF;
		memset(&m_input, 0, sizeof(state_t));
		memset(&m_output, 0, sizeof(state_t));
	}

	void Advanced::Reset()
	{
		// after reset all registers of the chip are zero
		Basic::Reset();
		memset(&m_input, 0, sizeof(state_t));
		memset(&m_output, 0, sizeof(state_t));
	}

	void Advanced::SetClock(Clock clock)
	{
		// limit the clock frequency range
		if (clock < Clock::F_MIN) clock = Clock::F_MIN;
		if (clock > Clock::F_MAX) clock = Clock::F_MAX;

		// set real/virtual clock frequency and
		// convert the periods on the chip to it
		Basic::SetClock(clock);
		m_clock = clock;
		update_output();
	}

	Clock Advanced::GetClock() const
	{
		// return virtual clock frequency
		return m_clock;
	}

	void Advanced::SetStereo(Stereo stereo)
	{
	#ifndef DISABLE_CHANNELS_REMAPPING
		// remap the channels on the chip at once
		m_stereo = stereo;
		update_output();
	#endif
	}

	Stereo Advanced::GetStereo() const
	{
		return m_stereo;
	}

	void Advanced::SetRegister(raddr_t addr, rdata_t data)
	{
		// set register data indirectly via bank switching
		// register address must be in range 0x00-0x0F
		if (addr < 0x10)
		{
			// redirect access to registers of bank B if
			// exp mode is active and bank B is selected
			if (m_input.status.exp_mode == 0xB0)
			{
				addr += BankB_Fst;
			}
			set_register(m_input, Reg(addr), data);
		}
	}

	rdata_t Advanced::GetRegister(raddr_t addr) const
	{
		// get register data indirectly via bank switching
		// register address must be in range 0x00-0x0F
		rdata_t data = 0xFF;
		if (addr < 0x10)
		{
			// redirect access to registers of bank B if
			// exp mode is active and bank B is selected
			if (m_input.status.exp_mode == 0xB0)
			{
				addr += BankB_Fst;
			}
			get_register(m_input, Reg(addr), data);
		}
		return data;
	}

	void Advanced::Update()
	{
		if (m_input.status.changed) update_output();
	}

	void Advanced::update_output()
	{
		// the output state of the previous update
		// is the state of registers on the chip
		state_t chip;
		memcpy(&chip, &m_output, sizeof(state_t));

		// copy input state to output and reset changes
		memcpy(&m_output, &m_input, sizeof(state_t));
		m_input.status.changed = 0;

		// process output state and write to PSG
		process_clock_conversion();
		process_channels_remapping();
		process_compatible_mode_fix();
		check_output_changes(chip);
		write_output_to_chip(chip);
	}

	void Advanced::set_register(state_t &state, Reg reg, rdata_t data)
	{
		// preserve the state of exp mode and bank of regs
		// separately from the shape of channel A envelope,
		// only AY8930 has exp mode: 0xA0 bank A, 0xB0 bank B
		if ((reg & 0x0F) == Mode_Bank)
		{
			bool exp_mode = (GetChipId() == ChipId::AY8930 && (data & 0xE0) == 0xA0);
			state.status.exp_mode = (exp_mode ? (data & 0xF0) : 0x00);
			data &= 0x0F; reg = Mode_Bank;
		}

		// map register to corresponding field of state
		switch(reg)
		{
			// bank A
			case A_Fine:    state.channels[0].t_period.fine = data; break;
			case A_Coarse:  state.channels[0].t_period.coarse = data; break;
			case B_Fine:    state.channels[1].t_period.fine = data; break;
			case B_Coarse:  state.channels[1].t_period.coarse = data; break;
			case C_Fine:    state.channels[2].t_period.fine = data; break;
			case C_Coarse:  state.channels[2].t_period.coarse = data; break;
			case N_Period:  state.commons.n_period = data; break;
			case Mixer:     state.commons.mixer = data; break;
			case A_Volume:  state.channels[0].t_volume = data; break;
			case B_Volume:  state.channels[1].t_volume = data; break;
			case C_Volume:  state.channels[2].t_volume = data; break;
			case EA_Fine:   state.channels[0].e_period.fine = data; break;
			case EA_Coarse: state.channels[0].e_period.coarse = data; break;
			case EA_Shape:  state.channels[0].e_shape = data; break;

			// bank B
			case EB_Fine:   state.channels[1].e_period.fine = data; break;
			case EB_Coarse: state.channels[1].e_period.coarse = data; break;
			case EC_Fine:   state.channels[2].e_period.fine = data; break;
			case EC_Coarse: state.channels[2].e_period.coarse = data; break;
			case EB_Shape:  state.channels[1].e_shape = data; break;
			case EC_Shape:  state.channels[2].e_shape = data; break;
			case A_Duty:    state.channels[0].t_duty = data; break;
			case B_Duty:    state.channels[1].t_duty = data; break;
			case C_Duty:    state.channels[2].t_duty = data; break;
			case N_AndMask: state.commons.n_and_mask = data; break;
			case N_OrMask:  state.commons.n_or_mask = data; break;

			// I/O ports and unused addresses
			default: return;
		}

		// mark register as changed
		state.status.changed |= to_mask(reg);
	}

	void Advanced::get_register(const state_t &state, Reg reg, rdata_t &data) const
	{
		// the 'mode/bank' register is in both banks
		if ((reg & 0x0F) == Mode_Bank) reg = Mode_Bank;

		// map register to corresponding field of state
		switch(reg)
		{
			// bank A
			case A_Fine:    data = state.channels[0].t_period.fine; break;
			case A_Coarse:  data = state.channels[0].t_period.coarse; break;
			case B_Fine:    data = state.channels[1].t_period.fine; break;
			case B_Coarse:  data = state.channels[1].t_period.coarse; break;
			case C_Fine:    data = state.channels[2].t_period.fine; break;
			case C_Coarse:  data = state.channels[2].t_period.coarse; break;
			case N_Period:  data = state.commons.n_period; break;
			case Mixer:     data = state.commons.mixer; break;
			case A_Volume:  data = state.channels[0].t_volume; break;
			case B_Volume:  data = state.channels[1].t_volume; break;
			case C_Volume:  data = state.channels[2].t_volume; break;
			case EA_Fine:   data = state.channels[0].e_period.fine; break;
			case EA_Coarse: data = state.channels[0].e_period.coarse; break;
			case EA_Shape:  data = state.channels[0].e_shape; break;

			// bank B
			case EB_Fine:   data = state.channels[1].e_period.fine; break;
			case EB_Coarse: data = state.channels[1].e_period.coarse; break;
			case EC_Fine:   data = state.channels[2].e_period.fine; break;
			case EC_Coarse: data = state.channels[2].e_period.coarse; break;
			case EB_Shape:  data = state.channels[1].e_shape; break;
			case EC_Shape:  data = state.channels[2].e_shape; break;
			case A_Duty:    data = state.channels[0].t_duty; break;
			case B_Duty:    data = state.channels[1].t_duty; break;
			case C_Duty:    data = state.channels[2].t_duty; break;
			case N_AndMask: data = state.commons.n_and_mask; break;
			case N_OrMask:  data = state.commons.n_or_mask; break;

			// I/O ports and unused addresses
			default: data = 0xFF; return;
		}

		// combine the current state of exp mode and bank
		// of regs with the shape of channel A envelope
		if (reg == Mode_Bank)
		{
			data |= state.status.exp_mode;
		}
	}

	void Advanced::process_clock_conversion()
	{
	#ifndef DISABLE_CLOCK_CONVERSION
		const uint32_t vclock = uint32_t(GetClock());
		const uint32_t rclock = uint32_t(Basic::GetClock());

		if (rclock != vclock)
		{
			// the same algorithm as ChipClockRateConvert in psg-tools
			const bool exp_mode = m_output.status.exp_mode;
			const rpair_t t_bound = (exp_mode ? 0xFFFF : 0x0FFF);
			const rpair_t n_bound = (exp_mode ? 0x00FF : 0x001F);
			const rdata_t e_enable = (exp_mode ? 0x20 : 0x10);

			// period * rclock / vclock, rounded to nearest. The period is
			// split into bytes, so all products fit 32 bits up to 2 MHz
			const auto convert_period = [&](rpair_t period, rpair_t bound)
			{
				uint32_t hi = (uint32_t(period >> 8) * rclock);
				uint32_t lo = ((hi % vclock) * 256 + uint32_t(period & 0xFF) * rclock + vclock / 2);
				uint32_t converted = ((hi / vclock) * 256 + lo / vclock);
				return rpair_t(converted > bound ? bound : converted);
			};

			// one period is a power of two multiple of another
			const auto is_synced = [](rpair_t tone, rpair_t envelope)
			{
				if (!tone || !envelope) return false;
				uint32_t lo = (tone < envelope ? tone : envelope);
				rpair_t  hi = (tone < envelope ? envelope : tone);
				while (lo < hi) lo <<= 1;
				return (lo == hi);
			};

			// convert envelope periods: each channel has its own
			// envelope in exp mode, channel A envelope is common
			// for all channels in comp mode
			rpair_t e_src[3];
			for (uint8_t i = 0; i < (exp_mode ? 3 : 1); ++i)
			{
				channel_t& channel = m_output.channels[i];
				e_src[i] = channel.e_period.full;
				channel.e_period.full = convert_period(e_src[i], 0xFFFF);
			}

			// convert tone periods
			for (uint8_t i = 0; i < 3; ++i)
			{
				channel_t& channel = m_output.channels[i];
				const channel_t& envelope = m_output.channels[exp_mode ? i : 0];
				const rpair_t t_period = channel.t_period.full;
				const rpair_t e_period = e_src[exp_mode ? i : 0];

				// tone and periodic envelope are often tuned to a power of
				// two ratio (buzzer, sync bass), separate rounding of each
				// period makes them beat, so derive tone from envelope
				if (is_synced(t_period, e_period) &&
					!isb_set(m_output.commons.mixer, i) &&
					(channel.t_volume & e_enable) &&
					(envelope.e_shape & 0x09) == 0x08)
				{
					uint32_t converted = ((uint32_t(t_period) * envelope.e_period.full + e_period / 2) / e_period);
					channel.t_period.full = rpair_t(converted > t_bound ? t_bound : converted);
				}
				else
				{
					channel.t_period.full = convert_period(t_period, t_bound);
				}
			}

			// convert noise period
			m_output.commons.n_period = rdata_t(convert_period(m_output.commons.n_period, n_bound));
		}
	#endif
	}

	void Advanced::process_channels_remapping()
	{
	#ifndef DISABLE_CHANNELS_REMAPPING
		const bool exp_mode = m_output.status.exp_mode;
		const Stereo stereo = GetStereo();

		if (stereo != Stereo::ABC)
		{
			const auto swap_channels = [&](uint8_t l, uint8_t r)
			{
				channel_t& ch_l = m_output.channels[l];
				channel_t& ch_r = m_output.channels[r];

				// tone period, duty cycle and volume/envelope enable
				swap_values(ch_l.t_period.full, ch_r.t_period.full);
				swap_values(ch_l.t_duty, ch_r.t_duty);
				swap_values(ch_l.t_volume, ch_r.t_volume);

				// each channel has its own envelope in exp mode, a shape
				// write restarts the envelope, so the change moves too
				if (exp_mode)
				{
					swap_values(ch_l.e_period.full, ch_r.e_period.full);
					swap_values(ch_l.e_shape, ch_r.e_shape);
					swap_bits(m_output.status.changed,
						to_mask(pgm_read_byte(e_shape + l)),
						to_mask(pgm_read_byte(e_shape + r)));
				}

				// tone and noise enable bits in mixer
				swap_bits(m_output.commons.mixer, rdata_t(0x01 << l), rdata_t(0x01 << r));
				swap_bits(m_output.commons.mixer, rdata_t(0x08 << l), rdata_t(0x08 << r));
			};

			// swap channels in pairs
			switch(stereo)
			{
			case Stereo::ABC: break;
			case Stereo::ACB: swap_channels(1, 2); break; // B <-> C
			case Stereo::BAC: swap_channels(0, 1); break; // A <-> B
			case Stereo::BCA: swap_channels(0, 1); swap_channels(1, 2); break; // A <-> B, B <-> C
			case Stereo::CAB: swap_channels(1, 2); swap_channels(0, 1); break; // B <-> C, A <-> B
			case Stereo::CBA: swap_channels(0, 2); break; // A <-> C
			}
		}
	#endif
	}

	void Advanced::process_compatible_mode_fix()
	{
	#ifndef DISABLE_COMPATIBLE_MODE_FIX
		if (GetChipId() == ChipId::AY8930)
		{
			for (int i = 0; i < 3; ++i)
			{
				rdata_t volume = m_output.channels[i].t_volume;
				if (m_output.status.exp_mode) volume >>= 1;

				// get tone, noise and envelope enable flags
				bool t_disable = isb_set(m_output.commons.mixer, 0 + i);
				bool n_disable = isb_set(m_output.commons.mixer, 3 + i);
				bool e_enable  = isb_set(volume, 4);

				// special case - pure envelope
				if (e_enable && t_disable && n_disable)
				{
					// fix by enabling inaudible tone
					m_output.channels[i].t_period.full = 0;
					m_output.channels[i].t_duty = 0x08;
					res_bit(m_output.commons.mixer, 0 + i);
				}
			}
		}
	#endif    
	}

	void Advanced::check_output_changes(const state_t& chip)
	{
		// a write to the envelope shape register restarts
		// the envelope, so keep the shape writes of the host
		m_output.status.changed &= e_shape_mask;

		// the mode switch clears all registers of the chip
		if (bool(m_output.status.exp_mode) != bool(chip.status.exp_mode))
		{
			m_output.status.changed = all_regs_mask;
			return;
		}

		// write the registers that differ from the chip
		rdata_t o_data, c_data;
		for (raddr_t addr = BankA_Fst; addr <= BankB_Lst; ++addr)
		{
			// skip the gap between bank A and bank B
			if (addr == BankA_Lst + 1) addr = BankB_Fst;

			get_register(m_output, Reg(addr), o_data);
			get_register(chip, Reg(addr), c_data);

			if (o_data != c_data)
				m_output.status.changed |= to_mask(addr);
		}
	}

	void Advanced::write_output_to_chip(const state_t& chip)
	{
		// write changes in output state to the PSG
		// in the same order as ChipEncoder in psg-tools
		const bool has_exp_mode = (GetChipId() == ChipId::AY8930);
		const bool mode_switch = (bool(m_output.status.exp_mode) != bool(chip.status.exp_mode));
		const bool exp_mode = (has_exp_mode && m_output.status.exp_mode);

		rdata_t data, mode_bank;
		bool switch_banks = false;

		// value of the 'mode/bank' register with bank A selected
		get_register(m_output, Mode_Bank, mode_bank);
		if (exp_mode) { mode_bank &= 0x0F; mode_bank |= 0xA0; }

		// the mode switch clears the registers of
		// the chip, so 'mode/bank' register goes first
		if (has_exp_mode && mode_switch)
		{
			Basic::SetRegister(Mode_Bank, mode_bank);
		}

		// check for changes in registers of bank A,
		// except the 'mode/bank' register
		for (raddr_t addr = A_Fine; addr <= EA_Coarse; ++addr)
		{
			if (m_output.status.changed & to_mask(addr))
			{
				// send register data to chip (within bank A)
				get_register(m_output, Reg(addr), data);
				Basic::SetRegister(addr & 0x0F, data);
			}
		}

		if (exp_mode)
		{
			// check for changes in registers of bank B
			for (raddr_t addr = EB_Fine; addr <= N_OrMask; ++addr)
			{
				if (m_output.status.changed & to_mask(addr))
				{
					// we have changes, so first
					// of all we switch to bank B
					if (!switch_banks)
					{
						switch_banks = true;
						Basic::SetRegister(Mode_Bank, mode_bank | 0x10);
					}

					// send register data to chip (within bank B)
					get_register(m_output, Reg(addr), data);
					Basic::SetRegister(addr & 0x0F, data);
				}
			}
		}

		// 'mode/bank' register goes last: envelope A restarts
		// after its period is set, and bank A is selected again
		if (switch_banks || (m_output.status.changed & to_mask(Mode_Bank)))
		{
			Basic::SetRegister(Mode_Bank, mode_bank);
		}
	}
}
