#pragma once

#include <set>
#include "glob.h"

class Filelist
{
	using FilePath = std::filesystem::path;
	using PathHash = std::size_t;

public:
	Filelist(const std::string& exts);
	Filelist(const std::string& exts, const FilePath& path);

public:
	bool     IsEmpty() const;
	uint32_t GetNumberOfFiles() const;
	int32_t  GetCurrFileIndex() const;

	bool GetPrevFile(FilePath& path) const;
	bool GetNextFile(FilePath& path) const;
	void RandomShuffle();

	bool InsertFile(const FilePath& path);
	bool ContainsFile(const FilePath& path);
	bool ExportPlaylist(const FilePath& path);

private:
	void ImportFolder(const FilePath& path);
	void ImportPlaylistM3U(const FilePath& path);
	void ImportPlaylistAYL(const FilePath& path);

	bool ExportPlaylistM3U(const FilePath& path);
	bool ExportPlaylistAYL(const FilePath& path);

private:
	std::vector<FilePath> m_exts;
	std::vector<FilePath> m_files;
	std::set<PathHash> m_hashes;
	mutable int32_t m_index;
};
