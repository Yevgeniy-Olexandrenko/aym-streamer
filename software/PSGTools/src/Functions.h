#pragma once

#include <filesystem>

class Stream;

namespace Functions
{
	bool DecodeFile(const std::filesystem::path& path, Stream& stream);
	bool EncodeFile(const std::filesystem::path& path, Stream& stream);
}