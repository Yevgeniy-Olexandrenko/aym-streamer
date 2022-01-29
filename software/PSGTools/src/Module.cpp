#include "Module.h"

void Module::AddFrame(const Frame& frame)
{
	m_frames.push_back(frame);
}

const Frame& Module::GetFrame(size_t index)
{
	return m_frames[index];
}

size_t Module::GetFrameCount() const
{
	return m_frames.size();
}
