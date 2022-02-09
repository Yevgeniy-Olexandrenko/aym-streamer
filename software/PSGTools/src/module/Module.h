#pragma once

#include <vector>
#include "Frame.h"

using FrameId = uint32_t;
using FrameRate = uint16_t;

enum class ChipType
{
	Unknown, AY, YM
};

enum class ChipFreq
{
	Unknown, F1000000, F1750000, F1773400, F2000000
};

enum class ChipStereo
{
	Unknown, MONO, ABC, ACB
};

class Module
{
	using FrameList = std::vector<Frame>;
	friend std::ostream& operator<<(std::ostream& stream, const Module& module);

public:
	Module();

	// input/output file folder, name and ext
	void SetFilePath(const std::string& filePath);
	const std::string GetFilePath() const;
	const std::string GetFileName() const;
	const std::string GetFileExt() const;

	// song title (optional)
	void SetTitle(const std::string& title);
	const std::string& GetTitle() const;
	bool HasTitle() const;

	// song artist (optional)
	void SetArtist(const std::string& artist);
	const std::string& GetArtist() const;
	bool HasArtist() const;

	// input file type
	void SetType(const std::string& type);
	const std::string& GetType() const;

	// chip type
	void SetChipType(ChipType chipType);
	ChipType GetChipType() const;
	bool HasChipType() const;

	// chip clock frequency
	void SetChipFreq(ChipFreq chipFreq);
	void SetChipFreqValue(uint32_t chipFreqValue);
	ChipFreq GetChipFreq() const;
	uint32_t GetChipFreqValue(uint32_t defChipFreq) const;
	bool HasChipFreq() const;

	// chip stereo mode
	void SetChipStereo(ChipStereo chipStereo);
	ChipStereo GetChipStereo() const;
	bool HasChipStereo() const;

	// frame rate
	void SetFrameRate(FrameRate frameRate);
	FrameRate GetFrameRate() const;

	// frames
	void AddFrame(const Frame& frame);
	const Frame& GetFrame(FrameId id) const;
	FrameId GetLastFrameId() const;
	uint32_t GetFrameCount() const;
	void GetDuration(int& hh, int& mm, int& ss, int& ms) const;
	void GetDuration(int& hh, int& mm, int& ss) const;

	// loop frame
	void SetLoopFrameId(FrameId id);
	FrameId GetLoopFrameId() const;
	uint32_t GetLoopFrameCount() const;
	bool HasLoop() const;

	// playback frames
	const Frame& GetPlaybackFrame(FrameId id) const;
	FrameId GetPlaybackLastFrameId() const;
	uint32_t GetPlaybackFrameCount() const;
	void GetPlaybackDuration(int& hh, int& mm, int& ss, int& ms) const;
	void GetPlaybackDuration(int& hh, int& mm, int& ss) const;

private:
	void ComputeExtraLoops();
	void UpdateLoopFrameChanges();
	void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const;
	void ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const;

private:
	struct {
		std::string m_folder;
		std::string m_name;
		std::string m_ext;
	} m_file;

	std::string m_title;
	std::string m_artist;
	std::string m_type;

	ChipType m_chipType;
	ChipFreq m_chipFreq;
	ChipStereo m_chipStereo;

	FrameList m_frames;
	FrameRate m_frameRate;
	FrameId   m_loopFrameId;
	int       m_extraLoops;
};