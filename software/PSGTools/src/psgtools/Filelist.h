#pragma once

#include <set>
#include "glob.h"

class Filelist
{
	using PathHash = std::size_t;

public:
	Filelist(const std::string& exts);
	Filelist(const std::string& exts, const std::filesystem::path& path);

public:
	bool     IsEmpty() const;
	uint32_t GetNumberOfFiles() const;
	int32_t  GetCurrFileIndex() const;

	bool GetPrevFile(std::filesystem::path& path) const;
	bool GetNextFile(std::filesystem::path& path) const;
	void RandomShuffle();

	bool ExportPlaylist(const std::filesystem::path& path);

private:
	void ImportPlaylistM3U(const std::filesystem::path& path);
	void ImportPlaylistAYL(const std::filesystem::path& path);
	void ImportFolder(const std::filesystem::path& path);
	void ImportFile(const std::filesystem::path& path);

	bool ExportPlaylistM3U(const std::filesystem::path& path);
	bool ExportPlaylistAYL(const std::filesystem::path& path);

private:
	std::vector<std::filesystem::path> m_exts;
	std::vector<std::filesystem::path> m_files;
	std::set<PathHash> m_hashes;
	mutable int32_t m_index;
};
