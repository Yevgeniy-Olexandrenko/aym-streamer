#pragma once

#include <stdint.h>

enum
{
	A_Fine   = 0x00,
	A_Coarse = 0x01,
	B_Fine   = 0x02,
	B_Coarse = 0x03,
	C_Fine   = 0x04,
	C_Coarse = 0x05,
	N_Period = 0x06,
	Mixer    = 0x07,
	A_Volume = 0x08,
	B_Volume = 0x09,
	C_Volume = 0x0A,
	E_Fine   = 0x0B,
	E_Coarse = 0x0C,
	E_Shape  = 0x0D,
	PortA    = 0x0E,
	PortB    = 0x0F,

	A_Period = A_Fine,
	B_Period = B_Fine,
	C_Period = C_Fine,
	E_Period = E_Fine,
};

class Frame
{
public:
	Frame() = default;
	Frame(const Frame& other);

public:
	uint8_t  Read(uint8_t chip, uint8_t reg) const;
	bool     IsChanged(uint8_t chip, uint8_t reg) const;

	uint16_t ReadPeriod(uint8_t chip, uint8_t period) const;
	bool     IsChangedPeriod(uint8_t chip, uint8_t period) const;

	uint8_t  Read(uint8_t reg) const;
	bool     IsChanged(uint8_t reg) const;

	uint16_t ReadPeriod(uint8_t period) const;
	bool     IsChangedPeriod(uint8_t period) const;

public:
	void Write(uint8_t chip, uint8_t reg, uint8_t data);
	void Update(uint8_t chip, uint8_t reg, uint8_t data);

	void WritePeriod(uint8_t chip, uint8_t period, uint16_t data);
	void UpdatePeriod(uint8_t chip, uint8_t period, uint16_t data);

	void Write(uint8_t reg, uint8_t data);
	void Update(uint8_t reg, uint8_t data);

	void WritePeriod(uint8_t period, uint16_t data);
	void UpdatePeriod(uint8_t period, uint16_t data);

public:
	void Reset();
	void ResetData();
	void ResetChanges();
	
public:
	uint8_t& data(uint8_t chip, uint8_t reg);
	bool& changed(uint8_t chip, uint8_t reg);

private:
	uint8_t m_data[2][16]{};
	bool m_changes[2][16]{};
};
