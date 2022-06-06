#pragma once

class ChipAY89xx
{
	static const int DECIMATE_FACTOR = 8;
	static const int FIR_SIZE        = 192;
	static const int DC_FILTER_SIZE  = 1024;

	struct Interpolator 
	{
		double c[4];
		double y[4];
	};

	struct DCFilter 
	{
		double sum;
		double delay[DC_FILTER_SIZE];
	};

public:
	using byte = unsigned char;

	bool Configure(double clockRate, int sampleRate);
	void SetPan(int index, double pan, int is_eqp);

	void Reset();
	void Write(byte reg, byte data);

	void Process();
	void RemoveDC();
	double GetOutL() const;
	double GetOutR() const;

	private:
	double Decimate(double* x) const;
	double FilterDC(DCFilter& flter, int index, double x) const;

private:
	double m_step;
	double m_counter;

	Interpolator m_interpolatorL;
	Interpolator m_interpolatorR;

	double m_firL[FIR_SIZE * 2];
	double m_firR[FIR_SIZE * 2];
	int m_firIndex;

	DCFilter m_dcFilterL;
	DCFilter m_dcFilterR;
	int m_dcFilterIndex;

	double m_outL;
	double m_outR;

protected:
	class ToneUnit
	{
	public:
		void SetPeriod(int period);
		int  GetPeriod() const;
		void Reset();
		int  Update();

	private:
		int m_period;
		int m_counter;
		int m_tone;
	};

	class NoiseUnit
	{
	public:
		void SetPeriod(int period);
		void Reset();
		int  Update();

	private:
		int m_period;
		int m_counter;
		int m_noise;
	};

	class MixerUnit
	{
	public:
		void SetFlags(bool T_Off, bool N_Off, bool E_On);

	private:
		int m_T_Off;
		int m_N_Off;
		int m_E_On;
		int m_volume;
	};

	class EnvelopeUnit
	{
	public:
		void SetPeriod(int period);
		void SetShape(int shape);
		int  GetPeriod() const;
		void Reset();
		int  Update();

	private:
		void ResetSegment();

	private:
		int m_counter;
		int m_period;
		int m_shape;
		int m_segment;
		int m_envelope;
	};

protected:
	virtual void InternalSetPan(int index, double panL, double panR) = 0;
	virtual void InternalReset() = 0;
	virtual void InternalWrite(byte reg, byte data) = 0;
	virtual void InternalUpdate(double& outL, double& outR) = 0;
};
