#include "Register.h"

Register::Register()
	: Register(0x00)
{
}

Register::Register(uint8_t data)
	: m_data(data)
	, m_changed(false)
{
}

bool Register::changed() const
{
	return m_changed;
}

uint8_t Register::data() const
{
	return m_data;
}

void Register::override(uint8_t data)
{
	m_changed = true;
	m_data = data;
}

void Register::update(uint8_t data)
{
	if (data != m_data) override(data);
}
