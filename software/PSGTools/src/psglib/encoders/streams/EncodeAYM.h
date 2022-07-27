#pragma once

#include <iostream>
#include "Property.h"
#include "encoders/Encoder.h"

class EncodeAYM : public Encoder
{
	class Delta
	{
	public:
		Delta(uint16_t from, uint16_t to);

		RO_PROP_DEF(int16_t, value);
		RO_PROP_DEF(uint8_t, size);
	};

	class DeltaList
	{
	public:
		DeltaList();
		int8_t GetIndex(const Delta& delta);

	private:
		int16_t m_list[32];
		uint8_t m_index;
	};

	class BitStream
	{
	public:
		void Open(std::ostream& stream);
		void Write(uint16_t data, uint8_t size);
		void Close();

	private:
		std::ostream* m_stream;
		uint8_t m_buffer;
		uint8_t m_count;
	};

public:
	bool Open(const Stream& stream) override;
	void Encode(const Frame& frame) override;
	void Close(const Stream& stream) override;

private:
	void WriteDelta(const Delta& delta);
	void WriteChipDelta(const Frame& frame, uint8_t chip, bool isLast);
	void WriteRegDelta (const Frame& frame, uint8_t chip, uint8_t reg);
	void WritePerDelta (const Frame& frame, uint8_t chip, uint8_t per);
	void WriteStepDelta();

private:
	std::ofstream m_output;
	DeltaList m_deltaList;
	BitStream m_bitStream;
	uint16_t m_oldStep = 1;
	uint16_t m_newStep = 1;
	Frame m_frame;
	bool m_isTS;
};