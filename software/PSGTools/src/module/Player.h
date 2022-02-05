#pragma once

#include "Module.h"

class Output;
class Module;

class Player
{
public:
	Player(Output& output);
	~Player();

public:
	bool InitWithModule(const Module& module);
	bool PlayModuleFrame();

	void Mute(bool on);


private:
	Output& m_output;
	const Module* m_module;
	Module::FrameIndex m_frame;

	bool m_isMuted;
	bool m_wasMuted;
};