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
	Module::FrameId GetFrameId() const;

private:
	Output& m_output;
	const Module* m_module;
	Module::FrameId m_frame;

	bool m_isMuted;
	bool m_wasMuted;
};