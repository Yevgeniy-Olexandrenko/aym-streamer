#pragma once

#include <string>

class Filepath
{
public:
	Filepath();
	Filepath(const std::string& path);

	void pathNameExt(const std::string& path);
	const std::string pathNameExt() const;
	const std::string nameExt() const;
	const std::string name() const;
	const std::string ext() const;

private:
	std::string m_folder;
	std::string m_name;
	std::string m_ext;
};