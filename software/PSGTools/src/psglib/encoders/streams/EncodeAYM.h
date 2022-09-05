#pragma once

#include <sstream>
#include "Property.h"
#include "encoders/Encoder.h"
#include "encoders/BitStream.h"

#define DBG_ENCODE_AYM 1

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

	class Chunk
	{
	public:
		void Start();
		void Stop();

		BitStream& GetStream();
		const uint8_t* GetData() const;
		const size_t GetSize() const;

	private:
		BitStream m_stream;
		std::string m_data;
	};

public:
	bool Open(const Stream& stream) override;
	void Encode(const Frame& frame) override;
	void Close(const Stream& stream) override;

private:
	void WriteDelta(const Delta& delta, BitStream& stream);
	void WriteChipData(const Frame& frame, int chip, bool isLast, BitStream& stream);
	void WriteFrameChunk(const Frame& frame);
	void WriteStepChunk();
	void WriteChunk(const Chunk& chunk);

private:
	std::ofstream m_output;
	DeltaList m_deltaList;
	uint16_t m_oldStep = 1;
	uint16_t m_newStep = 1;
	Frame m_frame;
	bool m_isTS;
};

