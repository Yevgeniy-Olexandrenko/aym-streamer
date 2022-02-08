#pragma once

#include <chrono>
#include <thread>
#include <atomic>
#include "Module.h"

class Output;

class Player
{
	using Duration = std::chrono::duration<int, std::milli>;
	using Clock = std::chrono::high_resolution_clock;
	using Time = Clock::time_point;

	using AtomicBoolean = std::atomic<bool>;
	using AtomicFrameId = std::atomic<FrameId>;

public:
	Player(Output& output);
	~Player();

public:
	bool Init(const Module& module);
	void Step(int step);
	void Play();
	void Stop();

	FrameId GetFrameId() const;
	bool IsPlaying() const;
	bool IsPaused() const;

private:
	void PlaybackThread();
	bool GotoNextFrame();

private:
	Output& m_output;
	const Module* m_module;

	std::thread m_playback;
	int m_step;

	AtomicBoolean m_isPlaying;
	AtomicBoolean m_isPaused;
	AtomicFrameId m_frameId;
};