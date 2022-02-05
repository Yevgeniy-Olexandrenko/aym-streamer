#pragma once

#include <stdint.h>

class Register
{
public:
	Register();
	Register(uint8_t data);
	Register& operator=(uint8_t data) = delete;

public:
	bool IsChanged() const;
	uint8_t GetData() const;

	void OverrideData(uint8_t data);
	void UpdateData(uint8_t data);

private:
	uint8_t m_data;
	bool m_changed;
};