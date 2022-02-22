#pragma once

#include <terminal/terminal.hpp>
#include "module/Module.h"
#include "output/Output.h"

namespace Interface
{
	const int k_consoleWidth  = 87;
	const int k_consoleHeight = 32;

	void Init();
	bool KeepSize();

	// -------------------------------------------------------------------------

	enum COLOUR
	{
		FG_BLACK = 0x0000,
		FG_DARK_BLUE = 0x0001,
		FG_DARK_GREEN = 0x0002,
		FG_DARK_CYAN = 0x0003,
		FG_DARK_RED = 0x0004,
		FG_DARK_MAGENTA = 0x0005,
		FG_DARK_YELLOW = 0x0006,
		FG_GREY = 0x0007,

		FG_DARK_GREY = 0x0008,
		FG_BLUE = 0x0009,
		FG_GREEN = 0x000A,
		FG_CYAN = 0x000B,
		FG_RED = 0x000C,
		FG_MAGENTA = 0x000D,
		FG_YELLOW = 0x000E,
		FG_WHITE = 0x000F,

		BG_BLACK = 0x0000,
		BG_DARK_BLUE = 0x0010,
		BG_DARK_GREEN = 0x0020,
		BG_DARK_CYAN = 0x0030,
		BG_DARK_RED = 0x0040,
		BG_DARK_MAGENTA = 0x0050,
		BG_DARK_YELLOW = 0x0060,
		BG_GREY = 0x0070,

		BG_DARK_GREY = 0x0080,
		BG_BLUE = 0x0090,
		BG_GREEN = 0x00A0,
		BG_CYAN = 0x00B0,
		BG_RED = 0x00C0,
		BG_MAGENTA = 0x00D0,
		BG_YELLOW = 0x00E0,
		BG_WHITE = 0x00F0,
	};

	class PrintBuffer
	{
	public:
		PrintBuffer(int w, int h);
		~PrintBuffer();

		void clear();
		void render();

	public:
		PrintBuffer& position(int x, int y);
		PrintBuffer& move(int dx, int dy);
		PrintBuffer& color(SHORT color);
		
		PrintBuffer& draw(std::wstring s);
		PrintBuffer& draw(std::string s);
		PrintBuffer& draw(wchar_t c);
		PrintBuffer& draw(char c);
		
	public:
		SHORT x, y, w, h;
		CHAR_INFO* buffer;
		SHORT col;
	};

	// -------------------------------------------------------------------------

	struct KeyState
	{
		bool pressed;
		bool released;
		bool held;
	};

	KeyState GetKey(int nKeyID);
	void HandleKeyboardInput();

	// -------------------------------------------------------------------------

	void PrintInputFile(const Module& module, int index, int total);
	void PrintModuleInfo(const Module& module, const Output& output);
	void PrintModuleFrames(const Module& module, FrameId frameId, size_t height);
	void PrintPlaybackProgress();
	void PrintBlankArea(int offset, size_t height);

	void PrintModuleFrames2(const Module& module, FrameId frameId, size_t height);
}