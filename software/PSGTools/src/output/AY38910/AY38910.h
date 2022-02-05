#pragma once

#include "../Output.h"
#include "WaveAudio.h"
#include "ayumi.h"

class AY38910 : public Output, public WaveAudio
{
public:
	AY38910(const Module& module);
	virtual ~AY38910();

public:
	void Open() override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

protected:
	void FillBuffer(unsigned char* buffer, unsigned long size) override;

private:
	ayumi m_ay;
};
