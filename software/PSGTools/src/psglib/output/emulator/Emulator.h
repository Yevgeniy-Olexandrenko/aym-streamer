#pragma once

#include "SoundChip.h"
#include "output/Output.h"
#include "WaveAudio.h"

constexpr int EmulatorSampleRate = 44100;

class Emulator : public Output, public WaveAudio
{
public:
	Emulator();
	virtual ~Emulator();

public:
	bool Open() override;
	bool Init(const Stream& stream) override;
	void Close() override;

protected:
	bool InitChip(int chip);
	void WriteToChip(int chip, const std::vector<uint8_t>& data) override;
	void FillBuffer(unsigned char* buffer, unsigned long size) override;
	const std::string GetOutputDeviceName() const override;

private:
	std::unique_ptr<SoundChip> m_ay[2];
};
