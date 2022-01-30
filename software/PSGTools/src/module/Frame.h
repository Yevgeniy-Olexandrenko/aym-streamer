#pragma once

#include <array>
#include <iostream>
#include "Register.h"

class Frame
{
	using RegisterArray = std::array<Register, size_t(Register::Index::COUNT)>;
	friend std::ostream& operator<<(std::ostream& stream, const Frame& frame);

public:
	Frame();
	Frame(const Frame& other);

public:
	operator bool() const; // has changed registers
	Register& operator[](size_t index); // access register
	Register& operator[](Register::Index index); // access register
	const Register& operator[](size_t index) const; // access register
	const Register& operator[](Register::Index index) const; // access register

	void MarkChanged(bool isChanged);
	void FixValues();

private:
	RegisterArray m_registers;
};
