#pragma once

#include <vector>
#include "Frame.h"
#include "Filepath.h"
#include "output/SoundChip.h"

using FrameId = uint32_t;
using FrameRate = uint16_t;

class Module
{
	struct Delegate
	{
		Delegate(Module& module) : m_module(module) {}

	protected:
		Module& m_module;
	};

public:
	enum class Property
	{
		Title, Artist, Type, Chip, Frames, Duration
	};

	///////////////////////////////////////////////////////////////////////////////

	struct Info : public Delegate
	{
		Info(Module& module);

		void title(const std::string& title);
		const std::string& title() const;
		bool titleKnown() const;

		void artist(const std::string& artist);
		const std::string& artist() const;
		bool artistKnown() const;

		void type(const std::string& type);
		const std::string& type() const;

	private:
		std::string m_title;
		std::string m_artist;
		std::string m_type;
	};

	///////////////////////////////////////////////////////////////////////////////

	struct Chip : public Delegate
	{
		Chip(Module& module);

		void type(ChipType type);
		ChipType type() const;
		bool typeKnown() const;

		void freq(ChipFreq freq);
		void freqValue(uint32_t freqValue);
		ChipFreq freq() const;
		uint32_t freqValue(uint32_t defFreqValue) const;
		bool freqKnown() const;

		void stereo(ChipStereo stereo);
		ChipStereo stereo() const;
		bool stereoKnown() const;

		void config(ChipConfig config);
		ChipConfig config() const;

	private:
		ChipType m_type;
		ChipFreq m_freq;
		ChipStereo m_stereo;
		ChipConfig m_config;
	};

	///////////////////////////////////////////////////////////////////////////////

	struct Frames : public Delegate
	{
		Frames(Module& module);

		void add(const Frame& frame);
		const Frame& get(FrameId id) const;
		uint32_t count() const;
		FrameId lastFrameId() const;
		bool available() const;

	private:
		std::vector<Frame> m_frames;
	};

	///////////////////////////////////////////////////////////////////////////////

	struct Loop : public Delegate
	{
		Loop(Module& module);

		void frameId(FrameId id);
		FrameId frameId() const;
		uint32_t framesCount() const;
		int extraLoops() const;
		bool available() const;

	private:
		void ComputeExtraLoops();
		void UpdateLoopFrameChanges();

	private:
		FrameId m_frameId;
		int m_extraLoops;
	};

	///////////////////////////////////////////////////////////////////////////////

	struct Playback : public Delegate
	{
		Playback(Module& module);

		const Frame& getFrame(FrameId id) const;
		uint32_t framesCount() const;
		FrameId lastFrameId() const;

		void frameRate(FrameRate frameRate);
		FrameRate frameRate() const;

		void realDuration(int& hh, int& mm, int& ss, int& ms) const;
		void realDuration(int& hh, int& mm, int& ss) const;

		void fakeDuration(int& hh, int& mm, int& ss, int& ms) const;
		void fakeDuration(int& hh, int& mm, int& ss) const;

	private:
		void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const;
		void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const;

	private:
		FrameRate m_frameRate;
	};

	///////////////////////////////////////////////////////////////////////////////

	Module();

	Filepath file;
	Info     info;
	Chip     chip;
	Frames   frames;
	Loop     loop;
	Playback playback;

	std::string property(Property property) const;

//	friend std::ostream& operator<<(std::ostream& stream, const Module& module);
};
