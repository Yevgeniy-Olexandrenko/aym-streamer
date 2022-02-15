#pragma once

#include <stdint.h>
#include <string>

class Chip
{
public:
	enum class Count
	{
		SingleChip, TurboSound
	};

	enum class Model
	{
		Unknown, AY, YM, Compatible
	};

	enum class Frequency
	{
		Unknown, F1000000, F1750000, F1773400, F2000000
	};

	enum class Channels
	{
		Unknown, MONO, ABC, ACB
	};

public:
	Chip();
	std::string toString() const;

	void count(Count count);
	Count count() const;

	void model(Model model);
	Model model() const;
	bool modelKnown() const;

	void frequency(Frequency frequency);
	void freqValue(uint32_t freqValue);
	Frequency frequency() const;
	uint32_t freqValue(uint32_t defFreqValue) const;
	bool frequencyKnown() const;

	void channels(Channels channels);
	Channels channels() const;
	bool channelsKnown() const;

private:
	Count     m_count;
	Model     m_model;
	Frequency m_frequency;
	Channels  m_channels;
};