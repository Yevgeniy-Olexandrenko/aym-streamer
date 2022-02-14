#include <iomanip>
#include <sstream>
#include "Module.h"

////////////////////////////////////////////////////////////////////////////////

namespace
{
	const int k_prefDurationMM = 4;
	const int k_prefDurationSS = 0;
	const int k_maxExtraLoops  = 3;
}

Module::Module()
	: info(*this)
	, chip(*this)
	, frames(*this)
	, loop(*this)
	, playback(*this)
{
}

std::string Module::property(Property property) const
{
	auto PrintChipType = [](std::ostream& stream, ChipModel type)
	{
		switch (type)
		{
		case ChipModel::AY: stream << "AY-3-8910(12)"; break;
		case ChipModel::YM: stream << "YM2149F"; break;
		case ChipModel::Compatible: stream << "AY/YM Compatible"; break;
		}
	};

	std::stringstream stream;
	switch (property)
	{
	case Property::Title:
		stream << info.title();
		break;

	case Property::Artist:
		stream << info.artist();
		break;

	case Property::Type:
		stream << info.type();
		break;;

	case Property::Chip:
		if (chip.count() == ChipCount::TurboSound)
		{
			if (chip.modelKnown())
			{
				stream << "2 x ";
				PrintChipType(stream, chip.model());
			}
			else
			{
				stream << "Turbo Sound";
			}
			stream << ' ';
		}
		else if (chip.modelKnown())
		{
			PrintChipType(stream, chip.model());
			stream << ' ';
		}

		if (chip.frequencyKnown())
		{
			stream << double(chip.freqValue(0)) / 1000000 << " MHz" << ' ';
		}

		if (chip.channelsKnown())
		{
			if (chip.channels() == ChipChannels::MONO) stream << "MONO";
			if (chip.channels() == ChipChannels::ABC ) stream << "ABC";
			if (chip.channels() == ChipChannels::ACB ) stream << "ACB";
		}
		break;

	case Property::Frames:
		stream << frames.count();
		if (loop.available()) stream << " -> " << loop.frameId();
		stream << " @ " << playback.frameRate() << " Hz";
		break;

	case Property::Duration:
		int hh0 = 0, mm0 = 0, ss0 = 0, ms0 = 0;
		int hh1 = 0, mm1 = 0, ss1 = 0, ms1 = 0;
		playback.realDuration(hh0, mm0, ss0, ms0);
		playback.fakeDuration(hh1, mm1, ss1, ms1);

		stream <<
			std::setfill('0') << std::setw(2) << hh0 << ':' <<
			std::setfill('0') << std::setw(2) << mm0 << ':' <<
			std::setfill('0') << std::setw(2) << ss0;
		stream << '.' << std::setfill('0') << std::setw(3) << ms0;

		if (hh0 != hh1 || mm0 != mm1 || ss0 != ss1)
		{
			stream << " (";
			stream <<
				std::setfill('0') << std::setw(2) << hh1 << ':' <<
				std::setfill('0') << std::setw(2) << mm1 << ':' <<
				std::setfill('0') << std::setw(2) << ss1;
			stream << '.' << std::setfill('0') << std::setw(3) << ms1;
			stream << ')';
		}
		break;
	}
	return stream.str();
}

//std::ostream& operator<<(std::ostream& stream, const Module& module)
//{
//	int hh, mm, ss, ms;
//	
//	stream << "File........: " << module.file.nameExt() << std::endl;
//	if (module.info.titleKnown())
//		stream << "Title.......: " << module.info.title() << std::endl;
//	if (module.info.artistKnown()) 
//		stream << "Artist......: " << module.info.artist() << std::endl;
//	stream << "Type........: " << module.info.type() << std::endl;
//	
//	if (module.chip.config() == ChipConfig::TurboSound)
//	{
//		stream << "Chip config.: ";
//		if (module.chip.typeKnown())
//		{
//			stream << "2 x ";
//			if (module.chip.type() == ChipType::AY) stream << "AY-3-8910(12)";
//			if (module.chip.type() == ChipType::YM) stream << "YM2149F";
//		}
//		else
//		{
//			stream << "Turbo Sound";
//		}
//		stream << std::endl;
//	}
//	else if (module.chip.typeKnown())
//	{
//		stream << "Chip type...: ";
//		if (module.chip.type() == ChipType::AY) stream << "AY-3-8910(12)";
//		if (module.chip.type() == ChipType::YM) stream << "YM2149F";
//		stream << std::endl;;
//	}
//
//	if (module.chip.freqKnown())
//		stream << "Chip freq...: " << module.chip.freqValue(0) << " Hz" << std::endl;
//
//	if (module.chip.stereoKnown())
//	{
//		stream << "Chip stereo.: ";
//		if (module.chip.stereo() == ChipStereo::MONO) stream << "MONO";
//		if (module.chip.stereo() == ChipStereo::ABC ) stream << "ABC";
//		if (module.chip.stereo() == ChipStereo::ACB ) stream << "ACB";
//		stream << std::endl;
//	}
//
//	module.playback.rawDuration(hh, mm, ss, ms);
//	stream << "Duration....: " << 
//		std::setfill('0') << std::setw(2) << hh << ':' << 
//		std::setfill('0') << std::setw(2) << mm << ':' << 
//		std::setfill('0') << std::setw(2) << ss << '.' <<
//		std::setfill('0') << std::setw(3) << ms << std::endl;
//	
//	module.playback.duration(hh, mm, ss, ms);
//	stream << "Duration....: " <<
//		std::setfill('0') << std::setw(2) << hh << ':' <<
//		std::setfill('0') << std::setw(2) << mm << ':' <<
//		std::setfill('0') << std::setw(2) << ss << '.' <<
//		std::setfill('0') << std::setw(3) << ms << std::endl;
//
//	stream << "Frames......: " << module.frames.count();
//	if (module.loop.available())
//		stream << " -> " << module.loop.frameId();
//	stream << std::endl;
//
//	stream << "Frame rate..: " << module.playback.frameRate() << std::endl;
//	return stream;
//}

////////////////////////////////////////////////////////////////////////////////

Module::Info::Info(Module& module)
	: Delegate(module)
{
}

void Module::Info::title(const std::string& title)
{
	m_title = title;
}

const std::string& Module::Info::title() const
{
	return m_title;
}

bool Module::Info::titleKnown() const
{
	return !m_title.empty();
}

void Module::Info::artist(const std::string& artist)
{
	m_artist = artist;
}

const std::string& Module::Info::artist() const
{
	return m_artist;
}

bool Module::Info::artistKnown() const
{
	return !m_artist.empty();
}

void Module::Info::type(const std::string& type)
{
	m_type = type;
}

const std::string& Module::Info::type() const
{
	return m_type;
}

////////////////////////////////////////////////////////////////////////////////

Module::Chip::Chip(Module& module)
	: Delegate(module)
	, m_model(ChipModel::Unknown)
	, m_frequency(ChipFrequency::Unknown)
	, m_channels(ChipChannels::Unknown)
	, m_count(ChipCount::SingleChip)
{
}

void Module::Chip::count(ChipCount count)
{
	m_count = count;
}

ChipCount Module::Chip::count() const
{
	return m_count;
}

void Module::Chip::model(ChipModel model)
{
	m_model = model;
}

ChipModel Module::Chip::model() const
{
	return m_model;
}

bool Module::Chip::modelKnown() const
{
	return (model() != ChipModel::Unknown);
}

void Module::Chip::frequency(ChipFrequency freq)
{
	m_frequency = freq;
}

void Module::Chip::freqValue(uint32_t freqValue)
{
	switch (freqValue)
	{
	case 1000000: frequency(ChipFrequency::F1000000); break;
	case 1750000: frequency(ChipFrequency::F1750000); break;
	case 1773400: frequency(ChipFrequency::F1773400); break;
	case 2000000: frequency(ChipFrequency::F2000000); break;
	default: frequency(ChipFrequency::Unknown); break;
	}
}

ChipFrequency Module::Chip::frequency() const
{
	return m_frequency;
}

uint32_t Module::Chip::freqValue(uint32_t defFreqValue) const
{
	switch (frequency())
	{
	case ChipFrequency::F1000000: return 1000000;
	case ChipFrequency::F1750000: return 1750000;
	case ChipFrequency::F1773400: return 1773400;
	case ChipFrequency::F2000000: return 2000000;
	}
	return defFreqValue;
}

bool Module::Chip::frequencyKnown() const
{
	return (frequency() != ChipFrequency::Unknown);
}

void Module::Chip::channels(ChipChannels channels)
{
	m_channels = channels;
}

ChipChannels Module::Chip::channels() const
{
	return m_channels;
}

bool Module::Chip::channelsKnown() const
{
	return (channels() != ChipChannels::Unknown);
}

////////////////////////////////////////////////////////////////////////////////

Module::Frames::Frames(Module& module)
	: Delegate(module)
{
}

void Module::Frames::add(const Frame& frame)
{
	m_frames.push_back(frame);
}

const Frame& Module::Frames::get(FrameId id) const
{
	return m_frames[id];
}

uint32_t Module::Frames::count() const
{
	return (uint32_t)m_frames.size();
}

FrameId Module::Frames::lastFrameId() const
{
	return count() - 1;
}

bool Module::Frames::available() const
{
	return (count() > 0);
}

////////////////////////////////////////////////////////////////////////////////

Module::Loop::Loop(Module& module)
	: Delegate(module)
	, m_frameId(0)
	, m_extraLoops(0)
{
}

void Module::Loop::frameId(FrameId id)
{
	m_frameId = id;

	UpdateLoopFrameChanges();
	ComputeExtraLoops();
}

FrameId Module::Loop::frameId() const
{
	return (m_frameId ? m_frameId : m_module.frames.count());
}

uint32_t Module::Loop::framesCount() const
{
	return m_module.frames.count() - frameId();
}

int Module::Loop::extraLoops() const
{
	return m_extraLoops;
}

bool Module::Loop::available() const
{
	return (framesCount() > (m_module.frames.count() / 2));
}

void Module::Loop::ComputeExtraLoops()
{
	m_extraLoops = 0;
	if (available())
	{
		uint32_t frameCount = m_module.frames.count();
		uint32_t maxPlaybackFrames = (k_prefDurationMM * 60 + k_prefDurationSS) * m_module.playback.frameRate();
		if (maxPlaybackFrames > frameCount)
		{
			m_extraLoops = (maxPlaybackFrames - frameCount) / framesCount();
			m_extraLoops = std::min(m_extraLoops, k_maxExtraLoops);
		}
	}
}

void Module::Loop::UpdateLoopFrameChanges()
{
	if (available())
	{
		uint8_t loopFrameData, lastFrameData;
		const Frame& lastFrame = m_module.frames.get(m_module.frames.lastFrameId());
		Frame& loopFrame = const_cast<Frame&>(m_module.frames.get(frameId()));

		for (uint8_t i = 0; i < 16; ++i)
		{
			if (!loopFrame.changed(0, i))
			{
				loopFrameData = loopFrame.data(0, i);
				lastFrameData = lastFrame.data(0, i);

				if (loopFrameData != lastFrameData)
					loopFrame[i].first.override(loopFrameData);
			}

			if (!loopFrame.changed(1, i))
			{
				loopFrameData = loopFrame.data(1, i);
				lastFrameData = lastFrame.data(1, i);

				if (loopFrameData != lastFrameData)
					loopFrame[i].second.override(loopFrameData);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

Module::Playback::Playback(Module& module)
	: Delegate(module)
	, m_frameRate(0)
{
}

const Frame& Module::Playback::getFrame(FrameId id) const
{
	uint32_t frameCount = m_module.frames.count();
	if (id >= frameCount)
	{
		id = m_module.loop.frameId() + ((id - frameCount) % m_module.loop.framesCount());
	}
	return m_module.frames.get(id);
}

uint32_t Module::Playback::framesCount() const
{
	return m_module.frames.count() + m_module.loop.extraLoops() * m_module.loop.framesCount();
}

FrameId Module::Playback::lastFrameId() const
{
	return framesCount() - 1;
}

void Module::Playback::frameRate(FrameRate frameRate)
{
	m_frameRate = frameRate;
}

FrameRate Module::Playback::frameRate() const
{
	return m_frameRate;
}

void Module::Playback::realDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss, ms);
}

void Module::Playback::realDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss);
}

void Module::Playback::fakeDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(framesCount(), hh, mm, ss, ms);
}

void Module::Playback::fakeDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(framesCount(), hh, mm, ss);
}

void Module::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const
{
	uint32_t duration = (frameCount * 1000) / frameRate();
	ms = duration % 1000; duration /= 1000;
	ss = duration % 60;   duration /= 60;
	mm = duration % 60;   duration /= 60;
	hh = duration;
}

void Module::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const
{
	uint32_t duration = (((frameCount * 1000) / frameRate()) + 500) / 1000;
	ss = duration % 60; duration /= 60;
	mm = duration % 60; duration /= 60;
	hh = duration;
}

////////////////////////////////////////////////////////////////////////////////
