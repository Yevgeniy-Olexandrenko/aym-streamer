#include <sstream>
#include "Filepath.h"

Filepath::Filepath()
{
}

Filepath::Filepath(const std::string& path)
{
	pathNameExt(path);
}

void Filepath::pathNameExt(const std::string& path)
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

const std::string Filepath::pathNameExt() const
{
	std::stringstream ss;
	if (!m_folder.empty()) ss << m_folder;
	ss << nameExt();
	return ss.str();
}

const std::string Filepath::nameExt() const
{
	std::stringstream ss;
	ss << m_name;
	if (!m_ext.empty()) ss << '.' << m_ext;
	return ss.str();
}

const std::string Filepath::name() const
{
	return m_name;
}

const std::string Filepath::ext() const
{
	return m_ext;
}

