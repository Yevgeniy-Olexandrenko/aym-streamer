#pragma once

#include <cstdint>
#include <iostream>

class BitStream
{
public:
	bool Open(std::ostream& stream);
	bool Open(std::istream& stream);

	std::ostream* GetOStream();
	std::istream* GetIStream();

	bool Write(uint32_t data, unsigned size);
	template <unsigned size> bool Write(uint32_t data);

	bool Read(uint32_t& data, unsigned size);
	template <unsigned size> bool Read(uint32_t& data);

	bool Align();
	bool Close();

private:
	std::ostream* m_ostream = nullptr;
	std::istream* m_istream = nullptr;

	uint8_t m_buffer;
	uint8_t m_count;
};

template<unsigned size>
inline bool BitStream::Write(uint32_t data)
{
	return Write(data, size);
}

template<unsigned size>
inline bool BitStream::Read(uint32_t& data)
{
	return Read(data, size);
}
