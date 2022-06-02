#include <sstream>
#include "Chip.h"

namespace
{
	const int frequencies[] = { 0, 1000000, 1750000, 1773400, 2000000 };
}

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

void Chip::freqValue(const uint32_t& freqValue)
{
	auto dist = [&](int i)
	{
		return std::abs(int(freqValue) - frequencies[i]);
	};

	int f = 0;
	for (int i = 1; i < 5; ++i)
	{
		if (dist(i) < dist(f)) f = i;
	}
	frequency(Frequency(f));
}

uint32_t Chip::freqValue() const
{
	size_t f = size_t(frequency());
	return frequencies[f];
}

bool Chip::modelKnown() const
{
	return (model() != Model::Unknown);
}

bool Chip::frequencyKnown() const
{
	return (frequency() != Frequency::Unknown);
}

bool Chip::channelsKnown() const
{
	return (channels() != Channels::Unknown);
}
