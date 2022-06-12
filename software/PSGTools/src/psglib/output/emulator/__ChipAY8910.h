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

public:
	class ToneUnit
	{
	public:
		void Reset();
		void SetPeriodL(int period);
		void SetPeriodH(int period);
		int  Update();

	protected:
		int m_period;
		int m_counter;
		int m_tone;
	};

	class NoiseUnit
	{
	public:
		void Reset();
		void SetPeriod(int period);
		int  Update();

	protected:
		int m_period;
		int m_counter;
		int m_noise;
	};

	class MixerUnit
	{
	public:
		void Reset();
		void SetEnable(int flags);
		void SetVolume(int volume);
		int  GetOutput(int tone, int noise, int envelope) const;

	protected:
		int m_T_Off;
		int m_N_Off;
		int m_E_On;
		int m_volume;
	};

	class EnvelopeUnit
	{
	public:
		void Reset();
		void SetPeriodL(int period);
		void SetPeriodH(int period);
		void SetShape(int shape);
		int  Update();

	protected:
		int m_counter;
		int m_period;
		int m_shape;
		int m_segment;
		int m_envelope;
	};

private:
	const double* m_dacTable = nullptr;

	struct Channel
	{
		ToneUnit  tone;
		MixerUnit mixer;
				
		double panL;
		double panR;
	} m_channels[3];

	NoiseUnit m_noise;
	EnvelopeUnit m_envelope;
};
