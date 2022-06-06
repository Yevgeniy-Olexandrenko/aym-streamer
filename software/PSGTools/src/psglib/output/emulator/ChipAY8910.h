#pragma once

#include "ChipAY89xx.h"

class ChipAY8910 : public ChipAY89xx
{
public:
	bool Configure(double clockRate, int sampleRate, bool isYM);

protected:
	void InternalSetPan(int chan, double panL, double panR) override;
	void InternalReset() override;
	void InternalWrite(byte reg, byte data) override;
	void InternalUpdate(double& outL, double& outR) override;

protected:
	struct Channel
	{
		ToneUnit tone;
		int volume;
		int t_off;
		int n_off;
		int e_on;
		
		double panL;
		double panR;

		void SetVolume(int volume);
		void SetMixer(bool t_off, bool n_off, bool e_on);
	};

protected:
	const double* m_dacTable = nullptr;
	Channel       m_channels[3];
	NoiseUnit     m_noise;
	EnvelopeUnit  m_envelope;
};
