#pragma once

#include "SerialPort.h"

class Module;
class Frame;

class Player
{
public:
	Player(int comPortIndex);
	~Player();

public:
	bool InitWithModule(const Module& module);
	bool PlayModuleFrame();

	void Mute(bool on);

private:
	void OutFrame(const Frame& frame, bool force);

private:
	SerialPort m_port;
	bool m_isPortOK;

	const Module* m_module;
	size_t m_frame;

	bool m_isMuted;
	bool m_wasMuted;
};