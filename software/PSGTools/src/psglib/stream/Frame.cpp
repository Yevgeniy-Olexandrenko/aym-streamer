#include "Frame.h"
#include <cstring>
#include <iomanip>

namespace
{
	const Register c_tFine[]   = { A_Fine,    B_Fine,    C_Fine    };
	const Register c_tCoarse[] = { A_Coarse,  B_Coarse,  C_Coarse  };
	const Register c_tDuty[]   = { A_Duty,    B_Duty,    C_Duty    };
	const Register c_volume[]  = { A_Volume,  B_Volume,  C_Volume  };
	const Register c_eFine[]   = { EA_Fine,   EB_Fine,   EC_Fine   };
	const Register c_eCoarse[] = { EA_Coarse, EB_Coarse, EC_Coarse };
	const Register c_eShape[]  = { EA_Shape,  EB_Shape,  EC_Shape  };

	struct RegDefine
	{
		uint8_t comIndex; // 0xFF -> unknown register, b7 = 1 -> envelope shape register
		uint8_t comMask;  // mask for register in compatibility mode
		uint8_t expIndex; // 0xFF -> unknown register, b7 = 1 -> envelope shape register
		uint8_t expMask;  // mask for register in expanded mode
	};

	const RegDefine c_regDefines[] =
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

Frame::Frame()
	: m_id(0)
{
	ResetData();
	ResetChanges();
}

Frame::Frame(const Frame& other)
	: m_id(other.m_id)
{
	for (int chip = 0; chip < 2; ++chip)
	{
		m_regs[chip] = other.m_regs[chip];
	}
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
		if (m_regs[chip].IsExpMode() != other.m_regs[chip].IsExpMode())
		{
			m_regs[chip].SetExpMode(other.m_regs[chip].IsExpMode());
		}

		for (Register reg = BankA_Fst; reg <= BankB_Lst; ++reg)
		{
			m_regs[chip].Update(reg, other.m_regs[chip].Read(reg));
		}
	}
	return *this;
}

void Frame::ResetData()
{
	for (int chip = 0; chip < 2; ++chip)
	{
		m_regs[chip].ResetData();
	}
}

void Frame::ResetChanges(bool val)
{
	for (int chip = 0; chip < 2; ++chip)
	{
		m_regs[chip].ResetChanges(val);
	}
}

bool Frame::HasChanges() const
{
	for (int chip = 0; chip < 2; ++chip)
	{
		if (m_regs[chip].HasChanges()) return true;
	}
	return false;
}

struct Frame::Registers::Info
{
	uint8_t flags;
	uint8_t index;
	uint8_t mask;
};

bool Frame::Registers::GetInfo(Register reg, Info& info) const
{
	if (reg < 32)
	{
		uint8_t index = (IsExpMode()
			? c_regDefines[reg].expIndex
			: c_regDefines[reg].comIndex);

		if (index != 0xFF)
		{
			info.flags = (index & 0xE0);
			info.index = (index & 0x1F);
			info.mask  = (IsExpMode()
				? c_regDefines[reg].expMask
				: c_regDefines[reg].comMask);
			return true;
		}
	}
	return false;
}

void Frame::Registers::ResetData()
{
	memset(m_data, 0x00, sizeof(m_data));
}

void Frame::Registers::ResetChanges(bool val)
{
	memset(m_changes, val, sizeof(m_changes));
}

bool Frame::Registers::HasChanges() const
{
	for (Register reg = BankA_Fst; reg <= BankB_Lst; ++reg)
	{
		if (IsChanged(reg)) return true;
	}
	return false;
}

bool Frame::Registers::IsExpMode() const
{
	return ((m_data[c_modeBankRegIdx] & 0xE0) == 0xA0);
}

void Frame::Registers::SetExpMode(bool yes)
{
	uint8_t data = (m_data[c_modeBankRegIdx] & 0x0F) | (yes ? 0xA0 : 0x00);
	Update(Mode_Bank, data);
}

uint8_t Frame::Registers::Read(Register reg) const
{
	Info info;
	if (GetInfo(reg, info))
	{
		if ((info.flags & 0x80) && !m_changes[info.index])
		{
			return c_unchangedShape;
		}
		return m_data[info.index];
	}
	return 0x00;
}

bool Frame::Registers::IsChanged(Register reg) const
{
	Info info;
	return (GetInfo(reg, info) ? m_changes[info.index] : false);
}

uint16_t Frame::Registers::ReadPeriod(PeriodRegister preg) const
{
	uint16_t data = 0;
	switch (preg)
	{
	case A_Period : case B_Period : case C_Period :
	case EA_Period: case EB_Period: case EC_Period:
		data |= Read(preg + 1) << 8;
	case N_Period:
		data |= Read(preg + 0);
	}
	return data;
}

bool Frame::Registers::IsChangedPeriod(PeriodRegister preg) const
{
	bool changed = false;
	switch (preg)
	{
	case A_Period : case B_Period : case C_Period :
	case EA_Period: case EB_Period: case EC_Period:
		changed |= IsChanged(preg + 1);
	case N_Period:
		changed |= IsChanged(preg + 0);
	}
	return changed;
}

void Frame::Registers::Update(Register reg, uint8_t data)
{
	Info info;
	if (GetInfo(reg, info))
	{
		// special case for the envelope shape register
		if ((info.flags & 0x80))
		{
			// the envelope shape register can be overwritten with
			// the same value, which resets the envelope generator
			if (data != c_unchangedShape)
			{
				data &= info.mask;
				if (info.index == c_modeBankRegIdx)
				{
					// check if mode changed, reset registers
					if ((m_data[info.index] ^ data) & 0xE0)
					{
						ResetData();
						ResetChanges(true);
					}
				}

				// write new value in register
				m_data[info.index] = data;
				m_changes[info.index] = true;
			}
		}
		else
		{
			// as for the remaining registers, only updating their
			// values ​​has an effect on the sound generation
			data &= info.mask;
			if (m_data[info.index] != data)
			{
				m_data[info.index] = data;
				m_changes[info.index] = true;
			}
		}
	}
}

void Frame::Registers::UpdatePeriod(PeriodRegister preg, uint16_t data)
{
	switch (preg)
	{
	case A_Period : case B_Period : case C_Period :
	case EA_Period: case EB_Period: case EC_Period:
		Update(preg + 1, data >> 8);
	case N_Period:
		Update(preg + 0, uint8_t(data));
	}
}

uint8_t Frame::Registers::GetData(Register reg) const
{
	Info info;
	return (GetInfo(reg, info) ? m_data[info.index] : 0x00);
}

Frame::Channel Frame::Registers::ReadChannel(int chan) const
{
		Channel data{};
		if (chan >= 0 && chan <= 2)
		{
			bool isExpMode = IsExpMode();
			
			data.tFine = Read(c_tFine[chan]);
			data.tCoarse = Read(c_tCoarse[chan]);
			data.tDuty = Read(c_tDuty[chan]);
			data.mixer = Read(Mixer) >> chan & 0x09;
			data.volume = Read(c_volume[chan]);
			data.eFine = Read(isExpMode ? c_eFine[chan] : E_Fine);
			data.eCoarse = Read(isExpMode ? c_eCoarse[chan] : E_Coarse);
			data.eShape = Read(isExpMode ? c_eShape[chan] : E_Shape);
	
			if (data.eShape != c_unchangedShape && isExpMode)
			{
				data.eShape &= 0x0F;
				data.eShape |= 0xA0;
			}
		}
		return data;
}

void Frame::Registers::UpdateChannel(int chan, const Channel& data)
{
	if (chan >= 0 && chan <= 2)
	{
		bool isExpMode = IsExpMode();
		auto mixerData = Read(Mixer) & ~(0x09 << chan);

		Update(c_tFine[chan], data.tFine);
		Update(c_tCoarse[chan], data.tCoarse);
		Update(c_tDuty[chan], data.tDuty);
		Update(Mixer, mixerData | data.mixer << chan);
		Update(c_volume[chan], data.volume);
		Update(c_eFine[chan], data.eFine);
		Update(c_eCoarse[chan], data.eCoarse);
		Update(c_eShape[chan], data.eShape);
	}
}

const Frame::Registers& Frame::operator[](int chip) const
{
	return m_regs[bool(chip)];
}

Frame::Registers& Frame::operator[](int chip)
{
	return m_regs[bool(chip)];
}
