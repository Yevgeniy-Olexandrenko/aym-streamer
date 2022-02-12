#pragma once

#include <stdint.h>

class Register
{
public:
	Register();
	Register(uint8_t data);
	Register& operator=(uint8_t data) = delete;

public:
	bool changed() const;
	uint8_t data() const;

	void override(uint8_t data);
	void update(uint8_t data);

private:
	uint8_t m_data;
	bool m_changed;
};