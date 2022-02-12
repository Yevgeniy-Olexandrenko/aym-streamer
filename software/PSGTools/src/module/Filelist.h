#pragma once

#include <string>
#include <vector>
#include "Filepath.h"

class Filelist
{
public:
	Filelist(const std::string& exts);
	Filelist(const std::string& exts, const std::string& path);

public:
	bool empty() const;
	bool prev(std::string& path) const;
	bool next(std::string& path) const;
	void shuffle();

private:
	bool IsSupportedExt(const Filepath& path);
	void ParsePlaylistM3U(const Filepath& path);
	void ParsePlaylistAYL(const Filepath& path);
	void ParseFolder(const Filepath& path);
	void InsertItem(const Filepath& path);

private:
	std::vector<std::string> m_exts;
	std::vector<Filepath> m_files;
	mutable int32_t m_index;
};
