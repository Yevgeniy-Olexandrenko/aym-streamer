#pragma once

class CMixer;

//
// This class is used to derive the audio channels
//

class CChannel {
public:
	CChannel(CMixer *pMixer, int Chip, uint8_t ID)
		: m_pMixer(pMixer)
		, m_iChip(Chip)
		, m_iChanId(ID)
		, m_iTime(0)
		, m_iLastValue(0) 
	{
	}

	virtual void EndFrame() { m_iTime = 0; }

	virtual double GetFrequency() const = 0;

protected:
	virtual void Mix(int32_t Value) {
		int32_t Delta = Value - m_iLastValue;
		if (Delta)
			m_pMixer->AddValue(m_iChanId, m_iChip, Delta, Value, m_iTime);
		m_iLastValue = Value;
	}

protected:
	CMixer		*m_pMixer;			// The mixer

	uint32_t	m_iTime;			// Cycle counter, resets every new frame
	uint8_t		m_iChanId;			// This channels unique ID
	uint8_t		m_iChip;			// Chip
	int32_t		m_iLastValue;		// Last value sent to mixer
};
