#include "Frame.h"
#include <cstring>

Frame::RegDefine Frame::s_regDefines[] =
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

bool Frame::GetRegInfo(int chip, Register reg, RegInfo& regInfo) const
{
	if (chip < 2 && reg < 32)
	{
		uint8_t index = (IsExpMode(chip)
			? s_regDefines[reg].expIndex
			: s_regDefines[reg].comIndex);

		if (index != 0xFF)
		{
			regInfo.flags = (index & 0xE0);
			regInfo.index = (index & 0x1F);
			regInfo.mask = (IsExpMode(chip)
				? s_regDefines[reg].expMask
				: s_regDefines[reg].comMask);
			return true;
		}
	}
	return false;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Frame::Frame(const Frame& other)
{
	memcpy(m_data, other.m_data, sizeof(m_data));
	memcpy(m_changes, other.m_changes, sizeof(m_changes));
}

void Frame::Reset()
{
	ResetData();
	ResetChanges();
}

void Frame::ResetData()
{
	memset(m_data, 0x00, sizeof(m_data));
}

void Frame::ResetChanges()
{
	memset(m_changes, false, sizeof(m_changes));
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
	return ((m_data[chip][0x0D] & 0xE0) == 0xA0);
}

bool Frame::HasChangesInBank(int chip, int bank) const
{
	if (IsExpMode(chip) || bank == 0)
	{
		for (Register reg = 0; reg < 16; ++reg)
		{
			if (IsChanged(chip, 16 * bank + reg)) return true;
		}
	}
	return false;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

uint8_t Frame::Read(int chip, Register reg) const
{
	RegInfo info;
	return (GetRegInfo(chip, reg, info) ? m_data[chip][info.index] : 0x00);
}

bool Frame::IsChanged(int chip, Register reg) const
{
	RegInfo info;
	return (GetRegInfo(chip, reg, info) ? m_changes[chip][info.index] : false);
}

uint16_t Frame::ReadPeriod(int chip, PeriodRegister preg) const
{
	if (preg == A_Period || preg == B_Period || preg == C_Period || preg == E_Period)
	{
		return (Read(chip, preg + 0) | Read(chip, preg + 1) << 8);
	}
	else if (preg == N_Period)
	{
		return Read(chip, preg);
	}
	return 0;
}

bool Frame::IsChangedPeriod(int chip, PeriodRegister preg) const
{
	if (preg == A_Period || preg == B_Period || preg == C_Period || preg == E_Period)
	{
		return (IsChanged(chip, preg + 0) || IsChanged(chip, preg + 1));
	}
	else if (preg == N_Period)
	{
		return IsChanged(chip, preg);
	}
	return false;
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

void Frame::Write(int chip, Register reg, uint8_t data)
{
	RegInfo info;
	if (GetRegInfo(chip, reg, info))
	{
		// special case for the envelope shape register
		if ((info.flags & 0x80) && data == UnchangedShape) return;

		// check if mode changed, reset registers
		data &= info.mask;
		if (info.index = 0x0D)
		{
			if ((m_data[chip][info.index] ^ data) & 0xE0)
			{
				ResetData();
				memset(m_changes, true, sizeof(m_changes));
			}
		}
		
		// write new value in register
		m_data[chip][info.index] = data;
		m_changes[chip][info.index] = true;
	}
}

void Frame::Update(int chip, Register reg, uint8_t data)
{
	RegInfo info;
	if (GetRegInfo(chip, reg, info))
	{
		if ((info.flags & 0x80))
		{
			// the envelope shape register can be overwritten with
			// the same value, which resets the envelope generator
			Write(chip, reg, data);
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

void Frame::WritePeriod(int chip, PeriodRegister preg, uint16_t data)
{
	if (preg == A_Period || preg == B_Period || preg == C_Period || preg == E_Period)
	{
		Write(chip, preg + 0, uint8_t(data));
		Write(chip, preg + 1, data >> 8);
	}
	else if (preg == N_Period)
	{
		Write(chip, preg, uint8_t(data));
	}
}

void Frame::UpdatePeriod(int chip, PeriodRegister preg, uint16_t data)
{
	if (preg == A_Period || preg == B_Period || preg == C_Period || preg == E_Period)
	{
		Update(chip, preg + 0, uint8_t(data));
		Update(chip, preg + 1, data >> 8);
	}
	else if (preg == N_Period)
	{
		Update(chip, preg, uint8_t(data));
	}
}

void Frame::Write(Register reg, uint8_t data)
{
	Write(0, reg, data);
}

void Frame::Update(Register reg, uint8_t data)
{
	Update(0, reg, data);
}

void Frame::WritePeriod(PeriodRegister preg, uint16_t data)
{
	WritePeriod(0, preg, data);
}

void Frame::UpdatePeriod(PeriodRegister preg, uint16_t data)
{
	UpdatePeriod(0, preg, data);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

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
