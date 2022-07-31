#pragma once

#include <stdint.h>
#include <iostream>

enum
{
	// bank A
	A_Fine    = 0x00, // 00
	A_Coarse  = 0x01, // 01
	B_Fine    = 0x02, // 02
	B_Coarse  = 0x03, // 03
	C_Fine    = 0x04, // 04
	C_Coarse  = 0x05, // 05
	N_Period  = 0x06, // 06
	Mixer     = 0x07, // 07
	A_Volume  = 0x08, // 08
	B_Volume  = 0x09, // 09
	C_Volume  = 0x0A, // 10
	EA_Fine   = 0x0B, // 11
	EA_Coarse = 0x0C, // 12
	EA_Shape  = 0x0D, // 13

	// bank B
	EB_Fine   = 0x10, // 14
	EB_Coarse = 0x11, // 15
	EC_Fine   = 0x12, // 16
	EC_Coarse = 0x13, // 17
	EB_Shape  = 0x14, // 18
	EC_Shape  = 0x15, // 19
	A_Duty    = 0x16, // 20
	B_Duty    = 0x17, // 21
	C_Duty    = 0x18, // 22
	N_AndMask = 0x19, // 23
	N_OrMask  = 0x1A, // 24

	// aliases
	E_Fine    = EA_Fine,
	E_Coarse  = EA_Coarse,
	E_Shape   = EA_Shape,
	Mode_Bank = EA_Shape,

	A_Period  = A_Fine,
	B_Period  = B_Fine,
	C_Period  = C_Fine,
	E_Period  = E_Fine,
	EA_Period = EA_Fine,
	EB_Period = EB_Fine,
	EC_Period = EC_Fine,

	BankA_Fst = 0x00,
	BankA_Lst = 0x0D,
	BankB_Fst = 0x10,
	BankB_Lst = 0x1D,
};

using FrameId = uint32_t;
using Register = uint8_t;
using PeriodRegister = uint8_t;

constexpr uint8_t k_modeBankRegIdx = 0x0D;
constexpr uint8_t k_unchangedShape = 0xFF;

class Frame
{
	struct RegInfo;
	bool GetRegInfo(int chip, Register reg, RegInfo& info) const;

public:
	Frame();
	Frame(const Frame& other);

	void SetId(FrameId id);
	FrameId GetId() const;

	Frame& operator!();
	Frame& operator+=(const Frame& other);

	void ResetData();
	void ResetChanges(bool val = false);
	bool HasChanges() const;

	bool IsExpMode(int chip) const;
	void SetExpMode(int chip, bool yes);

public:
	uint8_t  Read(int chip, Register reg) const;
	bool     IsChanged(int chip, Register reg) const;

	uint16_t ReadPeriod(int chip, PeriodRegister preg) const;
	bool     IsChangedPeriod(int chip, PeriodRegister preg) const;

	uint8_t  Read(Register reg) const;
	bool     IsChanged(Register reg) const;

	uint16_t ReadPeriod(PeriodRegister preg) const;
	bool     IsChangedPeriod(PeriodRegister preg) const;

public:
	void Update(int chip, Register reg, uint8_t data);
	void UpdatePeriod(int chip, PeriodRegister preg, uint16_t data);

	void Update(Register reg, uint8_t data);
	void UpdatePeriod(PeriodRegister preg, uint16_t data);

public:
	struct Channel
	{
		uint8_t tFine;
		uint8_t	tCoarse;
		uint8_t tDuty;   // doesn't matter in comp mode
		uint8_t	mixer;   // taken chan Tone & Noise bits
		uint8_t volume;
		uint8_t	eFine;   // taken from chan A in comp mode
		uint8_t	eCoarse; // taken from chan A in comp mode
		uint8_t	eShape;  // taken from chan A in comp mode + exp mode flags
	};

	Channel ReadChannel(int chip, int chan) const;
	void UpdateChannel(int chip, int chan, const Channel& data);
	
public:
	const uint8_t& data(int chip, Register reg) const;
	uint8_t& data(int chip, Register reg);
	bool& changed(int chip, Register reg);

private:
	FrameId m_id;
	uint8_t m_data[2][25];
	bool m_changes[2][25];

public:
	friend std::ostream& operator<<(std::ostream& os, const Frame& frame);
};
