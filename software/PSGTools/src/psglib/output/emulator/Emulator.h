#pragma once

#define USE_NEW_AY8910

#ifdef USE_NEW_AY8910
#include "ChipAY8910.h"
#include "ChipAY8930.h"
#else
#include "ayumi.h"
#endif
#include "output/Output.h"
#include "WaveAudio.h"

class Emulator : public Output, public WaveAudio
{
public:
	Emulator();
	virtual ~Emulator();
	std::string name() const override;

public:
	bool Open() override;
	bool Init(const Stream& stream) override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

protected:
	void FillBuffer(unsigned char* buffer, unsigned long size) override;

private:
	bool InitChip(uint8_t chipIndex);
	void WriteToChip(uint8_t chipIndex, const Frame& frame, bool force);

private:
#ifdef USE_NEW_AY8910
	ChipAY8930 m_ay[2];
#else
	ayumi m_ay[2];
#endif
};
