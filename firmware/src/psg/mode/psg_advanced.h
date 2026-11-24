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
		void Reset() override;

		void SetClock(Clock clock) override;
		Clock GetClock() const override;

		void SetStereo(Stereo stereo);
		Stereo GetStereo() const;

		void SetRegister(raddr_t addr, rdata_t data) override;
		void GetRegister(raddr_t addr, rdata_t &data) const override;
		void Commit();

	private:
		void set_register(state_t& state, Reg reg, rdata_t data);
		void get_register(const state_t& state, Reg reg, rdata_t &data) const;

		void process_clock_conversion();
		void process_channels_remapping();
		void process_compatible_mode_fix();

		void check_output_changes(const state_t& chip);
		void write_output_to_chip(const state_t& chip);

	private:
		Clock m_clock;
		Stereo m_sstereo{ Stereo::ABC };
		Stereo m_dstereo{ Stereo::ABC };
		state_t m_input, m_output;
	};
}
