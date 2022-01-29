#pragma once

#include <vector>
#include "Frame.h"

class Module
{
	using FrameArray = std::vector<Frame>;

public:
	// frames
	void AddFrame(const Frame& frame);
	const Frame& GetFrame(size_t index);
	size_t GetFrameCount() const;

private:
	FrameArray m_frames;
};