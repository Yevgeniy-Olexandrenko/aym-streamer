#pragma once

#include <array>
#include <iostream>
#include "Register.h"

enum
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
	PortB_Data   = 0x0F
};

class Frame
{
	using RegisterArray = std::array<Register, 16>;
	friend std::ostream& operator<<(std::ostream& stream, const Frame& frame);

public:
	Frame();
	Frame(const Frame& other);

public:
	operator bool() const; // has changed registers
	Register& operator[](uint8_t index); // access register
	const Register& operator[](uint8_t index) const; // access register

	void SetUnchanged();
	void FixValues();

private:
	RegisterArray m_registers;
};
