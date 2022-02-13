#pragma once

#include <ayumi/ayumi.h>
#include "output/Output.h"
#include "WaveAudio.h"

class AY38910 : public Output, public WaveAudio
{
public:
	AY38910();
	virtual ~AY38910();

public:
	bool Open() override;
	bool Init(const Module& module) override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

protected:
	void FillBuffer(unsigned char* buffer, unsigned long size) override;

private:
	bool InitChip(uint8_t chip, const Module& module);
	void WriteToChip(uint8_t chip, const Frame& frame, bool force);

private:
	bool  m_ts;
	ayumi m_ay[2];
};
