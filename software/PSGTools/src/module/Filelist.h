#pragma once

#include <string>
#include <vector>

class Filelist
{
public:
	Filelist(const std::string& exts);
	Filelist(const std::string& exts, const std::string& path);

private:
	std::vector<std::string> m_exts;
	std::vector<std::string> m_files;
};
