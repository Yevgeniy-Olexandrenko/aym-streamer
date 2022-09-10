#pragma once

#include "terminal.hpp"
#include "KeyboardInput.h"
#include "stream/Stream.h"
#include "output/Output.h"

namespace gui
{
	const int k_consoleWidth  = 100;
	const int k_consoleHeight = 40;

	void Init(const std::wstring& title);
	void Update();

	void Clear(size_t height);
	void Clear(int offset, size_t height);
	const KeyState& GetKeyState(int key);

	size_t PrintInputFile(const Stream& stream, int index, int amount, bool isFavorite);
	size_t PrintStreamInfo(const Stream& stream, const Output& output);
	size_t PrintStreamFrames(const Stream& stream, int frameId, const Output::Enables& enables);
	size_t PrintPlaybackProgress(const Stream& stream, int frameId);
}
