#pragma once

#include <filesystem>
#include "stream/Stream.h"
#include "stream/Player.h"
#include "output/Emulator/Emulator.h"
#include "output/Streamer/Streamer.h"

namespace PSG
{
	bool Decode(const std::filesystem::path& path, Stream& stream);
	bool Encode(const std::filesystem::path& path, Stream& stream);
}
