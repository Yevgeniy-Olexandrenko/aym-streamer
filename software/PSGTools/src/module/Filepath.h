#pragma once

#include <string>

class Filepath
{
public:
	Filepath();
	Filepath(const std::string& path);

	void dirNameExt(const std::string& path);
	const std::string dirNameExt() const;
	const std::string nameExt() const;
	bool hasNameExt() const;

	void dir(const std::string& dir);
	const std::string& dir() const;
	bool hasDir() const;

	void name(const std::string& name);
	const std::string& name() const;
	bool hasName() const;

	void ext(const std::string& ext);
	const std::string& ext() const;
	bool hasExt() const;

private:
	std::string m_dir;
	std::string m_name;
	std::string m_ext;
};