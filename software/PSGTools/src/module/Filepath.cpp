#include <sstream>
#include <algorithm>
#include "Filepath.h"

Filepath::Filepath()
{
}

Filepath::Filepath(const std::string& path)
{
	dirNameExt(path);
}

void Filepath::dirNameExt(const std::string& path)
{
	size_t slashPos = path.find_last_of("/\\");
	size_t dotPos = path.find_last_of(".");

	if (slashPos != std::string::npos)
	{
		m_dir = path.substr(0, slashPos + 1);
	}
	else
	{
		m_dir.clear();
		slashPos = -1;
	}

	if (dotPos != std::string::npos)
	{
		ext(path.substr(dotPos + 1));
	}
	else
	{
		m_ext.clear();
		dotPos = path.length();
	}

	size_t namePos = slashPos + 1;
	name(path.substr(namePos, dotPos - namePos));
}

const std::string Filepath::dirNameExt() const
{
	std::stringstream ss;
	if (hasDir()) ss << dir();
	ss << nameExt();
	return ss.str();
}

const std::string Filepath::nameExt() const
{
	std::stringstream ss;
	ss << name();
	if (hasExt()) ss << '.' << ext();
	return ss.str();
}

bool Filepath::hasNameExt() const
{
	return !nameExt().empty();
}

void Filepath::dir(const std::string& dir)
{
	m_dir = dir;
	size_t slashPos = dir.find_last_of("/\\");
	if (slashPos != dir.size() - 1) m_dir += '\\';
}

const std::string& Filepath::dir() const
{
	return m_dir;
}

bool Filepath::hasDir() const
{
	return !m_dir.empty();
}

void Filepath::name(const std::string& name)
{
	m_name = name;
}

const std::string& Filepath::name() const
{
	return m_name;
}

bool Filepath::hasName() const
{
	return !m_name.empty();
}

void Filepath::ext(const std::string& ext)
{
	m_ext = ext;
	std::transform(
		m_ext.begin(), m_ext.end(),
		m_ext.begin(), [](unsigned char c) { return std::tolower(c); });
}

const std::string& Filepath::ext() const
{
	return m_ext;
}

bool Filepath::hasExt() const
{
	return !m_ext.empty();
}

