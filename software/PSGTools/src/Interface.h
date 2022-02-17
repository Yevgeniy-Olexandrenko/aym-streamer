#pragma once

#include <terminal/terminal.hpp>
#include "module/Module.h"
#include "output/Output.h"

namespace Interface
{
	struct sKeyState
	{
		bool bPressed;
		bool bReleased;
		bool bHeld;
	};

	sKeyState GetKey(int nKeyID);
	void HandleKeyboardInput();

	void PrintInputFile(const Module& module, int index, int total);
	void PrintPlaybackFooter();
	void PrintModuleInfo(const Module& module, const Output& output);
	void PrintFrameStream(const Module& module, FrameId frameId, size_t height);
	void PrintBlankArea(int offset, size_t height);
}