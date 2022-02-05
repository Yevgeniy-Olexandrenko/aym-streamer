#pragma once

#include "../Output.h"
#include "WaveAudio.h"
#include "ayumi.h"

class AY38910 : public Output, public WaveAudio
{
public:
	AY38910();
	~AY38910();

public:
	void Open() override;
	bool IsOpened() const override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

protected:
	void FillBuffer(unsigned char* buffer, unsigned long size) override;

private:
	struct ayumi m_ay;
	bool m_isOpened;
};
