#pragma once

#include "psg_basic.h"

//#define DISABLE_CLOCK_CONVERSION
//#define DISABLE_CHANNELS_REMAPPING
//#define DISABLE_COMPATIBLE_MODE_FIX

namespace PSG
{
	enum Reg : uint8_t;

	enum class Stereo : uint8_t
	{
		ABC, ACB, BAC, BCA, CAB, CBA
	};

	class Advanced : public Basic
	{
		using rpair_t = uint16_t;

		union period_t
		{
			rpair_t full;
			struct { rdata_t fine, coarse; };
		};

		struct channel_t
		{
			period_t t_period;
			rdata_t  t_volume;
			rdata_t  t_duty;
			period_t e_period;
			rdata_t  e_shape;
		};

		struct commons_t
		{
			rdata_t n_period;
			rdata_t n_and_mask;
			rdata_t n_or_mask;
			rdata_t mixer;
		};

		struct status_t
		{
			uint32_t changed;
			uint8_t  exp_mode;
		};

		struct state_t
		{
			channel_t channels[3];
			commons_t commons;
			status_t  status;
		};

	public:
		// these methods hide the methods of Basic,
		// there are no virtual calls between them
		void Init();
		void Reset();
		void Mute();

		// get returns the clock requested by the host, the
		// periods on the chip are converted to the real clock
		void SetClock(Clock clock);
		Clock GetClock() const;

		// The host picks a stereo that suits the stream. In exp mode
		// it must be ABC or ACB: other layouts move channel A to bank B,
		// and each bank switch restarts the envelope on chip channel A.
		void SetStereo(Stereo stereo);
		Stereo GetStereo() const;

		void SetRegister(raddr_t addr, rdata_t data);
		rdata_t GetRegister(raddr_t addr) const;
		void Update();

	private:
		void set_register(state_t& state, Reg reg, rdata_t data);
		void get_register(const state_t& state, Reg reg, rdata_t &data) const;

		void update_output();
		void process_clock_conversion();
		void process_channels_remapping();
		void process_compatible_mode_fix();

		void check_output_changes(const state_t& chip);
		void write_output_to_chip(const state_t& chip);

	private:
		Clock m_clock{ Clock::F_DEF };
		Stereo m_stereo{ Stereo::ABC };
		state_t m_input, m_output;
	};
}
