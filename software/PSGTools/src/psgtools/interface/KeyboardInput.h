#pragma once

#include <windows.h>

struct KeyState
{
	bool pressed;
	bool released;
	bool held;
};

class KeyboardInput
{
public:
	void Update();
	const KeyState& GetKeyState(int key) const;

private:
	SHORT m_keyOldState[256];
	SHORT m_keyNewState[256];
	KeyState m_keys[256];
};
