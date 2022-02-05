#include <sstream>
#include <iomanip>
#include "Module.h"

Module::Module()
	: m_frameRate(0)
	, m_loopFrameId(0)
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

void Module::SetFrameRate(FrameRate frameRate)
{
	m_frameRate = frameRate;
}

Module::FrameRate Module::GetFrameRate() const
{
	return m_frameRate;
}

void Module::AddFrame(const Frame& frame)
{
	m_frames.push_back(frame);
}

const Frame& Module::GetFrame(FrameId id) const
{
	return m_frames[id];
}

uint32_t Module::GetFrameCount() const
{
	return (uint32_t)m_frames.size();
}

void Module::SetLoopUnavailable()
{
	SetLoopFrameId(GetFrameCount());
}

void Module::SetLoopFrameId(FrameId id)
{
	m_loopFrameId = id;
}

Module::FrameId Module::GetLoopFrameId() const
{
	return m_loopFrameId;
}

uint32_t Module::GetLoopFrameCount() const
{
	return GetFrameCount() - GetLoopFrameId();
}

bool Module::HasLoop() const
{
	return (GetLoopFrameCount() > 0);
}

void Module::GetDuration(int& hh, int& mm, int& ss, int& ms) const
{
	int duration = 1000 * GetFrameCount() / GetFrameRate();
	ms = duration % 1000; duration /= 1000;
	ss = duration % 60; duration /= 60;
	mm = duration % 60; duration /= 60;
	hh = duration;
}

std::ostream& operator<<(std::ostream& stream, const Module& module)
{
	int hh, mm, ss, ms;
	module.GetDuration(hh, mm, ss, ms);

	stream << "File........: " << module.GetFileName() << std::endl;
	if (module.HasTitle())stream << "Title.......: " << module.GetTitle() << std::endl;
	if (module.HasArtist()) stream << "Artist......: " << module.GetArtist() << std::endl;
	stream << "Type........: " << module.GetType() << std::endl;
	stream << "Duration....: " << 
		std::setfill('0') << std::setw(2) << hh << ':' << 
		std::setfill('0') << std::setw(2) << mm << ':' << 
		std::setfill('0') << std::setw(2) << ss << std::endl;
	stream << "Frames count: " << module.GetFrameCount() << std::endl;
	if (module.HasLoop()) stream << "Loop frame..: " << module.GetLoopFrameId() << std::endl;
	stream << "Frame rate..: " << module.GetFrameRate() << std::endl;
	return stream;
}
