#include <sstream>
#include "Chip.h"

namespace
{
	const int k_clocks[] = { 0, 1000000, 1750000, 1773400, 2000000 };
}

Chip::Chip()
	: m_clock(Clock::Unknown)
	, m_output(Output::Unknown)
	, m_stereo(Stereo::Unknown)
{
	first.model(Model::Compatible);
	second.model(Model::Unknown);
}

std::string Chip::toString() const
{
	const auto OutputModel = [](std::ostream& stream, Model type)
	{
		switch (type)
		{
		case Model::AY8910:	    stream << "AY-3-8910";        break;
		case Model::YM2149:     stream << "YM2149F";          break;
		case Model::AY8930:     stream << "AY8930";           break;
		case Model::Compatible: stream << "AY/YM Compatible"; break;
		}
	};

	std::stringstream stream;
	if (count() == 2)
	{
		if (first.model() == second.model())
		{
			stream << "2 x ";
			OutputModel(stream, first.model());
		}
		else
		{
			OutputModel(stream, first.model());
			stream << " + ";
			OutputModel(stream, second.model());
		}
		stream << ' ';
	}
	else
	{
		OutputModel(stream, first.model());
		stream << ' ';
	}

	if (clockKnown())
	{
		stream << double(clockValue()) / 1000000 << " MHz" << ' ';
	}

	if (outputKnown())
	{
		if (output() == Output::Mono)
		{
			stream << "Mono";
		}
		else if (stereoKnown())
		{
			switch (stereo())
			{
			case Stereo::ABC: stream << "ABC"; break;
			case Stereo::ACB: stream << "ACB"; break;
			case Stereo::BAC: stream << "BAC"; break;
			case Stereo::BCA: stream << "BCA"; break;
			case Stereo::CAB: stream << "CAB"; break;
			case Stereo::CBA: stream << "CBA"; break;
			}
		}
		else
		{
			stream << "Stereo";
		}
	}

	return stream.str();
}

const Chip::Model& Chip::model(int index) const
{
	return (index ? second.model() : first.model());
}

void Chip::F::model(const Model& model)
{
	m_model = (model == Model::Unknown ? Model::Compatible : model);
}

bool Chip::S::modelKnown() const
{
	return (model() != Model::Unknown);
}

int Chip::count() const
{
	return (second.modelKnown() ? 2 : 1);
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

bool Chip::clockKnown() const
{
	return (clock() != Clock::Unknown);
}

bool Chip::outputKnown() const
{
	return (output() != Output::Unknown);
}

bool Chip::stereoKnown() const
{
	return (stereo() != Stereo::Unknown);
}
