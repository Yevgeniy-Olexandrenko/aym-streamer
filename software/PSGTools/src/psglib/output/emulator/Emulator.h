#pragma once

#include "SoundChip.h"
#include "output/Output.h"
#include "WaveAudio.h"

constexpr int k_emulatorSampleRate = 44100;

class Emulator : public Output, public WaveAudio
{
public:
	Emulator();
	virtual ~Emulator();

protected:
	const std::string GetDeviceName() const override;

	bool OpenDevice() override;
	bool InitDstChip(const Chip& srcChip, Chip& dstChip) override;
	bool WriteToChip(int chip, const Data& data) override;
	void FillBuffer(unsigned char* buffer, unsigned long size) override;
	void CloseDevice() override;

private:
	std::unique_ptr<SoundChip> m_ay[2];
};
