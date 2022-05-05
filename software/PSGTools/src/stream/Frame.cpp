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

uint8_t Frame::Read(uint8_t reg) const
{
	return Read(0, reg);
}

bool Frame::IsChanged(uint8_t reg) const
{
	return IsChanged(0, reg);
}

void Frame::Write(uint8_t chip, uint8_t reg, uint8_t data)
{
	if (reg == Env_Shape && data == 0xFF) return;

	data &= mask[reg];
	m_data[chip][reg] = data;
	m_changes[chip][reg] = true;
}

void Frame::Update(uint8_t chip, uint8_t reg, uint8_t data)
{
	if (reg == Env_Shape)
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

void Frame::Write(uint8_t reg, uint8_t data)
{
	Write(0, reg, data);
}

void Frame::Update(uint8_t reg, uint8_t data)
{
	Update(0, reg, data);
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
