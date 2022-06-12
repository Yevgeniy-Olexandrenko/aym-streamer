#pragma once

#include "ChipAY8910.h"

class ChipAY8930 : public ChipAY89xx
{
protected:
	void InternalSetPan(int chan, double panL, double panR) override;
	void InternalReset() override;
	void InternalWrite(byte reg, byte data) override;
	void InternalUpdate(double& outL, double& outR) override;
	void ResetMode();

public:
	class ToneUnit : public ChipAY8910::ToneUnit
	{
	public:
		void Reset();
		void SetPeriodL(bool exp, int period);
		void SetPeriodH(bool exp, int period);
		void SetDuty(int duty);
		int  Update(bool exp);
	};

	class NoiseUnit : public ChipAY8910::NoiseUnit
	{
	public:
		void Reset();
		void SetPeriod(bool exp, int period);
		void SetAndMask(int mask);
		void SetOrMask(int mask);
		int  Update(bool exp);
	};

	class MixerUnit : public ChipAY8910::MixerUnit
	{
	public:
		void SetVolume(bool exp, int volume);
		int  GetOutput(bool exp, int tone, int noise, int envelope) const;
	};

	// there are no changes in envelope unit
	using EnvelopeUnit = ChipAY8910::EnvelopeUnit;

private:
	struct Channel
	{
		ToneUnit tone;
		MixerUnit mixer;
		EnvelopeUnit envelope;

		double panL;
		double panR;
	} m_channels[3];

	NoiseUnit m_noise;

	bool m_expMode = false;
	int  m_expBank = 0x00;
	
};