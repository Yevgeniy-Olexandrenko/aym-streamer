#pragma once

#include <stdint.h>

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
public:
	Frame() = default;
	Frame(const Frame& other);

public:
	uint8_t Read(uint8_t chip, uint8_t reg) const;
	bool    IsChanged(uint8_t chip, uint8_t reg) const;

	uint8_t Read(uint8_t reg) const;
	bool    IsChanged(uint8_t reg) const;

	void    Write(uint8_t chip, uint8_t reg, uint8_t data);
	void    Update(uint8_t chip, uint8_t reg, uint8_t data);

	void    Write(uint8_t reg, uint8_t data);
	void    Update(uint8_t reg, uint8_t data);

	void    Reset();
	void    ResetData();
	void    ResetChanges();
	
public:
	uint8_t& data(uint8_t chip, uint8_t reg);
	bool& changed(uint8_t chip, uint8_t reg);

private:
	uint8_t m_data[2][16]{};
	bool m_changes[2][16]{};
};
