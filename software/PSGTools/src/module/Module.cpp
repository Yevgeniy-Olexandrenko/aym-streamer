#include <sstream>
#include "Module.h"

Module::Module()
	: m_frameRate(0)
	, m_loopFrameIndex(0)
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

const Frame& Module::GetFrame(FrameIndex index) const
{
	return m_frames[index];
}

size_t Module::GetFrameCount() const
{
	return m_frames.size();
}

void Module::SetLoopFrameUnavailable()
{
	SetLoopFrameIndex(GetFrameCount());
}

void Module::SetLoopFrameIndex(FrameIndex index)
{
	m_loopFrameIndex = index;
}

bool Module::IsLoopFrameAvailable() const
{
	return (GetLoopFrameIndex() < GetFrameCount());
}

Module::FrameIndex Module::GetLoopFrameIndex() const
{
	return m_loopFrameIndex;
}
