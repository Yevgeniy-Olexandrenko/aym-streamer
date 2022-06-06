#include <algorithm>
#include "AY8930.h"

namespace CAPU
{
	const uint32_t	BASE_FREQ_NTSC		= 1789773;
	const uint32_t	BASE_FREQ_PAL		= 1662607;
	const uint8_t	FRAME_RATE_NTSC		= 60;
	const uint8_t	FRAME_RATE_PAL		= 50;
}

// AY8930 channel class
const int32_t EXP_VOLUME[32] = {
	  1,   1,   1,   2,
	  2,   2,   3,   4,
	  4,   5,   6,   7,
	  9,  11,  13,  15,
	 18,  22,  26,  31,
	 37,  45,  53,  63,
	 75,  90, 107, 127,
	151, 180, 214, 255,
};

const int32_t DUTY_CYCLES[16][32] = {
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CAY8930Channel::CAY8930Channel(CMixer *pMixer, uint8_t ID)
	: CChannel(pMixer, SNDCHIP_AY8930, ID),
	m_iVolume(0),
	m_iPeriod(0),
	m_iPeriodClock(0),
	m_iDutyCycle(0),
	m_iDutyCycleCounter(0),
	m_iEnvelopePeriod(0),
	m_iEnvelopeClock(0),
	m_iEnvelopeLevel(0),
	m_iEnvelopeShape(0),
	m_bEnvelopeHold(true),
	m_bSquareHigh(false),
	m_bSquareDisable(false),
	m_bNoiseDisable(false)
{
}

void CAY8930Channel::Process(uint32_t Time)
{
	m_iPeriodClock += Time;
	if (m_iPeriodClock >= m_iPeriod) {
		m_iPeriodClock = 0;
		m_iDutyCycleCounter = (m_iDutyCycleCounter + 1) & 0x1F;
	}
	m_iTime += Time;
}

void CAY8930Channel::Reset()
{
	m_iVolume = 0;
	m_iPeriod = 0;
	m_iPeriodClock = 0;
	m_iDutyCycleCounter = 0;
	m_iDutyCycle = 0;
	m_bSquareHigh = false;
	m_bSquareDisable = true;
	m_bNoiseDisable = true;
	m_iEnvelopePeriod = 0;
	m_iEnvelopeClock = 0;
	m_iEnvelopeLevel = 0;
	m_iEnvelopeShape = 0;
	m_bEnvelopeHold = true;
}

uint32_t CAY8930Channel::GetTime()
{
	if (m_iPeriod < 2U || !m_iVolume)
		return 0xFFFFFU;
	return m_iPeriod - m_iPeriodClock;
}

void CAY8930Channel::Output(uint32_t Noise)
{
	int Level = ((m_iVolume & 0x20) ? m_iEnvelopeLevel : m_iVolume) & 0x1F;
	int32_t Output = EXP_VOLUME[Level];
	if (!m_bSquareDisable && !(DUTY_CYCLES[m_iDutyCycle & 0x0F][m_iDutyCycleCounter]) && m_iPeriod >= 2U)
		Output = 0;
	if (!m_bNoiseDisable && !Noise)
		Output = 0;
	Mix(Output);
}

double CAY8930Channel::GetFrequency() const
{
	if (m_bSquareDisable || !m_iPeriod)
		return 0.;
	return CAPU::BASE_FREQ_NTSC / 2. / (m_iPeriod*16);
}

void CAY8930Channel::RunEnvelope(uint32_t Time)
{
	m_iEnvelopeClock += Time;
	if (m_iEnvelopeClock >= m_iEnvelopePeriod && m_iEnvelopePeriod) {
		m_iEnvelopeClock = 0;
		if (!m_bEnvelopeHold) {
			m_iEnvelopeLevel += (m_iEnvelopeShape & 0x04) ? 1 : -1;
			m_iEnvelopeLevel &= 0x3F;
		}
		if (m_iEnvelopeLevel & 0x20) {
			if (m_iEnvelopeShape & 0x08) {
				if ((m_iEnvelopeShape & 0x03) == 0x01 || (m_iEnvelopeShape & 0x03) == 0x02)
					m_iEnvelopeShape ^= 0x04;
				if (m_iEnvelopeShape & 0x01)
					m_bEnvelopeHold = true;
				m_iEnvelopeLevel = (m_iEnvelopeShape & 0x04) ? 0 : 0x1F;
			}
			else {
				m_bEnvelopeHold = true;
				m_iEnvelopeLevel = 0;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CAY8930::CAY8930(CMixer *pMixer)
	: m_pMixer(pMixer)
{
	m_pChannel[0] = new CAY8930Channel(pMixer, CHANID_AY8930_CH1);
	m_pChannel[1] = new CAY8930Channel(pMixer, CHANID_AY8930_CH2);
	m_pChannel[2] = new CAY8930Channel(pMixer, CHANID_AY8930_CH3);
	Reset();
}

CAY8930::~CAY8930()
{
	for (auto x : m_pChannel)
		if (x) delete x;
}

void CAY8930::Reset()
{
	m_iNoiseState = 0x1FFFF;
	m_iNoisePeriod = 0xFF << 5;
	m_iNoiseClock = 0;
	m_iNoiseValue = 0;
	m_iNoiseLatch = 0;
	m_iNoiseANDMask = 0xFF;
	m_iNoiseORMask = 0x00;
	
	for (auto x : m_pChannel) x->Reset();
}

void CAY8930::Process(uint32_t Time)
{
	while (Time > 0U) 
	{
		uint32_t TimeToRun = Time;

		for (const auto x : m_pChannel)
		{
			if (x->m_iEnvelopeClock < x->m_iEnvelopePeriod)
			{
				TimeToRun = std::min<uint32_t>(x->m_iEnvelopePeriod - x->m_iEnvelopeClock, TimeToRun);
			}
		}
		if (m_iNoiseClock < m_iNoisePeriod)
		{
			TimeToRun = std::min<uint32_t>(m_iNoisePeriod - m_iNoiseClock, TimeToRun);
		}
		for (const auto x : m_pChannel)
		{
			TimeToRun = std::min<uint32_t>(x->GetTime(), TimeToRun);
		}

		Time -= TimeToRun;

		RunNoise(TimeToRun);
		for (auto x : m_pChannel) x->RunEnvelope(TimeToRun);
		for (auto x : m_pChannel) x->Process(TimeToRun);
		for (auto x : m_pChannel) x->Output(m_iNoiseLatch & 0x01);
	}
}

void CAY8930::EndFrame()
{
	for (auto x : m_pChannel) x->EndFrame();
}

void CAY8930::WriteReg(uint8_t Port, uint8_t Value)
{
	switch (Port) {
	case 0x00: case 0x02: case 0x04:
	{
		auto pChan = m_pChannel[Port >> 1];
		pChan->m_iPeriod = (pChan->m_iPeriod & 0xFF00) | (Value);
	}
		break;
	case 0x01: case 0x03: case 0x05:
	{
		auto pChan = m_pChannel[Port >> 1];
		pChan->m_iPeriod = (pChan->m_iPeriod & 0x00FF) | (Value << 8);
	}
		break;
	case 0x06:
		m_iNoisePeriod = Value ? ((Value & 0xFF) << 5) : 0x10;
		break;
	case 0x07:
		for (int i = 0; i < 3; ++i) {
			auto pChan = m_pChannel[i];
			pChan->m_bSquareDisable = (Value & (1 << i)) != 0;
			pChan->m_bNoiseDisable = (Value & (1 << (i + 3))) != 0;
		}
		break;
	case 0x08: case 0x09: case 0x0A:
		m_pChannel[Port - 0x08]->m_iVolume = Value;
		break;
	case 0x0B:
		m_pChannel[0]->m_iEnvelopePeriod = (m_pChannel[0]->m_iEnvelopePeriod & 0x7F800) | (Value << 3);
		break;
	case 0x0C:
		m_pChannel[0]->m_iEnvelopePeriod = (m_pChannel[0]->m_iEnvelopePeriod & 0x007F8) | (Value << 11);
		break;
	case 0x0D:
		m_pChannel[0]->m_iEnvelopeClock = 0;
		m_pChannel[0]->m_iEnvelopeShape = Value;
		m_pChannel[0]->m_bEnvelopeHold = false;
		m_pChannel[0]->m_iEnvelopeLevel = (Value & 0x04) ? 0 : 0x1F;
		break;
	case 0x10:
		m_pChannel[1]->m_iEnvelopePeriod = (m_pChannel[1]->m_iEnvelopePeriod & 0x7F800) | (Value << 3);
		break;
	case 0x11:
		m_pChannel[1]->m_iEnvelopePeriod = (m_pChannel[1]->m_iEnvelopePeriod & 0x007F8) | (Value << 11);
		break;
	case 0x12:
		m_pChannel[2]->m_iEnvelopePeriod = (m_pChannel[2]->m_iEnvelopePeriod & 0x7F800) | (Value << 3);
		break;
	case 0x13:
		m_pChannel[2]->m_iEnvelopePeriod = (m_pChannel[2]->m_iEnvelopePeriod & 0x007F8) | (Value << 11);
		break;
	case 0x14:
		m_pChannel[1]->m_iEnvelopeClock = 0;
		m_pChannel[1]->m_iEnvelopeShape = Value;
		m_pChannel[1]->m_bEnvelopeHold = false;
		m_pChannel[1]->m_iEnvelopeLevel = (Value & 0x04) ? 0 : 0x1F;
		break;
	case 0x15:
		m_pChannel[2]->m_iEnvelopeClock = 0;
		m_pChannel[2]->m_iEnvelopeShape = Value;
		m_pChannel[2]->m_bEnvelopeHold = false;
		m_pChannel[2]->m_iEnvelopeLevel = (Value & 0x04) ? 0 : 0x1F;
		break;
	case 0x16: case 0x17: case 0x18:
		m_pChannel[Port - 0x16]->m_iDutyCycle = Value;
		break;
	case 0x19:
		m_iNoiseANDMask = Value;
		break;
	case 0x1A:
		m_iNoiseORMask = Value;
		break;
	}
}

void CAY8930::RunNoise(uint32_t Time)
{
	m_iNoiseClock += (int)(Time * 2);
	while (m_iNoiseClock >= m_iNoisePeriod) {
		m_iNoiseClock -= m_iNoisePeriod;
		if (m_iNoiseValue >= ((m_iNoiseState & 0xFF & m_iNoiseANDMask) | m_iNoiseORMask)) {
			m_iNoiseValue = 0;

			m_iNoiseLatch ^= 1;

			// credits to Enfau
			int feedback = (m_iNoiseState & 1) ^ ((m_iNoiseState >> 2) & 1);
			m_iNoiseState >>= 1;
			m_iNoiseState |= (feedback << 16);
		}
		m_iNoiseValue += 1;
		
	}
}