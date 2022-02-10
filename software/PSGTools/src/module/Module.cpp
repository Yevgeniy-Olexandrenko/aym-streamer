#include <sstream>
#include <iomanip>
#include "Module.h"

namespace
{
	const int k_prefDurationMM = 3;
	const int k_prefDurationSS = 30;
	const int k_maxExtraLoops  = 4;
}

Module::Module()
	: m_chipFreq(ChipFreq::Unknown)
	, m_frameRate(0)
	, m_loopFrameId(0)
	, m_extraLoops(0)
{
}

void Module::SetFilePath(const std::string& filePath)
{
	size_t slashPos = filePath.find_last_of("/\\");
	size_t dotPos = filePath.find_last_of(".");

	if (slashPos != std::string::npos)
	{
		m_file.m_folder = filePath.substr(0, slashPos + 1);
	}
	else
	{
		m_file.m_folder.clear();
		slashPos = -1;
	}

	if (dotPos != std::string::npos)
	{
		m_file.m_ext = filePath.substr(dotPos + 1);
	}
	else
	{
		m_file.m_ext.clear();
		dotPos = filePath.length();
	}

	size_t namePos = slashPos + 1;
	m_file.m_name = filePath.substr(namePos, dotPos - namePos);
}

const std::string Module::GetFilePath() const
{
	std::stringstream ss;
	if (!m_file.m_folder.empty())
	{
		ss << m_file.m_folder;
	}

	ss << GetFileName();
	return ss.str();
}

const std::string Module::GetFileName() const
{
	std::stringstream ss;
	ss << m_file.m_name;

	if (!m_file.m_ext.empty())
	{
		ss << '.' << m_file.m_ext;
	}
	return ss.str();
}

const std::string Module::GetFileExt() const
{
	return m_file.m_ext;
}

void Module::SetTitle(const std::string& title)
{
	m_title = title;
}

const std::string& Module::GetTitle() const
{
	return m_title;
}

bool Module::HasTitle() const
{
	return !m_title.empty();
}

void Module::SetArtist(const std::string& artist)
{
	m_artist = artist;
}

const std::string& Module::GetArtist() const
{
	return m_artist;
}

bool Module::HasArtist() const
{
	return !m_artist.empty();
}

void Module::SetType(const std::string& type)
{
	m_type = type;
}

const std::string& Module::GetType() const
{
	return m_type;
}

////////////////////////////////////////////////////////////////////////////////

void Module::SetChipType(ChipType chipType)
{
	m_chipType = chipType;
}

ChipType Module::GetChipType() const
{
	return m_chipType;
}

bool Module::HasChipType() const
{
	return (GetChipType() != ChipType::Unknown);
}

void Module::SetChipFreq(ChipFreq chipFreq)
{
	m_chipFreq = chipFreq;
}

void Module::SetChipFreqValue(uint32_t chipFreqValue)
{
	switch (chipFreqValue)
	{
	case 1000000: SetChipFreq(ChipFreq::F1000000); break;
	case 1750000: SetChipFreq(ChipFreq::F1750000); break;
	case 1773400: SetChipFreq(ChipFreq::F1773400); break;
	case 2000000: SetChipFreq(ChipFreq::F2000000); break;
	default: SetChipFreq(ChipFreq::Unknown); break;
	}
}

ChipFreq Module::GetChipFreq() const
{
	return m_chipFreq;
}

uint32_t Module::GetChipFreqValue(uint32_t defaultClockRate) const
{
	switch (GetChipFreq())
	{
	case ChipFreq::F1000000: return 1000000;
	case ChipFreq::F1750000: return 1750000;
	case ChipFreq::F1773400: return 1773400;
	case ChipFreq::F2000000: return 2000000;
	}
	return defaultClockRate;
}

bool Module::HasChipFreq() const
{
	return (GetChipFreq() != ChipFreq::Unknown);
}

void Module::SetChipStereo(ChipStereo chipStereo)
{
	m_chipStereo = chipStereo;
}

ChipStereo Module::GetChipStereo() const
{
	return m_chipStereo;
}

bool Module::HasChipStereo() const
{
	return (GetChipStereo() != ChipStereo::Unknown);
}

void Module::SetFrameRate(FrameRate frameRate)
{
	m_frameRate = frameRate;
}

FrameRate Module::GetFrameRate() const
{
	return m_frameRate;
}

////////////////////////////////////////////////////////////////////////////////

void Module::AddFrame(const Frame& frame)
{
	m_frames.push_back(frame);
}

const Frame& Module::GetFrame(FrameId id) const
{
	return m_frames[id];
}

FrameId Module::GetLastFrameId() const
{
	return GetFrameCount() - 1;
}

uint32_t Module::GetFrameCount() const
{
	return (uint32_t)m_frames.size();
}

void Module::GetDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(GetFrameCount(), hh, mm, ss, ms);
}

void Module::GetDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(GetFrameCount(), hh, mm, ss);
}

////////////////////////////////////////////////////////////////////////////////

void Module::SetLoopFrameId(FrameId id)
{
	m_loopFrameId = id;

	UpdateLoopFrameChanges();
	ComputeExtraLoops();
}

FrameId Module::GetLoopFrameId() const
{
	return (m_loopFrameId ? m_loopFrameId : GetFrameCount());
}

uint32_t Module::GetLoopFrameCount() const
{
	return GetFrameCount() - GetLoopFrameId();
}

bool Module::HasLoop() const
{
	return (GetLoopFrameCount() > GetFrameCount() / 2);
}

////////////////////////////////////////////////////////////////////////////////

const Frame& Module::GetPlaybackFrame(FrameId id) const
{
	uint32_t frameCount = GetFrameCount();
	if (id >= frameCount)
	{
		id = GetLoopFrameId() + ((id - frameCount) % GetLoopFrameCount());
	}
	return GetFrame(id);
}

FrameId Module::GetPlaybackLastFrameId() const
{
	return GetPlaybackFrameCount() - 1;
}

uint32_t Module::GetPlaybackFrameCount() const
{
	return GetFrameCount() + m_extraLoops * GetLoopFrameCount();
}

void Module::GetPlaybackDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(GetPlaybackFrameCount(), hh, mm, ss, ms);
}

void Module::GetPlaybackDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(GetPlaybackFrameCount(), hh, mm, ss);
}

////////////////////////////////////////////////////////////////////////////////

void Module::ComputeExtraLoops()
{
	m_extraLoops = 0;
	if (HasLoop())
	{
		uint32_t frameCount = GetFrameCount();
		uint32_t maxPlaybackFrames = (k_prefDurationMM * 60 + k_prefDurationSS) * GetFrameRate();
		if (maxPlaybackFrames > frameCount)
		{
			m_extraLoops = (maxPlaybackFrames - frameCount) / GetLoopFrameCount();
			m_extraLoops = std::min(m_extraLoops, k_maxExtraLoops);
		}
	}
}

void Module::UpdateLoopFrameChanges()
{
	if (HasLoop())
	{
		const Frame& lastFrame = GetFrame(GetLastFrameId());
		Frame& loopFrame = const_cast<Frame&>(GetFrame(GetLoopFrameId()));

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

void Module::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const
{
	uint32_t duration = (frameCount * 1000) / GetFrameRate();
	ms = duration % 1000; duration /= 1000;
	ss = duration % 60;   duration /= 60;
	mm = duration % 60;   duration /= 60;
	hh = duration;
}

void Module::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const
{
	uint32_t duration = (((frameCount * 1000) / GetFrameRate()) + 500) / 1000;
	ss = duration % 60; duration /= 60;
	mm = duration % 60; duration /= 60;
	hh = duration;
}

std::ostream& operator<<(std::ostream& stream, const Module& module)
{
	int hh, mm, ss, ms;
	
	stream << "File........: " << module.GetFileName() << std::endl;
	if (module.HasTitle())
		stream << "Title.......: " << module.GetTitle() << std::endl;
	if (module.HasArtist()) 
		stream << "Artist......: " << module.GetArtist() << std::endl;
	stream << "Type........: " << module.GetType() << std::endl;
	if (module.HasChipType())
	{
		stream << "Chip type...: ";
		if (module.GetChipType() == ChipType::AY) stream << "AY-3-8910(12)";
		if (module.GetChipType() == ChipType::YM) stream << "YM2149F";
		stream << std::endl;
	}
	if (module.HasChipFreq()) 
		stream << "Chip freq...: " << module.GetChipFreqValue(0) << " Hz" << std::endl;

	if (module.HasChipStereo())
	{
		stream << "Chip stereo.: ";
		if (module.GetChipStereo() == ChipStereo::MONO) stream << "MONO";
		if (module.GetChipStereo() == ChipStereo::ABC ) stream << "ABC";
		if (module.GetChipStereo() == ChipStereo::ACB ) stream << "ACB";
		stream << std::endl;
	}

	module.GetDuration(hh, mm, ss, ms);
	stream << "Duration....: " << 
		std::setfill('0') << std::setw(2) << hh << ':' << 
		std::setfill('0') << std::setw(2) << mm << ':' << 
		std::setfill('0') << std::setw(2) << ss << '.' <<
		std::setfill('0') << std::setw(3) << ms << std::endl;
	
	module.GetPlaybackDuration(hh, mm, ss, ms);
	stream << "Duration....: " <<
		std::setfill('0') << std::setw(2) << hh << ':' <<
		std::setfill('0') << std::setw(2) << mm << ':' <<
		std::setfill('0') << std::setw(2) << ss << '.' <<
		std::setfill('0') << std::setw(3) << ms << std::endl;

	stream << "Frames count: " << module.GetFrameCount() << std::endl;
	if (module.HasLoop()) 
		stream << "Loop frame..: " << module.GetLoopFrameId() << std::endl;
	stream << "Frame rate..: " << module.GetFrameRate() << std::endl;
	return stream;
}
