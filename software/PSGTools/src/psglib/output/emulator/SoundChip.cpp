/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///
/// Emulation of the AY-3-8910 / YM2149 sound chip.
/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

/// Decaps:
/// AY-3-8914 - http://siliconpr0n.org/map/gi/ay-3-8914/mz_mit20x/
/// AY-3-8910 - http://privatfrickler.de/blick-auf-den-chip-soundchip-general-instruments-ay-3-8910/
/// AY-3-8910A - https://seanriddledecap.blogspot.com/2017/01/gi-ay-3-8910-ay-3-8910a-gi-8705-cba.html

/// Links:
/// AY-3-8910 'preliminary' datasheet (which actually describes the AY-3-8914) from 1978:
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_100.png
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_101.png
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_102.png
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_103.png
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_104.png
///   http://spatula-city.org/~im14u2c/intv/gi_micro_programmable_tv_games/page_7_105.png
/// AY-3-8910/8912 Feb 1979 manual: https://web.archive.org/web/20140217224114/http://dev-docs.atariforge.org/files/GI_AY-3-8910_Feb-1979.pdf
/// AY-3-8910/8912/8913 post-1983 manual: http://map.grauw.nl/resources/sound/generalinstrument_ay-3-8910.pdf or http://www.ym2149.com/ay8910.pdf
/// AY-8930 datasheet: http://www.ym2149.com/ay8930.pdf
/// YM2149 datasheet: http://www.ym2149.com/ym2149.pdf
/// YM2203 English datasheet: http://www.appleii-box.de/APPLE2/JonasCard/YM2203%20datasheet.pdf
/// YM2203 Japanese datasheet contents, translated: http://www.larwe.com/technical/chip_ymopn.html

#include "SoundChip.h"
#include <string.h>
#include <math.h>

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

#define BIT(x,n) (((x)>>(n))&1)

namespace
{
	enum
	{
		AY_AFINE    = 0x00,
		AY_ACOARSE  = 0x01,
		AY_BFINE    = 0x02,
		AY_BCOARSE  = 0x03,
		AY_CFINE    = 0x04,
		AY_CCOARSE  = 0x05,
		AY_NOISEPER = 0x06,
		AY_ENABLE   = 0x07,
		AY_AVOL     = 0x08,
		AY_BVOL     = 0x09,
		AY_CVOL     = 0x0a,
		AY_EAFINE   = 0x0b,
		AY_EACOARSE = 0x0c,
		AY_EASHAPE  = 0x0d,
		AY_PORTA    = 0x0e,
		AY_PORTB    = 0x0f,
		AY_EBFINE   = 0x10,
		AY_EBCOARSE = 0x11,
		AY_ECFINE   = 0x12,
		AY_ECCOARSE = 0x13,
		AY_EBSHAPE  = 0x14,
		AY_ECSHAPE  = 0x15,
		AY_ADUTY    = 0x16,
		AY_BDUTY    = 0x17,
		AY_CDUTY    = 0x18,
		AY_NOISEAND = 0x19,
		AY_NOISEOR  = 0x1a,
		AY_TEST     = 0x1f
	};

	const uint8_t MAP_8914_TO_8910[]
	{ 
		AY_AFINE,   AY_BFINE,    AY_CFINE,   AY_EAFINE,
		AY_ACOARSE, AY_BCOARSE,  AY_CCOARSE, AY_EACOARSE,
		AY_ENABLE,  AY_NOISEPER, AY_EASHAPE, AY_AVOL, 
		AY_BVOL,    AY_CVOL,     AY_PORTA,   AY_PORTB
	};

	const double AY_DAC_TABLE[]
	{
		0.0, 0.0,
		0.00999465934234, 0.00999465934234,
		0.01445029373620, 0.01445029373620,
		0.02105745021740, 0.02105745021740,
		0.03070115205620, 0.03070115205620,
		0.04554818036160, 0.04554818036160,
		0.06449988555730, 0.06449988555730,
		0.10736247806500, 0.10736247806500,
		0.12658884565500, 0.12658884565500,
		0.20498970016000, 0.20498970016000,
		0.29221026932200, 0.29221026932200,
		0.37283894102400, 0.37283894102400,
		0.49253070878200, 0.49253070878200,
		0.63532463569100, 0.63532463569100,
		0.80558480201400, 0.80558480201400,
		1.0, 1.0
	};

	const double YM_DAC_TABLE[] =
	{
		0.0, 0.0,
		0.00465400167849, 0.00772106507973,
		0.01095597772180, 0.01396200503550,
		0.01699855039290, 0.02001983672850,
		0.02436865796900, 0.02969405661100,
		0.03506523231860, 0.04039063096060,
		0.04853894865340, 0.05833524071110,
		0.06805523765930, 0.07777523460750,
		0.09251544975970, 0.11108567940800,
		0.12974746318800, 0.14848554207700,
		0.17666895552000, 0.21155107957600,
		0.24638742656600, 0.28110170138100,
		0.33373006790300, 0.40042725261300,
		0.46738384069600, 0.53443198291000,
		0.63517204547200, 0.75800717174000,
		0.87992675669500, 1.0
	};

	// duty cycle used for AY8930 expanded mode
	const uint32_t DUTY_CYCLES[9] =
	{
		0x80000000, // 03.125 %
		0xC0000000, // 06.250 %
		0xF0000000, // 12.500 %
		0xFF000000, // 25.000 %
		0xFFFF0000, // 50.000 %
		0xFFFFFF00, // 75.000 %
		0xFFFFFFF0, // 87.500 %
		0xFFFFFFFC, // 93.750 %
		0xFFFFFFFE  // 96.875 %
	};
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

SoundChip::SoundChip(ChipType chipType, PSGType psgType, int clockRate, int sampleRate)
	: m_chipType(chipType)
	, m_dacTable((!(m_chipType == ChipType::AY8930)) && (psgType == PSGType::AY) ? AY_DAC_TABLE : YM_DAC_TABLE)
	, m_counter(0)
	, m_interpolatorL{}
	, m_interpolatorR{}
	, m_firL{}
	, m_firR{}
	, m_firIndex(0)
	, m_dcFilterL{}
	, m_dcFilterR{}
	, m_dcFilterIndex(0)
	, m_step(double(clockRate) / (sampleRate * 8 * DECIMATE_FACTOR))
{
	Reset();
}

void SoundChip::Reset()
{
	m_mode = 0; // AY-3-8910 compatible mode
	memset(&m_regs, 0, sizeof(m_regs));

	for (int chan = 0; chan < NUM_CHANNELS; chan++)
	{
		m_tone[chan].Reset();
		m_envelope[chan].Reset();
	}
	m_noise.Reset();
}

void SoundChip::Write(uint8_t reg, uint8_t data)
{
	reg &= 0x0F;
	reg |= get_register_bank();
	WriteDirect(reg, data);
}

void SoundChip::WriteDirect(uint8_t reg, uint8_t data)
{
	if (m_chipType == ChipType::AY8914)
	{
		// AY8914 has different register map
		reg = MAP_8914_TO_8910[reg & 0x0F];
	}

	if ((reg & 0x0F) == AY_EASHAPE) reg &= 0x0F;
	m_regs[reg] = data;

	switch (reg)
	{
	case AY_AFINE:
	case AY_ACOARSE:
		m_tone[0].SetPeriod(is_expanded_mode(), m_regs[AY_AFINE], m_regs[AY_ACOARSE]);
		break;

	case AY_BFINE:
	case AY_BCOARSE:
		m_tone[1].SetPeriod(is_expanded_mode(), m_regs[AY_BFINE], m_regs[AY_BCOARSE]);
		break;

	case AY_CFINE:
	case AY_CCOARSE:
		m_tone[2].SetPeriod(is_expanded_mode(), m_regs[AY_CFINE], m_regs[AY_CCOARSE]);
		break;

	case AY_NOISEPER:
		m_noise.SetPeriod(is_expanded_mode(), m_regs[AY_NOISEPER]);
		break;

	case AY_AVOL:
		m_tone[0].SetVolume(m_regs[AY_AVOL]);
		break;

	case AY_BVOL:
		m_tone[1].SetVolume(m_regs[AY_BVOL]);
		break;

	case AY_CVOL:
		m_tone[2].SetVolume(m_regs[AY_CVOL]);
		break;

	case AY_EACOARSE:
	case AY_EAFINE:
		m_envelope[0].SetPeriod(m_regs[AY_EAFINE], m_regs[AY_EACOARSE]);
		break;

	case AY_ENABLE:
		// No action required
		break;

	case AY_EASHAPE:
		if (m_chipType == ChipType::AY8930)
		{
			const uint8_t old_mode = m_mode;
			m_mode = (data >> 4) & 0x0F;
			if (old_mode != m_mode)
			{
				// AY8930 expanded mode
				if (((old_mode & 0x0E) == 0x0A) ^ ((m_mode & 0x0E) == 0x0A))
				{
					for (int i = 0; i < AY_EASHAPE; i++)
					{
						SoundChip::Write(i + 0x00, 0);
						SoundChip::Write(i + 0x10, 0);
					}
				}
			}
		}
		m_envelope[0].SetShape(m_regs[AY_EASHAPE]);
		break;

	case AY_EBFINE:
	case AY_EBCOARSE:
		m_envelope[1].SetPeriod(m_regs[AY_EBFINE], m_regs[AY_EBCOARSE]);
		break;

	case AY_ECFINE:
	case AY_ECCOARSE:
		m_envelope[2].SetPeriod(m_regs[AY_ECFINE], m_regs[AY_ECCOARSE]);
		break;

	case AY_EBSHAPE:
		m_envelope[1].SetShape(m_regs[AY_EBSHAPE]);
		break;

	case AY_ECSHAPE:
		m_envelope[2].SetShape(m_regs[AY_ECSHAPE]);
		break;

	case AY_ADUTY:
		m_tone[0].SetDuty(m_regs[AY_ADUTY]);
		break;

	case AY_BDUTY:
		m_tone[1].SetDuty(m_regs[AY_BDUTY]);
		break;

	case AY_CDUTY:
		m_tone[2].SetDuty(m_regs[AY_CDUTY]);
		break;

	case AY_NOISEAND:
		m_noise.SetMaskAND(m_regs[AY_NOISEAND]);
		break;

	case AY_NOISEOR:
		m_noise.SetMaskOR(m_regs[AY_NOISEOR]);
		break;

	default:
		m_regs[reg] = 0; // reserved, set as 0
		break;
	}
}

void SoundChip::Process(double& outL, double& outR)
{
	// The 8910 has three outputs, each output is the mix of one of the three
	// tone generators and of the (single) noise generator. The two are mixed
	// BEFORE going into the DAC. The formula to mix each channel is:
	// (ToneOn | ToneDisable) & (NoiseOn | NoiseDisable).
	// Note that this means that if both tone and noise are disabled, the output
	// is 1, not 0, and can be modulated changing the volume.

	bool isExp = is_expanded_mode();
	m_noise.Update(isExp, m_chipType == ChipType::AY8930);

	for (int chan = 0; chan < NUM_CHANNELS; chan++)
	{
		m_tone[chan].Update(isExp);
		m_envelope[chan].Update();

		uint8_t disableT = BIT(m_regs[AY_ENABLE], 0 + chan);
		uint8_t disableN = BIT(m_regs[AY_ENABLE], 3 + chan);

#if 0
		int output = (m_tone[chan].GetOutput() | disableT) & (m_noise.GetOutput() | disableN);
#else
		int output = (m_chipType == ChipType::AY8930)
			? (m_tone[chan].GetOutput() & ~disableT) | (m_noise.GetOutput() & ~disableN)
			: (m_tone[chan].GetOutput() |  disableT) & (m_noise.GetOutput() |  disableN);
#endif

		if (output)
		{
			int tone_envelope = m_tone[chan].GetEField(isExp, m_chipType == ChipType::AY8914);

			if (tone_envelope != 0)
			{
				EnvelopeUnit& envelope = m_envelope[isExp ? chan : 0];
				int env_volume = envelope.GetVolume();

				if (m_chipType == ChipType::AY8930)
				{
					if (!isExp)
					{
						// AY8930 has 16 step envelope in comp mode
						env_volume |= 0x01;
					}
				}
				else
				{
					if (m_chipType == ChipType::AY8914)
					{
						// AY8914 has a two bit tone_envelope field
						env_volume >>= (3 - tone_envelope);
					}
				}
				output = env_volume;
			}
			else
			{
				output = m_tone[chan].GetVolume(isExp);
			}
		}

		outL += m_dacTable[output] * m_panL[chan];
		outR += m_dacTable[output] * m_panR[chan];
	}
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void SoundChip::ToneUnit::Reset()
{
	SetPeriod(false, 0, 0);
	SetVolume(0);
	SetDuty(0);

	m_counter = 0;
	m_dutyCounter = 0;
	m_output = 0;
}

void SoundChip::ToneUnit::Update(bool isExp)
{
	if (isExp)
	{
		m_counter += 32;
		while (m_counter >= m_period)
		{
			m_counter -= m_period;
			m_dutyCounter = (m_dutyCounter + 1) & 0x1F;
			m_output = BIT(m_dutyCycle, 0x1F - m_dutyCounter);
		}
	}
	else
	{
		if (++m_counter >= m_period)
		{
			m_counter = 0;
			m_output ^= true;
		}
	}
}

void SoundChip::ToneUnit::SetPeriod(bool isExp, uint8_t fine, uint8_t coarse)
{
	coarse &= (isExp ? 0xFF : 0x0F);
	m_period = fine | (coarse << 8);
	m_period |= (m_period == 0);
}

void SoundChip::ToneUnit::SetVolume(uint8_t volume)
{
	m_volume = volume;
}

void SoundChip::ToneUnit::SetDuty(uint8_t duty)
{
	m_dutyCycle = DUTY_CYCLES[(duty & 0x08) ? 0x08 : (duty & 0x0F)];
}

int SoundChip::ToneUnit::GetOutput() const
{
	return m_output;
}

int SoundChip::ToneUnit::GetVolume(bool isExp) const
{
	return (isExp ? (m_volume & 0x1F) : ((m_volume & 0x0F) << 1 | 0x01));
}

int SoundChip::ToneUnit::GetEField(bool isExp, bool isWide) const
{
	return (m_volume >> (isExp ? 5 : 4)) & (isWide ? 3 : 1);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void SoundChip::NoiseUnit::Reset()
{
	SetPeriod(false, 0);
	SetMaskAND(0);
	SetMaskOR(0);

	m_prescale = 0;
	m_counter = 0;
	m_value = 0;
	m_shift = 1;
	m_output = 0;
}

void SoundChip::NoiseUnit::Update(bool isExp, bool isNew)
{
	// The Random Number Generator of the 8910 is a 17-bit shift
	// register. The input to the shift register is bit0 XOR bit3
	// (bit0 is the output). This was verified on AY-3-8910 and YM2149 chips.
	// AY8930 LFSR algorithm is slightly different, verified from manual.

	if (++m_counter >= m_period)
	{
		// toggle the prescaler output. Noise is no different to channels
		m_counter = 0;
		m_prescale ^= true;

		// TODO : verify algorithm for AY8930
		if (isExp)
		{
			// AY8930 noise generator rate is twice compares as compatibility mode
			if (++m_value >= uint8_t(m_shift & m_maskAND | m_maskOR))
			{
				m_value = 0;
				m_shift = (m_shift >> 1) | ((BIT(m_shift, 0) ^ BIT(m_shift, 2)) << 16);
				m_output ^= 0x01;
			}
		}
		else if (!m_prescale)
		{
			m_shift = (m_shift >> 1) | ((BIT(m_shift, 0) ^ BIT(m_shift, (isNew ? 2 : 3))) << 16);
			m_output = (m_shift & 0x01);
		}
	}
}

void SoundChip::NoiseUnit::SetPeriod(bool isExp, uint8_t period)
{
	m_period = (period & (isExp ? 0xFF : 0x1F));
	m_period |= (m_period == 0);
}

void SoundChip::NoiseUnit::SetMaskAND(uint8_t mask)
{
	m_maskAND = (mask & 0xFF);
}

void SoundChip::NoiseUnit::SetMaskOR(uint8_t mask)
{
	m_maskOR = (mask & 0xFF);
}

int SoundChip::NoiseUnit::GetOutput() const
{
	return m_output;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

namespace
{
	enum { SU = +1, SD = -1, HT = 0, HB = 0, RT = 0x1F, RB = 0x00 };

	const int envelopes[16][2][2]
	{
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SU, RB }, { HB, RB }, },
		{ { SD, RT }, { SD, RT }, },
		{ { SD, RT }, { HB, RB }, },
		{ { SD, RT }, { SU, RB }, },
		{ { SD, RT }, { HT, RT }, },
		{ { SU, RB }, { SU, RB }, },
		{ { SU, RB }, { HT, RT }, },
		{ { SU, RB }, { SD, RT }, },
		{ { SU, RB }, { HB, RB }, },
	};
}

void SoundChip::EnvelopeUnit::Reset()
{
	SetPeriod(0, 0);
	SetShape(0);
}

void SoundChip::EnvelopeUnit::Update()
{
	if (++m_counter >= m_period)
	{
		m_counter = 0;

		m_volume += envelopes[m_shape][m_segment][0];
		if (m_volume < RB || m_volume > RT)
		{
			m_segment ^= 1;
			m_volume = envelopes[m_shape][m_segment][1];
		}
	}
}

void SoundChip::EnvelopeUnit::SetPeriod(uint8_t fine, uint8_t coarse)
{
	m_period = fine | (coarse << 8);
	m_period |= (m_period == 0);
}

void SoundChip::EnvelopeUnit::SetShape(uint8_t shape)
{
	m_shape = (shape & 0x0F);
	m_counter = 0;
	m_segment = 0;
	m_volume = envelopes[m_shape][m_segment][1];
}

int SoundChip::EnvelopeUnit::GetVolume() const
{
	return m_volume;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

void SoundChip::SetPan(int chan, double pan, int is_eqp)
{
	if (is_eqp)
	{
		m_panL[chan] = sqrt(1 - pan);
		m_panR[chan] = sqrt(pan);
	}
	else
	{
		m_panL[chan] = 1 - pan;
		m_panR[chan] = pan;
	}
}

void SoundChip::Process()
{
	double* c_L = m_interpolatorL.c;
	double* y_L = m_interpolatorL.y;
	double* c_R = m_interpolatorR.c;
	double* y_R = m_interpolatorR.y;

	double* firL = &m_firL[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
	double* firR = &m_firR[FIR_SIZE - m_firIndex * DECIMATE_FACTOR];
	m_firIndex = (m_firIndex + 1) % (FIR_SIZE / DECIMATE_FACTOR - 1);

	double y;
	for (int i = DECIMATE_FACTOR - 1; i >= 0; --i)
	{
		m_counter += m_step;
		if (m_counter >= 1)
		{
			m_counter -= 1;

			y_L[0] = y_L[1];
			y_L[1] = y_L[2];
			y_L[2] = y_L[3];

			y_R[0] = y_R[1];
			y_R[1] = y_R[2];
			y_R[2] = y_R[3];

			m_outL = m_outR = 0;
			Process(m_outL, m_outR);
			y_L[3] = m_outL;
			y_R[3] = m_outR;

			y = y_L[2] - y_L[0];
			c_L[0] = 0.50 * y_L[1] + 0.25 * (y_L[0] + y_L[2]);
			c_L[1] = 0.50 * y;
			c_L[2] = 0.25 * (y_L[3] - y_L[1] - y);

			y = y_R[2] - y_R[0];
			c_R[0] = 0.50 * y_R[1] + 0.25 * (y_R[0] + y_R[2]);
			c_R[1] = 0.50 * y;
			c_R[2] = 0.25 * (y_R[3] - y_R[1] - y);
		}

		firL[i] = (c_L[2] * m_counter + c_L[1]) * m_counter + c_L[0];
		firR[i] = (c_R[2] * m_counter + c_R[1]) * m_counter + c_R[0];
	}

	m_outL = Decimate(firL);
	m_outR = Decimate(firR);
}

void SoundChip::RemoveDC()
{
	m_outL = FilterDC(m_dcFilterL, m_dcFilterIndex, m_outL);
	m_outR = FilterDC(m_dcFilterR, m_dcFilterIndex, m_outR);
	m_dcFilterIndex = (m_dcFilterIndex + 1) & (DC_FILTER_SIZE - 1);
}

double SoundChip::GetOutL() const
{
	return m_outL;
}

double SoundChip::GetOutR() const
{
	return m_outR;
}

double SoundChip::Decimate(double* x) const
{
	double y =
		-0.0000046183113992051936 * (x[ 1] + x[191]) +
		-0.0000111776164088722500 * (x[ 2] + x[190]) +
		-0.0000186102645020054320 * (x[ 3] + x[189]) +
		-0.0000251345861356310120 * (x[ 4] + x[188]) +
		-0.0000284942816906661970 * (x[ 5] + x[187]) +
		-0.0000263968287932751590 * (x[ 6] + x[186]) +
		-0.0000170942125588021560 * (x[ 7] + x[185]) +
		+0.0000237981935769668660 * (x[ 9] + x[183]) +
		+0.0000512811602422021830 * (x[10] + x[182]) +
		+0.0000776219782624342700 * (x[11] + x[181]) +
		+0.0000967594266641204160 * (x[12] + x[180]) +
		+0.0001024022930039340200 * (x[13] + x[179]) +
		+0.0000893446142180771060 * (x[14] + x[178]) +
		+0.0000548757001189491830 * (x[15] + x[177]) +
		-0.0000698390822106801650 * (x[17] + x[175]) +
		-0.0001447966132360757000 * (x[18] + x[174]) +
		-0.0002115845291770830800 * (x[19] + x[173]) +
		-0.0002553506910655054400 * (x[20] + x[172]) +
		-0.0002622871437432210400 * (x[21] + x[171]) +
		-0.0002225880592702779900 * (x[22] + x[170]) +
		-0.0001332323049569570400 * (x[23] + x[169]) +
		+0.0001618257876705520600 * (x[25] + x[167]) +
		+0.0003284617538509658100 * (x[26] + x[166]) +
		+0.0004704561157618486300 * (x[27] + x[165]) +
		+0.0005571385145753094400 * (x[28] + x[164]) +
		+0.0005621256512151872600 * (x[29] + x[163]) +
		+0.0004690191855396247800 * (x[30] + x[162]) +
		+0.0002762486683895298600 * (x[31] + x[161]) +
		-0.0003256417948683862200 * (x[33] + x[159]) +
		-0.0006518231028671038800 * (x[34] + x[158]) +
		-0.0009212778730931929800 * (x[35] + x[157]) +
		-0.0010772534348943575000 * (x[36] + x[156]) +
		-0.0010737727700273478000 * (x[37] + x[155]) +
		-0.0008855664539039263400 * (x[38] + x[154]) +
		-0.0005158189609076553400 * (x[39] + x[153]) +
		+0.0005954876719379527700 * (x[41] + x[151]) +
		+0.0011803558710661009000 * (x[42] + x[150]) +
		+0.0016527320270369871000 * (x[43] + x[149]) +
		+0.0019152679330965555000 * (x[44] + x[148]) +
		+0.0018927324805381538000 * (x[45] + x[147]) +
		+0.0015481870327877937000 * (x[46] + x[146]) +
		+0.0008947069583494130600 * (x[47] + x[145]) +
		-0.0010178225878206125000 * (x[49] + x[143]) +
		-0.0020037400552054292000 * (x[50] + x[142]) +
		-0.0027874356824117317000 * (x[51] + x[141]) +
		-0.0032103299880219430000 * (x[52] + x[140]) +
		-0.0031540624117984395000 * (x[53] + x[139]) +
		-0.0025657163651900345000 * (x[54] + x[138]) +
		-0.0014750752642111449000 * (x[55] + x[137]) +
		+0.0016624165446378462000 * (x[57] + x[135]) +
		+0.0032591192839069179000 * (x[58] + x[134]) +
		+0.0045165685815867747000 * (x[59] + x[133]) +
		+0.0051838984346123896000 * (x[60] + x[132]) +
		+0.0050774264697459933000 * (x[61] + x[131]) +
		+0.0041192521414141585000 * (x[62] + x[130]) +
		+0.0023628575417966491000 * (x[63] + x[129]) +
		-0.0026543507866759182000 * (x[65] + x[127]) +
		-0.0051990251084333425000 * (x[66] + x[126]) +
		-0.0072020238234656924000 * (x[67] + x[125]) +
		-0.0082672928192007358000 * (x[68] + x[124]) +
		-0.0081033739572956287000 * (x[69] + x[123]) +
		-0.0065831115395702210000 * (x[70] + x[122]) +
		-0.0037839040415292386000 * (x[71] + x[121]) +
		+0.0042781252851152507000 * (x[73] + x[119]) +
		+0.0084176358598320178000 * (x[74] + x[118]) +
		+0.0117256605746305500000 * (x[75] + x[117]) +
		+0.0135504766477886720000 * (x[76] + x[116]) +
		+0.0133881893699974960000 * (x[77] + x[115]) +
		+0.0109795012423412590000 * (x[78] + x[114]) +
		+0.0063812749416854130000 * (x[79] + x[113]) +
		-0.0074212296041538880000 * (x[81] + x[111]) +
		-0.0148645630434021300000 * (x[82] + x[110]) +
		-0.0211435846221781040000 * (x[83] + x[109]) +
		-0.0250427505875860900000 * (x[84] + x[108]) +
		-0.0254735309425472010000 * (x[85] + x[107]) +
		-0.0216273100178821960000 * (x[86] + x[106]) +
		-0.0131043233832255430000 * (x[87] + x[105]) +
		+0.0170651339899804760000 * (x[89] + x[103]) +
		+0.0369789192644519520000 * (x[90] + x[102]) +
		+0.0582331806209395800000 * (x[91] + x[101]) +
		+0.0790720120814059490000 * (x[92] + x[100]) +
		+0.0976759987169523170000 * (x[93] + x[99 ]) +
		+0.1123604593695093200000 * (x[94] + x[98 ]) +
		+0.1217634357728773100000 * (x[95] + x[97 ]) +
		+0.125 * x[96];

	memcpy(&x[FIR_SIZE - DECIMATE_FACTOR], x, DECIMATE_FACTOR * sizeof(double));
	return y;
}

double SoundChip::FilterDC(DCFilter& filter, int index, double x) const
{
	filter.sum += -filter.delay[index] + x;
	filter.delay[index] = x;
	return (x - (filter.sum / DC_FILTER_SIZE));
}
