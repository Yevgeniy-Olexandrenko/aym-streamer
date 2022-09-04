#pragma once

#include <map>
#include <vector>
#include "encoders/Encoder.h"

using CharType = uint16_t;
using CodeType = uint32_t;

const uint8_t CharTypeSize = 16;
const uint8_t CodeTypeSize = 16;

class EncodeTST : public Encoder
{
public:
	bool Open(const Stream& stream) override;
	void Encode(const Frame& frame) override;
	void Close(const Stream& stream) override;

private:
	void reset_dictionary();
	void compress_char();
	void compress_last();

private:
	class BitStream
	{
	public:
		void Open(std::ostream& stream);
		void Write(uint32_t data, uint8_t size);
		void Close();

	private:
		std::ostream* m_stream;
		uint8_t m_buffer;
		uint8_t m_count;
	};

private:
	std::ofstream m_output;
	BitStream m_bitStream;
	Frame m_frame;

	std::map<std::vector<CharType>, CodeType> dictionary;
	std::vector<CharType> s; // String
	CharType c;
};