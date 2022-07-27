#include <sstream>
#include "Chip.h"

namespace
{
	const int k_clocks[] = { 0, 1000000, 1750000, 1773400, 2000000 };
}

Chip::Chip()
	: m_count(Count::OneChip)
	, m_model(Model::Unknown)
	, m_clock(Clock::Unknown)
	, m_channels(Channels::Unknown)
{
}

std::string Chip::toString() const
{
	auto PrintChipType = [](std::ostream& stream, Model type)
	{
		switch (type)
		{
		case Model::AY8910:
			stream << "AY-3-8910(12)";
			break;

		case Model::YM2149:
			stream << "YM2149F";
			break;

		case Model::AY8930:
			stream << "AY8930";
			break;

		case Model::Compatible:
			stream << "AY/YM Compatible";
			break;
		}
	};

	std::stringstream stream;
	if (count() == Count::TwoChips)
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

	if (clockKnown())
	{
		stream << double(clockValue()) / 1000000 << " MHz" << ' ';
	}

	if (channelsKnown())
	{
		if (channels() == Chip::Channels::MONO) stream << "MONO";
		if (channels() == Chip::Channels::ABC ) stream << "ABC";
		if (channels() == Chip::Channels::ACB ) stream << "ACB";
	}

	return stream.str();
}

int Chip::countValue() const
{
	return (count() == Chip::Count::TwoChips ? 2 : 1);
}

void Chip::clockValue(const int& clockValue)
{
	auto dist = [&](int i)
	{
		return std::abs(clockValue - k_clocks[i]);
	};

	int f = 0;
	for (int i = 1; i < 5; ++i)
	{
		if (dist(i) < dist(f)) f = i;
	}
	clock(Clock(f));
}

int Chip::clockValue() const
{
	return k_clocks[size_t(clock())];
}

bool Chip::modelKnown() const
{
	return (model() != Model::Unknown);
}

bool Chip::clockKnown() const
{
	return (clock() != Clock::Unknown);
}

bool Chip::channelsKnown() const
{
	return (channels() != Channels::Unknown);
}
