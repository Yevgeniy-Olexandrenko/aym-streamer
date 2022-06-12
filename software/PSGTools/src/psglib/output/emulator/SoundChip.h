#pragma once

#include <stdint.h>

#define BIT(x,n) (((x)>>(n))&1)

class SoundChip
{
	static const int DECIMATE_FACTOR = 8;
	static const int FIR_SIZE = 192;
	static const int DC_FILTER_SIZE = 1024;

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
	void SetPan(int chan, double pan, int is_eqp);

	void Reset();
	void Write(uint8_t reg, uint8_t data);

	void Process();
	void RemoveDC();

	double GetOutL() const;
	double GetOutR() const;

private:
	void   Compute (double& outL, double& outR);
	double Decimate(double* x) const;
	double FilterDC(DCFilter& filter, int index, double x) const;

protected:
	enum class ChipType
	{
		AY8910,
		AY8914,
		AY8930,
		YM2149,
	};

	enum class PSGType
	{
		AY,
		YM
	};

	SoundChip(ChipType chipType, PSGType psgType, uint32_t clockRate, uint32_t sampleRate);

private:
	static constexpr uint32_t NUM_CHANNELS = 3;

	enum
	{
		AY_AFINE = 0x00,
		AY_ACOARSE = 0x01,
		AY_BFINE = 0x02,
		AY_BCOARSE = 0x03,
		AY_CFINE = 0x04,
		AY_CCOARSE = 0x05,
		AY_NOISEPER = 0x06,
		AY_ENABLE = 0x07,
		AY_AVOL = 0x08,
		AY_BVOL = 0x09,
		AY_CVOL = 0x0a,
		AY_EAFINE = 0x0b,
		AY_EACOARSE = 0x0c,
		AY_EASHAPE = 0x0d,
		AY_PORTA = 0x0e,
		AY_PORTB = 0x0f,
		AY_EBFINE = 0x10,
		AY_EBCOARSE = 0x11,
		AY_ECFINE = 0x12,
		AY_ECCOARSE = 0x13,
		AY_EBSHAPE = 0x14,
		AY_ECSHAPE = 0x15,
		AY_ADUTY = 0x16,
		AY_BDUTY = 0x17,
		AY_CDUTY = 0x18,
		AY_NOISEAND = 0x19,
		AY_NOISEOR = 0x1a,
		AY_TEST = 0x1f
	};

	struct tone_t
	{
		uint32_t period;
		uint8_t volume;
		uint8_t duty;
		int32_t count;
		uint8_t duty_cycle;
		uint8_t output;

		void reset()
		{
			period = 0;
			volume = 0;
			duty = 0;
			count = 0;
			duty_cycle = 0;
			output = 0;
		}

		void set_period(uint8_t fine, uint8_t coarse)
		{
			period = fine | (coarse << 8);
		}

		void set_volume(uint8_t val)
		{
			volume = val;
		}

		void set_duty(uint8_t val)
		{
			duty = val;
		}
	};

	class ToneUnit
	{
	public:
		void Reset();
		void Update(bool isExp);
		void SetPeriod(bool isExp, uint8_t fine, uint8_t coarse);
		void SetVolume(uint8_t volume);
		void SetDuty(uint8_t duty);
		int  GetOutput() const;
		int  GetVolume(bool isExp) const;
		int  GetEField(bool isExp, bool isWide) const;

	private:
		int m_counter;
		int m_period;
		int m_dutyCounter;
		int m_dutyCycle;
		int m_output;
		int m_volume;
	};

	class EnvelopeUnit
	{
	public:
		void Reset();
		void Update();
		void SetPeriod(uint8_t fine, uint8_t coarse);
		void SetShape(uint8_t shape);
		int  GetVolume() const;

	private:
		int m_counter;
		int m_period;
		int m_shape;
		int m_segment;
		int m_volume;
	};

	class NoiseUnit
	{
	public:
		void Reset();
		void Update(bool isExp, bool isNew);
		void SetPeriod(bool isExp, uint8_t period);
		void SetMaskAND(uint8_t mask);
		void SetMaskOR(uint8_t mask);
		int  GetOutput() const;

	private:
		int m_prescale;
		int m_counter;
		int m_period;
		int m_value;
		int m_maskAND;
		int m_maskOR;
		int m_shift;
		int m_output;
	};

	// inlines
	inline bool is_expanded_mode() { return ((m_chipType == ChipType::AY8930) && ((m_mode & 0xe) == 0xa)); }
	inline uint8_t get_register_bank() { return is_expanded_mode() ? (m_mode & 0x1) << 4 : 0; }
	inline uint8_t tone_enable(int chan) { return BIT(m_regs[AY_ENABLE], chan); }
	inline uint8_t noise_enable(int chan) { return BIT(m_regs[AY_ENABLE], 3 + chan); }

private:
	ChipType m_chipType;
	const double* m_dacTable;

	uint8_t m_regs[16 * 2];
	uint8_t m_mode;

	ToneUnit m_tone[NUM_CHANNELS];
	EnvelopeUnit m_envelope[NUM_CHANNELS];
	NoiseUnit m_noise;
	uint8_t m_vol_enabled[NUM_CHANNELS];

	double m_panL[NUM_CHANNELS];
	double m_panR[NUM_CHANNELS];

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
};

class ChipAY8910 : public SoundChip
{
public:
	ChipAY8910(uint32_t clock, uint32_t sampleRate)
		: SoundChip(ChipType::AY8910, PSGType::AY, clock, sampleRate)
	{}
};

class ChipAY8914 : public SoundChip
{
public:
	ChipAY8914(uint32_t clock, uint32_t sampleRate)
		: SoundChip(ChipType::AY8914, PSGType::AY, clock, sampleRate)
	{}
};

class ChipAY8930 : public SoundChip
{
public:
	ChipAY8930(uint32_t clock, uint32_t sampleRate)
		: SoundChip(ChipType::AY8930,  PSGType::YM, clock, sampleRate)
	{}
};

class ChipYM2149 : public SoundChip
{
public:
	ChipYM2149(uint32_t clock, uint32_t sampleRate)
		: SoundChip(ChipType::YM2149, PSGType::YM, clock, sampleRate)
	{}
};
