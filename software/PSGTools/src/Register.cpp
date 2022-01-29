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

Register::Register(const Register& other)
	: m_data(other.m_data)
	, m_changed(other.m_changed)
{
}

Register::operator bool() const
{
	return m_changed;
}

Register::operator uint8_t() const
{
	return m_data;
}

Register& Register::operator=(uint8_t data)
{
	m_data = data;
	m_changed = true;
	return *this;
}
