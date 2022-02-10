#pragma once

#include <vector>
#include "Frame.h"
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

	///////////////////////////////////////////////////////////////////////////////

	struct File : public Delegate
	{
		File(Module& module);

		void pathNameExt(const std::string& path);
		const std::string pathNameExt() const;
		const std::string nameExt() const;
		const std::string name() const;
		const std::string ext() const;

	private:
		std::string m_folder;
		std::string m_name;
		std::string m_ext;
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

		void rawDuration(int& hh, int& mm, int& ss, int& ms) const;
		void rawDuration(int& hh, int& mm, int& ss) const;

		void duration(int& hh, int& mm, int& ss, int& ms) const;
		void duration(int& hh, int& mm, int& ss) const;

	private:
		void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const;
		void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const;

	private:
		FrameRate m_frameRate;
	};

	///////////////////////////////////////////////////////////////////////////////

	Module();

	File     file;
	Info     info;
	Chip     chip;
	Frames   frames;
	Loop     loop;
	Playback playback;

	friend std::ostream& operator<<(std::ostream& stream, const Module& module);
};