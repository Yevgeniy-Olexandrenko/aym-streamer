#include "Frame.h"
#include <cstring>
#include <iomanip>

namespace
{
	const Register k_tFine[]   = { A_Fine,    B_Fine,    C_Fine    };
	const Register k_tCoarse[] = { A_Coarse,  B_Coarse,  C_Coarse  };
	const Register k_tDuty[]   = { A_Duty,    B_Duty,    C_Duty    };
	const Register k_volume[]  = { A_Volume,  B_Volume,  C_Volume  };
	const Register k_eFine[]   = { EA_Fine,   EB_Fine,   EC_Fine   };
	const Register k_eCoarse[] = { EA_Coarse, EB_Coarse, EC_Coarse };
	const Register k_eShape[]  = { EA_Shape,  EB_Shape,  EC_Shape  };

	struct RegDefine
	{
		uint8_t comIndex; // 0xFF -> unknown register, b7 = 1 -> envelope shape register
		uint8_t comMask;  // mask for register in compatibility mode
		uint8_t expIndex; // 0xFF -> unknown register, b7 = 1 -> envelope shape register
		uint8_t expMask;  // mask for register in expanded mode
	};

	const RegDefine k_regDefines[] =
	{
		// bank A
		{ 0x00, 0xFF, 0x00, 0xFF },
		{ 0x01, 0x0F, 0x01, 0xFF },
		{ 0x02, 0xFF, 0x02, 0xFF },
		{ 0x03, 0x0F, 0x03, 0xFF },
		{ 0x04, 0xFF, 0x04, 0xFF },
		{ 0x05, 0x0F, 0x05, 0xFF },
		{ 0x06, 0x1F, 0x06, 0xFF },
		{ 0x07, 0x3F, 0x07, 0x3F },
		{ 0x08, 0x1F, 0x08, 0x3F },
		{ 0x09, 0x1F, 0x09, 0x3F },
		{ 0x0A, 0x1F, 0x0A, 0x3F },
		{ 0x0B, 0xFF, 0x0B, 0xFF },
		{ 0x0C, 0xFF, 0x0C, 0xFF },
		{ 0x8D, 0xEF, 0x8D, 0xEF },
		{ 0xFF, 0x00, 0xFF, 0x00 },
		{ 0xFF, 0x00, 0xFF, 0x00 },

		// bank B
		{ 0xFF, 0x00, 0x0E, 0xFF },
		{ 0xFF, 0x00, 0x0F, 0xFF },
		{ 0xFF, 0x00, 0x10, 0xFF },
		{ 0xFF, 0x00, 0x11, 0xFF },
		{ 0xFF, 0x00, 0x92, 0x0F },
		{ 0xFF, 0x00, 0x93, 0x0F },
		{ 0xFF, 0x00, 0x14, 0x0F },
		{ 0xFF, 0x00, 0x15, 0x0F },
		{ 0xFF, 0x00, 0x16, 0x0F },
		{ 0xFF, 0x00, 0x17, 0xFF },
		{ 0xFF, 0x00, 0x18, 0xFF },
		{ 0xFF, 0x00, 0xFF, 0x00 },
		{ 0xFF, 0x00, 0xFF, 0x00 },
		{ 0xFF, 0x00, 0x8D, 0xEF },
		{ 0xFF, 0x00, 0xFF, 0x00 },
		{ 0xFF, 0x00, 0xFF, 0x00 }
	};
}

struct Frame::RegInfo
{
	uint8_t flags;
	uint8_t index;
	uint8_t mask;
};

bool Frame::GetRegInfo(int chip, Register reg, RegInfo& info) const
{
	if (chip < 2 && reg < 32)
	{
		uint8_t index = (IsExpMode(chip)
			? k_regDefines[reg].expIndex
			: k_regDefines[reg].comIndex);

		if (index != 0xFF)
		{
			info.flags = (index & 0xE0);
			info.index = (index & 0x1F);
			info.mask  = (IsExpMode(chip)
				? k_regDefines[reg].expMask
				: k_regDefines[reg].comMask);
			return true;
		}
	}
	return false;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Frame::Frame()
	: m_id(0)
{
	ResetData();
	ResetChanges();
}

Frame::Frame(const Frame& other)
	: m_id(other.m_id)
{
	memcpy(m_data, other.m_data, sizeof(m_data));
	memcpy(m_changes, other.m_changes, sizeof(m_changes));
}

void Frame::SetId(FrameId id)
{
	m_id = id;
}

FrameId Frame::GetId() const
{
	return m_id;
}

Frame& Frame::operator!()
{
	ResetChanges(true);
	return *this;
}

Frame& Frame::operator+=(const Frame& other)
{
	for (int chip = 0; chip < 2; ++chip)
	{
		if (IsExpMode(chip) != other.IsExpMode(chip))
		{
			SetExpMode(chip, other.IsExpMode(chip));
		}

		for (Register reg = BankA_Fst; reg <= BankB_Lst; ++reg)
		{
			Update(chip, reg, other.Read(chip, reg));
		}
	}
	return *this;
}

void Frame::ResetData()
{
	memset(m_data, 0x00, sizeof(m_data));
}

void Frame::ResetChanges(bool val)
{
	memset(m_changes, val, sizeof(m_changes));
}

bool Frame::HasChanges() const
{
	for (int chip = 0; chip < 2; ++chip)
	{
		for (Register reg = 0; reg < 32; ++reg)
		{
			if (IsChanged(chip, reg)) return true;
		}
	}
	return false;
}

bool Frame::IsExpMode(int chip) const
{
	return ((m_data[chip][k_modeBankRegIdx] & 0xE0) == 0xA0);
}

void Frame::SetExpMode(int chip, bool yes)
{
	uint8_t data = (m_data[chip][k_modeBankRegIdx] & 0x0F) | (yes ? 0xA0 : 0x00);
	Update(chip, Mode_Bank, data);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

uint8_t Frame::Read(int chip, Register reg) const
{
	RegInfo info;
	if (GetRegInfo(chip, reg, info))
	{
		if ((info.flags & 0x80) && !m_changes[chip][info.index])
		{
			return k_unchangedShape;
		}
		return m_data[chip][info.index];
	}
	return 0x00;
}

bool Frame::IsChanged(int chip, Register reg) const
{
	RegInfo info;
	return (GetRegInfo(chip, reg, info) ? m_changes[chip][info.index] : false);
}

uint16_t Frame::ReadPeriod(int chip, PeriodRegister preg) const
{
	uint16_t data = 0;
	switch (preg)
	{
	case A_Period:
	case B_Period:
	case C_Period:
	case EA_Period:
	case EB_Period:
	case EC_Period:
		data |= Read(chip, preg + 1) << 8;
	case N_Period:
		data |= Read(chip, preg + 0);
	}
	return data;
}

bool Frame::IsChangedPeriod(int chip, PeriodRegister preg) const
{
	bool changed = false;
	switch (preg)
	{
	case A_Period:
	case B_Period:
	case C_Period:
	case EA_Period:
	case EB_Period:
	case EC_Period:
		changed |= IsChanged(chip, preg + 1);
	case N_Period:
		changed |= IsChanged(chip, preg + 0);
	}
	return changed;
}

uint8_t Frame::Read(Register reg) const
{
	return Read(0, reg);
}

bool Frame::IsChanged(Register reg) const
{
	return IsChanged(0, reg);
}

uint16_t Frame::ReadPeriod(PeriodRegister preg) const
{
	return ReadPeriod(0, preg);
}

bool Frame::IsChangedPeriod(PeriodRegister preg) const
{
	return IsChangedPeriod(0, preg);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void Frame::Update(int chip, Register reg, uint8_t data)
{
	RegInfo info;
	if (GetRegInfo(chip, reg, info))
	{
		// special case for the envelope shape register
		if ((info.flags & 0x80))
		{
			// the envelope shape register can be overwritten with
			// the same value, which resets the envelope generator
			if (data != k_unchangedShape)
			{
				data &= info.mask;
				if (info.index == k_modeBankRegIdx)
				{
					// check if mode changed, reset registers
					if ((m_data[chip][info.index] ^ data) & 0xE0)
					{
						ResetData();
						ResetChanges(true);
					}
				}

				// write new value in register
				m_data[chip][info.index] = data;
				m_changes[chip][info.index] = true;
			}
		}
		else
		{
			// as for the remaining registers, only updating their
			// values ​​has an effect on the sound generation
			data &= info.mask;
			if (m_data[chip][info.index] != data)
			{
				m_data[chip][info.index] = data;
				m_changes[chip][info.index] = true;
			}
		}
	}
}

void Frame::UpdatePeriod(int chip, PeriodRegister preg, uint16_t data)
{
	switch (preg)
	{
	case A_Period:
	case B_Period:
	case C_Period:
	case EA_Period:
	case EB_Period:
	case EC_Period:
		Update(chip, preg + 1, data >> 8);
	case N_Period:
		Update(chip, preg + 0, uint8_t(data));
	}
}

void Frame::Update(Register reg, uint8_t data)
{
	Update(0, reg, data);
}

void Frame::UpdatePeriod(PeriodRegister preg, uint16_t data)
{
	UpdatePeriod(0, preg, data);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Frame::Channel Frame::ReadChannel(int chip, int chan) const
{
	Channel data{};
	if (chan >= 0 && chan <= 2)
	{
		bool isExpMode = IsExpMode(chip);
		
		data.tFine   = Read(chip, k_tFine[chan]);
		data.tCoarse = Read(chip, k_tCoarse[chan]);
		data.tDuty   = Read(chip, k_tDuty[chan]);
		data.mixer   = Read(chip, Mixer) >> chan & 0x09;
		data.volume  = Read(chip, k_volume[chan]);
		data.eFine   = Read(chip, isExpMode ? k_eFine[chan] : E_Fine);
		data.eCoarse = Read(chip, isExpMode ? k_eFine[chan] : E_Coarse);
		data.eShape  = Read(chip, isExpMode ? k_eFine[chan] : E_Shape);

		if (data.eShape != k_unchangedShape && isExpMode)
		{
			data.eShape &= 0x0F;
			data.eShape |= 0xA0;
		}
	}
	return data;
}

void Frame::UpdateChannel(int chip, int chan, const Channel& data)
{
	if (chan >= 0 && chan <= 2)
	{
		bool isExpMode = IsExpMode(chip);
		auto mixerData = Read(chip, Mixer) & ~(0x09 << chan);

		Update(chip, k_tFine[chan], data.tFine);
		Update(chip, k_tCoarse[chan], data.tCoarse);
		Update(chip, k_tDuty[chan], data.tDuty);
		Update(chip, Mixer, mixerData | data.mixer << chan);
		Update(chip, k_volume[chan], data.volume);
		Update(chip, k_eFine[chan], data.eFine);
		Update(chip, k_eCoarse[chan], data.eCoarse);
		Update(chip, k_eShape[chan], data.eShape);
	}
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

const uint8_t& Frame::data(int chip, Register reg) const
{
	RegInfo info;
	static uint8_t dummy = 0;
	return (GetRegInfo(chip, reg, info) ? m_data[chip][info.index] : dummy);
}

uint8_t& Frame::data(int chip, Register reg)
{
	RegInfo info;
	static uint8_t dummy = 0;
	return (GetRegInfo(chip, reg, info) ? m_data[chip][info.index] : dummy);
}

bool& Frame::changed(int chip, Register reg)
{
	RegInfo info;
	static bool dummy = 0;
	return (GetRegInfo(chip, reg, info) ? m_changes[chip][info.index] : dummy);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

std::ostream& operator<<(std::ostream& os, const Frame& frame)
{
	Frame::RegInfo info;
	for (int chip = 0; chip < 2; ++chip)
	{
		for (Register reg = BankA_Fst; reg <= BankB_Lst; ++reg)
		{
			if (frame.GetRegInfo(chip, reg, info))
			{
				if (frame.m_changes[chip][info.index])
				{
					os << std::hex << std::setw(2) << std::setfill('0');
					os << int(reg);
					os << ':';

					uint8_t data = frame.m_data[chip][info.index];
					os << std::hex << std::setw(2) << std::setfill('0');
					os << int(data);
					os << ' ';
				}
				else
				{
					os << "--:-- ";
				}
			}
		}
		os << (chip ? "\n" : "| ");
	}
	return os;
}
