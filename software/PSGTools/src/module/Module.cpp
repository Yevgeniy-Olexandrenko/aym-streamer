#include <sstream>
#include <iomanip>
#include "Module.h"

////////////////////////////////////////////////////////////////////////////////

namespace
{
	const int k_prefDurationMM = 4;
	const int k_prefDurationSS = 0;
	const int k_maxExtraLoops  = 3;
}

Module::Module()
	: file(*this)
	, info(*this)
	, chip(*this)
	, frames(*this)
	, loop(*this)
	, playback(*this)
{
}

std::ostream& operator<<(std::ostream& stream, const Module& module)
{
	int hh, mm, ss, ms;
	
	stream << "File........: " << module.file.nameExt() << std::endl;
	if (module.info.titleKnown())
		stream << "Title.......: " << module.info.title() << std::endl;
	if (module.info.artistKnown()) 
		stream << "Artist......: " << module.info.artist() << std::endl;
	stream << "Type........: " << module.info.type() << std::endl;
	
	if (module.chip.config() == ChipConfig::TurboSound)
	{
		stream << "Chip config.: ";
		if (module.chip.typeKnown())
		{
			stream << "2 x ";
			if (module.chip.type() == ChipType::AY) stream << "AY-3-8910(12)";
			if (module.chip.type() == ChipType::YM) stream << "YM2149F";
		}
		else
		{
			stream << "Turbo Sound";
		}
		stream << std::endl;
	}
	else if (module.chip.typeKnown())
	{
		stream << "Chip type...: ";
		if (module.chip.type() == ChipType::AY) stream << "AY-3-8910(12)";
		if (module.chip.type() == ChipType::YM) stream << "YM2149F";
		stream << std::endl;;
	}

	if (module.chip.freqKnown())
		stream << "Chip freq...: " << module.chip.freqValue(0) << " Hz" << std::endl;

	if (module.chip.stereoKnown())
	{
		stream << "Chip stereo.: ";
		if (module.chip.stereo() == ChipStereo::MONO) stream << "MONO";
		if (module.chip.stereo() == ChipStereo::ABC ) stream << "ABC";
		if (module.chip.stereo() == ChipStereo::ACB ) stream << "ACB";
		stream << std::endl;
	}

	module.playback.rawDuration(hh, mm, ss, ms);
	stream << "Duration....: " << 
		std::setfill('0') << std::setw(2) << hh << ':' << 
		std::setfill('0') << std::setw(2) << mm << ':' << 
		std::setfill('0') << std::setw(2) << ss << '.' <<
		std::setfill('0') << std::setw(3) << ms << std::endl;
	
	module.playback.duration(hh, mm, ss, ms);
	stream << "Duration....: " <<
		std::setfill('0') << std::setw(2) << hh << ':' <<
		std::setfill('0') << std::setw(2) << mm << ':' <<
		std::setfill('0') << std::setw(2) << ss << '.' <<
		std::setfill('0') << std::setw(3) << ms << std::endl;

	stream << "Frames count: " << module.frames.count() << std::endl;
	if (module.loop.available()) 
		stream << "Loop frame..: " << module.loop.frameId() << std::endl;
	stream << "Frame rate..: " << module.playback.frameRate() << std::endl;
	return stream;
}

////////////////////////////////////////////////////////////////////////////////

Module::File::File(Module& module)
	: Delegate(module)
{
}

void Module::File::pathNameExt(const std::string& path)
{
	size_t slashPos = path.find_last_of("/\\");
	size_t dotPos = path.find_last_of(".");

	if (slashPos != std::string::npos)
	{
		m_folder = path.substr(0, slashPos + 1);
	}
	else
	{
		m_folder.clear();
		slashPos = -1;
	}

	if (dotPos != std::string::npos)
	{
		m_ext = path.substr(dotPos + 1);
	}
	else
	{
		m_ext.clear();
		dotPos = path.length();
	}

	size_t namePos = slashPos + 1;
	m_name = path.substr(namePos, dotPos - namePos);
}

const std::string Module::File::pathNameExt() const
{
	std::stringstream ss;
	if (!m_folder.empty()) ss << m_folder;
	ss << nameExt();
	return ss.str();
}

const std::string Module::File::nameExt() const
{
	std::stringstream ss;
	ss << m_name;
	if (!m_ext.empty()) ss << '.' << m_ext;
	return ss.str();
}

const std::string Module::File::name() const
{
	return m_name;
}

const std::string Module::File::ext() const
{
	return m_ext;
}

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
	, m_type(ChipType::Unknown)
	, m_freq(ChipFreq::Unknown)
	, m_stereo(ChipStereo::Unknown)
	, m_config(ChipConfig::SingleChip)
{
}

void Module::Chip::type(ChipType type)
{
	m_type = type;
}

ChipType Module::Chip::type() const
{
	return m_type;
}

bool Module::Chip::typeKnown() const
{
	return (type() != ChipType::Unknown);
}

void Module::Chip::freq(ChipFreq freq)
{
	m_freq = freq;
}

void Module::Chip::freqValue(uint32_t freqValue)
{
	switch (freqValue)
	{
	case 1000000: freq(ChipFreq::F1000000); break;
	case 1750000: freq(ChipFreq::F1750000); break;
	case 1773400: freq(ChipFreq::F1773400); break;
	case 2000000: freq(ChipFreq::F2000000); break;
	default: freq(ChipFreq::Unknown); break;
	}
}

ChipFreq Module::Chip::freq() const
{
	return m_freq;
}

uint32_t Module::Chip::freqValue(uint32_t defFreqValue) const
{
	switch (freq())
	{
	case ChipFreq::F1000000: return 1000000;
	case ChipFreq::F1750000: return 1750000;
	case ChipFreq::F1773400: return 1773400;
	case ChipFreq::F2000000: return 2000000;
	}
	return defFreqValue;
}

bool Module::Chip::freqKnown() const
{
	return (freq() != ChipFreq::Unknown);
}

void Module::Chip::stereo(ChipStereo stereo)
{
	m_stereo = stereo;
}

ChipStereo Module::Chip::stereo() const
{
	return m_stereo;
}

bool Module::Chip::stereoKnown() const
{
	return (stereo() != ChipStereo::Unknown);
}

void Module::Chip::config(ChipConfig config)
{
	m_config = config;
}

ChipConfig Module::Chip::config() const
{
	return m_config;
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
		const Frame& lastFrame = m_module.frames.get(m_module.frames.lastFrameId());
		Frame& loopFrame = const_cast<Frame&>(m_module.frames.get(frameId()));

		for (uint8_t i = 0; i < 16; ++i)
		{
			if (loopFrame[i].IsChanged()) continue;

			uint8_t loopFrameData = loopFrame[i].GetData();
			uint8_t lastFrameData = lastFrame[i].GetData();

			if (loopFrameData != lastFrameData)
				loopFrame[i].OverrideData(loopFrameData);
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

void Module::Playback::rawDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss, ms);
}

void Module::Playback::rawDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss);
}

void Module::Playback::duration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(framesCount(), hh, mm, ss, ms);
}

void Module::Playback::duration(int& hh, int& mm, int& ss) const
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
