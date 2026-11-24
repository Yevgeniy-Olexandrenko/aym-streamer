
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

	const raddr_t e_fine   [] PROGMEM = { EA_Fine,   EB_Fine,   EC_Fine   };
	const raddr_t e_coarse [] PROGMEM = { EA_Coarse, EB_Coarse, EC_Coarse };
	const raddr_t e_shape  [] PROGMEM = { EA_Shape,  EB_Shape,  EC_Shape  };

	constexpr uint32_t to_mask(raddr_t reg)
	{
		return (UINT32_C(1) << reg);
	}

	void Advanced::Reset()
	{
		Basic::Reset();
		memset(&m_input, 0, sizeof(state_t));
	}

	void Advanced::SetClock(Clock clock)
	{
		// limit the clock frequency range
		if (clock < Clock::F_MIN) clock = Clock::F_MIN;
		if (clock > Clock::F_MAX) clock = Clock::F_MAX;

		// set real/virtual clock frequency
		Basic::SetClock(clock);
		m_clock = clock;
	}

	Clock Advanced::GetClock() const
	{
		// return virtual clock frequency
		return m_clock;
	}

	void Advanced::SetStereo(Stereo stereo)
	{
		m_sstereo = stereo;
		m_dstereo = stereo;
	}

	Stereo Advanced::GetStereo() const
	{
		return m_dstereo;
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

	void Advanced::GetRegister(raddr_t addr, rdata_t &data) const
	{
		// get register data indirectly via bank switching
		// register address must be in range 0x00-0x0F
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
	}

	void Advanced::Commit()
	{
		if (m_input.status.changed)
		{
			// the output state still has the previous mode
			bool mode_switch = (bool(m_input.status.exp_mode) != bool(m_output.status.exp_mode));

			// copy input state to output and reset changes
			memcpy(&m_output, &m_input, sizeof(state_t));
			m_input.status.changed = 0;

			// process output state and write to PSG
			process_clock_conversion();
			process_channels_remapping();
			process_compatible_mode_fix();
			
			check_output_changes();
			write_output_to_chip(mode_switch);
		}
	}

	void Advanced::set_register(state_t &state, Reg reg, rdata_t data)
	{
		// preserve the state of exp mode and bank of regs
		// separately from the shape of channel A envelope
		if ((reg & 0x0F) == Mode_Bank)
		{
			state.status.exp_mode = (data & 0xF0);
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
		}

		// mark register as changed
		state.status.changed |= to_mask(reg);
	}

	void Advanced::get_register(const state_t &state, Reg reg, rdata_t &data) const
	{
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
		}

		// combine the current state of exp mode and bank
		// of regs with the shape of channel A envelope
		if ((reg & 0x0F) == Mode_Bank)
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
			rpair_t t_bound = (m_output.status.exp_mode ? 0xFFFF : 0x0FFF);
			rpair_t n_bound = (m_output.status.exp_mode ? 0x00FF : 0x001F);

			// safe period conversion based on clock ratio
			const auto convert_period = [&](rpair_t& period, rpair_t bound)
			{
				uint32_t converted = ((period * (rclock >> 5) / (vclock >> 6) + 1) >> 1);
				period = rpair_t(converted > bound ? bound : converted);
			};

			// convert tone and envelope periods
			for (int i = 0; i < 3; ++i)
			{
				convert_period(m_output.channels[i].t_period.full, t_bound);
				convert_period(m_output.channels[i].e_period.full, 0xFFFF);
			}
			m_output.status.changed |= 0b01100001111111; // hack

			// convert noise period
			rpair_t period = m_output.commons.n_period;
			convert_period(period, n_bound);
			m_output.commons.n_period = rdata_t(period);
		}
	#endif
	}

	void Advanced::process_channels_remapping()
	{
	#ifndef DISABLE_CHANNELS_REMAPPING
		// restrict stereo modes available for exp mode
		m_dstereo = (m_output.status.exp_mode && m_sstereo != Stereo::ABC && m_sstereo != Stereo::ACB)
			? Stereo::ABC
			: m_sstereo;

		if (m_dstereo != Stereo::ABC)
		{
			const auto swap_register = [&](raddr_t reg_l, raddr_t reg_r)
			{
				rdata_t data_l, data_r;
				get_register(m_output, Reg(reg_l), data_l);
				get_register(m_output, Reg(reg_r), data_r);
				set_register(m_output, Reg(reg_l), data_r);
				set_register(m_output, Reg(reg_r), data_l);
			};

			const auto swap_channels = [&](raddr_t l, raddr_t r)
			{
				// swap tone period and tone volume/envelope enable
				swap_register(A_Fine + 2 * l, A_Fine + 2 * r);
				swap_register(A_Coarse + 2 * l, A_Coarse + 2 * r);
				swap_register(A_Volume + l, A_Volume + r);

				if (m_output.status.exp_mode)
				{
					// swap tone duty cycle and envelope period
					swap_register(A_Duty + l, A_Duty + r);
					swap_register(pgm_read_byte(e_fine + l), pgm_read_byte(e_fine + r));
					swap_register(pgm_read_byte(e_coarse + l), pgm_read_byte(e_coarse + r));

					// swap envelope shape
					raddr_t shape_l = pgm_read_byte(e_shape + l);
					raddr_t shape_r = pgm_read_byte(e_shape + r);
					bool schanged_l = (m_output.status.changed & to_mask(shape_l));
					bool schanged_r = (m_output.status.changed & to_mask(shape_r));
					swap_register(shape_l, shape_r);
					if (schanged_l) m_output.status.changed |= to_mask(shape_r);
					else m_output.status.changed &= ~to_mask(shape_r);
					if (schanged_r) m_output.status.changed |= to_mask(shape_l);
					else m_output.status.changed &= ~to_mask(shape_l);
				}

				// swap bits in mixer register
				rdata_t msk_l = (0b00001001 << l);
				rdata_t msk_r = (0b00001001 << r);
				rdata_t mix_l = ((m_output.commons.mixer & msk_l) >> l);
				rdata_t mix_r = ((m_output.commons.mixer & msk_r) >> r);
				res_bits(m_output.commons.mixer, msk_l | msk_r);
				set_bits(m_output.commons.mixer, mix_l << r);
				set_bits(m_output.commons.mixer, mix_r << l);
			};

			// swap channels in pairs
			switch(m_dstereo)
			{
			case Stereo::ACB:
				swap_channels(1, 2); // B <-> C
				break;

			case Stereo::BAC:
				swap_channels(0, 1); // A <-> B
				break;

			case Stereo::BCA:
				swap_channels(0, 1); // A <-> B
				swap_channels(1, 2); // B <-> C
				break;

			case Stereo::CAB:
				swap_channels(1, 2); // B <-> C
				swap_channels(0, 1); // A <-> B
				break;

			case Stereo::CBA:
				swap_channels(0, 2); // A <-> C
				break;

			default: break;
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

	void Advanced::check_output_changes()
	{
		// apply changes in registers made by processing
		rdata_t i_data, o_data;
		for (raddr_t addr = BankA_Fst; addr <= BankB_Lst; ++addr)
		{
			get_register(m_input,  Reg(addr), i_data);
			get_register(m_output, Reg(addr), o_data);

			if (i_data != o_data)
				m_output.status.changed |= to_mask(addr);
		}
	}

	void Advanced::write_output_to_chip(bool mode_switch)
	{
		// write changes in output state to the PSG
		// in the same order as ChipEncoder in psg-tools
		const bool has_exp_mode = (GetChipId() == ChipId::AY8930);
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
