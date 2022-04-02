#pragma once

#include <set>
#include <glob/glob.h>

class Filelist
{
	using PathHash = std::size_t;

public:
	Filelist(const std::string& exts);
	Filelist(const std::string& exts, const std::filesystem::path& path);

public:
	bool empty() const;
	uint32_t count() const;
	int32_t index() const;

	bool prev(std::filesystem::path& path) const;
	bool next(std::filesystem::path& path) const;
	void shuffle();

private:
	void ParsePlaylistM3U(const std::filesystem::path& path);
	void ParsePlaylistAYL(const std::filesystem::path& path);
	void ParseFolder(const std::filesystem::path& path);
	void InsertPath(const std::filesystem::path& path);

private:
	std::vector<std::filesystem::path> m_exts;
	std::vector<std::filesystem::path> m_files;
	std::set<PathHash> m_hashes;
	mutable int32_t m_index;
};
