#include "Frame.h"
#include <cstring>

namespace
{
	uint8_t mask[]
	{
		0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x0F, 0x1F, 0x3F,
		0x1F, 0x1F, 0x1F, 0xFF, 0xFF, 0x0F, 0x00, 0x00
	};
}

Frame::Frame(const Frame& other)
{
	memcpy(m_data, other.m_data, sizeof(m_data));
	memcpy(m_changes, other.m_changes, sizeof(m_changes));
}

uint8_t Frame::Read(uint8_t chip, uint8_t reg) const
{
	return m_data[chip][reg];
}

bool Frame::IsChanged(uint8_t chip, uint8_t reg) const
{
	return m_changes[chip][reg];
}

uint16_t Frame::ReadPeriod(uint8_t chip, uint8_t period) const
{
	if (period == A_Period || period == B_Period || period == C_Period || period == E_Period)
	{
		return (Read(chip, period + 0) | Read(chip, period + 1) << 8);
	}
	else if (period == N_Period)
	{
		return Read(chip, period);
	}
	return 0;
}

bool Frame::IsChangedPeriod(uint8_t chip, uint8_t period) const
{
	if (period == A_Period || period == B_Period || period == C_Period || period == E_Period)
	{
		return (IsChanged(chip, period + 0) || IsChanged(chip, period + 1));
	}
	else if (period == N_Period)
	{
		return IsChanged(chip, period);
	}
	return false;
}

uint8_t Frame::Read(uint8_t reg) const
{
	return Read(0, reg);
}

bool Frame::IsChanged(uint8_t reg) const
{
	return IsChanged(0, reg);
}

uint16_t Frame::ReadPeriod(uint8_t period) const
{
	return ReadPeriod(0, period);
}

bool Frame::IsChangedPeriod(uint8_t period) const
{
	return IsChangedPeriod(0, period);
}

void Frame::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
	if (reg == E_Shape && data == 0xFF) return;

	data &= mask[reg];
	m_data[chip][reg] = data;
	m_changes[chip][reg] = true;
}

void Frame::Update(uint8_t chip, uint8_t reg, uint8_t data)
{
	if (reg == E_Shape)
	{
		Write(chip, reg, data);
	}
	else
	{
		data &= mask[reg];
		if (Read(chip, reg) != data)
		{
			m_data[chip][reg] = data;
			m_changes[chip][reg] = true;
		}
	}
}

void Frame::WritePeriod(uint8_t chip, uint8_t period, uint16_t data)
{
	if (period == A_Period || period == B_Period || period == C_Period || period == E_Period)
	{
		Write(chip, period + 0, uint8_t(data));
		Write(chip, period + 1, data >> 8);
	}
	else if (period == N_Period)
	{
		Write(chip, period, uint8_t(data));
	}
}

void Frame::UpdatePeriod(uint8_t chip, uint8_t period, uint16_t data)
{
	if (period == A_Period || period == B_Period || period == C_Period || period == E_Period)
	{
		Update(chip, period + 0, uint8_t(data));
		Update(chip, period + 1, data >> 8);
	}
	else if (period == N_Period)
	{
		Update(chip, period, uint8_t(data));
	}
}

void Frame::Write(uint8_t reg, uint8_t data)
{
	Write(0, reg, data);
}

void Frame::Update(uint8_t reg, uint8_t data)
{
	Update(0, reg, data);
}

void Frame::WritePeriod(uint8_t period, uint16_t data)
{
	WritePeriod(0, period, data);
}

void Frame::UpdatePeriod(uint8_t period, uint16_t data)
{
	UpdatePeriod(0, period, data);
}

void Frame::Reset()
{
	ResetData();
	ResetChanges();
}

void Frame::ResetData()
{
	memset(m_data, 0, sizeof(m_data));
}

void Frame::ResetChanges()
{
	memset(m_changes, false, sizeof(m_changes));
}

uint8_t& Frame::data(uint8_t chip, uint8_t reg)
{
	return m_data[chip][reg];
}

bool& Frame::changed(uint8_t chip, uint8_t reg)
{
	return m_changes[chip][reg];
}
