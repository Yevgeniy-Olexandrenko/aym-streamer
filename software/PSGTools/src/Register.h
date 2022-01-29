#pragma once

#include <stdint.h>

class Register
{
public:
	enum class Index
	{ 
		TonA_PeriodL = 0x00,
		TonA_PeriodH = 0x01,
		TonB_PeriodL = 0x02,
		TonB_PeriodH = 0x03,
		TonC_PeriodL = 0x04,
		TonC_PeriodH = 0x05,
		Noise_Period = 0x06,
		Mixer_Flags  = 0x07,
		VolA_EnvFlg  = 0x08,
		VolB_EnvFlg  = 0x09,
		VolC_EnvFlg  = 0x0A,
		Env_PeriodL  = 0x0B,
		Env_PeriodH  = 0x0C,
		Env_Shape    = 0x0D,
		PortA_Data   = 0x0E,
		PortB_Data   = 0x0F,
		COUNT
	};

	Register(); // zero
	Register(uint8_t data); // init
	Register(const Register& other); // copy

public:
	operator bool() const; // get changed
	operator uint8_t() const; // get value
	Register& operator=(uint8_t data); // assign

private:
	uint8_t m_data;
	bool m_changed;
};