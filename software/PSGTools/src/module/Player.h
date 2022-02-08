#pragma once

#include <thread>
#include <atomic>
#include "Module.h"

class Output;

class Player
{
	using AtomicBoolean = std::atomic<bool>;
	using AtomicFrameId = std::atomic<FrameId>;

public:
	Player(Output& output);
	~Player();

public:
	bool Init(const Module& module);
	void Play(int playbackStep = 1);
	void Stop();

	FrameId GetFrameId() const;
	bool IsPlaying() const;
	bool IsPaused() const;

private:
	void PlaybackThread();

private:
	Output& m_output;
	const Module* m_module;

	std::thread m_playback;
	int m_playbackStep;

	AtomicBoolean m_isPlaying;
	AtomicBoolean m_isPaused;
	AtomicFrameId m_frameId;
};