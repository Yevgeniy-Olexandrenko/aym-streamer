#pragma once

#include <terminal/terminal.hpp>
#include "module/Module.h"
#include "output/Output.h"

namespace Interface
{
	void PrintInputFile(const Module& module, int number, int total);
	void PrintModuleInfo(const Module& module, const Output& output);
	void PrintFrameStream(const Module& module, FrameId frameId, size_t height);
	void PrintBlankArea(int offset, size_t height);
}