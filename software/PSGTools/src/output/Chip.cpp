#include <sstream>
#include "Chip.h"

Chip::Chip()
	: m_count(Count::SingleChip)
	, m_model(Model::Unknown)
	, m_frequency(Frequency::Unknown)
	, m_channels(Channels::Unknown)
{
}

std::string Chip::toString() const
{
	auto PrintChipType = [](std::ostream& stream, Model type)
	{
		switch (type)
		{
		case Model::AY: stream << "AY-3-8910(12)"; break;
		case Model::YM: stream << "YM2149F"; break;
		case Model::Compatible: stream << "AY/YM Compatible"; break;
		}
	};

	std::stringstream stream;
	if (count() == Count::TurboSound)
	{
		if (modelKnown())
		{
			stream << "2 x ";
			PrintChipType(stream, model());
		}
		else
		{
			stream << "Turbo Sound";
		}
		stream << ' ';
	}
	else if (modelKnown())
	{
		PrintChipType(stream, model());
		stream << ' ';
	}

	if (frequencyKnown())
	{
		stream << double(freqValue()) / 1000000 << " MHz" << ' ';
	}

	if (channelsKnown())
	{
		if (channels() == Chip::Channels::MONO) stream << "MONO";
		if (channels() == Chip::Channels::ABC ) stream << "ABC";
		if (channels() == Chip::Channels::ACB ) stream << "ACB";
	}

	return stream.str();
}

void Chip::count(Count count)
{
	m_count = count;
}

Chip::Count Chip::count() const
{
	return m_count;
}

void Chip::model(Model model)
{
	m_model = model;
}

Chip::Model Chip::model() const
{
	return m_model;
}

bool Chip::modelKnown() const
{
	return (model() != Model::Unknown);
}

void Chip::frequency(Frequency freq)
{
	m_frequency = freq;
}

void Chip::freqValue(uint32_t freqValue)
{
	switch (freqValue)
	{
	case 1000000: frequency(Frequency::F1000000); break;
	case 1750000: frequency(Frequency::F1750000); break;
	case 1773400: frequency(Frequency::F1773400); break;
	case 2000000: frequency(Frequency::F2000000); break;
	default: frequency(Frequency::Unknown); break;
	}
}

Chip::Frequency Chip::frequency() const
{
	return m_frequency;
}

uint32_t Chip::freqValue() const
{
	switch (frequency())
	{
	case Frequency::F1000000: return 1000000;
	case Frequency::F1750000: return 1750000;
	case Frequency::F1773400: return 1773400;
	case Frequency::F2000000: return 2000000;
	}
	return 0; // should not happen!
}

bool Chip::frequencyKnown() const
{
	return (frequency() != Frequency::Unknown);
}

void Chip::channels(Channels channels)
{
	m_channels = channels;
}

Chip::Channels Chip::channels() const
{
	return m_channels;
}

bool Chip::channelsKnown() const
{
	return (channels() != Channels::Unknown);
}
