#include <iomanip>
#include <sstream>
#include "Module.h"

////////////////////////////////////////////////////////////////////////////////

namespace
{
	const int k_prefDurationMM = 4;
	const int k_prefDurationSS = 0;
	const int k_maxExtraLoops  = 3;
}

Module::Module()
	: info(*this)
	, frames(*this)
	, loop(*this)
	, playback(*this)
{
}

std::string Module::property(Property property) const
{
	std::stringstream stream;
	switch (property)
	{
	case Property::Title:
		stream << info.title();
		break;

	case Property::Artist:
		stream << info.artist();
		break;

	case Property::Type:
		stream << info.type();
		break;;

	case Property::Chip:
		return chip.toString();

	case Property::Frames:
		stream << frames.count();
		if (loop.available()) stream << " -> " << loop.frameId();
		stream << " @ " << playback.frameRate() << " Hz";
		break;

	case Property::Duration:
		int hh0 = 0, mm0 = 0, ss0 = 0, ms0 = 0;
		int hh1 = 0, mm1 = 0, ss1 = 0, ms1 = 0;
		playback.realDuration(hh0, mm0, ss0, ms0);
		playback.fakeDuration(hh1, mm1, ss1, ms1);

		stream <<
			std::setfill('0') << std::setw(2) << hh0 << ':' <<
			std::setfill('0') << std::setw(2) << mm0 << ':' <<
			std::setfill('0') << std::setw(2) << ss0;
		//	stream << '.' << std::setfill('0') << std::setw(3) << ms0;

		if (hh0 != hh1 || mm0 != mm1 || ss0 != ss1)
		{
			stream << " (";
			stream <<
				std::setfill('0') << std::setw(2) << hh1 << ':' <<
				std::setfill('0') << std::setw(2) << mm1 << ':' <<
				std::setfill('0') << std::setw(2) << ss1;
		//	stream << '.' << std::setfill('0') << std::setw(3) << ms1;
			stream << ')';
		}
		break;
	}
	return stream.str();
}

////////////////////////////////////////////////////////////////////////////////

Module::Info::Info(Module& module)
	: Delegate(module)
{
}

void Module::Info::title(const std::string& title)
{
	m_title = title;
}

const std::string& Module::Info::title() const
{
	return m_title;
}

bool Module::Info::titleKnown() const
{
	return !m_title.empty();
}

void Module::Info::artist(const std::string& artist)
{
	m_artist = artist;
}

const std::string& Module::Info::artist() const
{
	return m_artist;
}

bool Module::Info::artistKnown() const
{
	return !m_artist.empty();
}

void Module::Info::type(const std::string& type)
{
	m_type = type;
}

const std::string& Module::Info::type() const
{
	return m_type;
}

////////////////////////////////////////////////////////////////////////////////

Module::Frames::Frames(Module& module)
	: Delegate(module)
{
}

void Module::Frames::add(const Frame& frame)
{
	m_frames.push_back(frame);
}

const Frame& Module::Frames::get(FrameId id) const
{
	return m_frames[id];
}

uint32_t Module::Frames::count() const
{
	return (uint32_t)m_frames.size();
}

FrameId Module::Frames::lastFrameId() const
{
	return count() - 1;
}

bool Module::Frames::available() const
{
	return (count() > 0);
}

////////////////////////////////////////////////////////////////////////////////

Module::Loop::Loop(Module& module)
	: Delegate(module)
	, m_frameId(0)
	, m_extraLoops(0)
{
}

void Module::Loop::frameId(FrameId id)
{
	m_frameId = id;

	UpdateLoopFrameChanges();
	ComputeExtraLoops();
}

FrameId Module::Loop::frameId() const
{
	return (m_frameId ? m_frameId : m_module.frames.count());
}

uint32_t Module::Loop::framesCount() const
{
	return m_module.frames.count() - frameId();
}

int Module::Loop::extraLoops() const
{
	return m_extraLoops;
}

bool Module::Loop::available() const
{
	return (framesCount() > (m_module.frames.count() / 2));
}

void Module::Loop::ComputeExtraLoops()
{
	m_extraLoops = 0;
	if (available())
	{
		uint32_t frameCount = m_module.frames.count();
		uint32_t maxPlaybackFrames = (k_prefDurationMM * 60 + k_prefDurationSS) * m_module.playback.frameRate();
		if (maxPlaybackFrames > frameCount)
		{
			m_extraLoops = (maxPlaybackFrames - frameCount) / framesCount();
			m_extraLoops = std::min(m_extraLoops, k_maxExtraLoops);
		}
	}
}

void Module::Loop::UpdateLoopFrameChanges()
{
	if (available())
	{
		uint8_t loopFrameData, lastFrameData;
		const Frame& lastFrame = m_module.frames.get(m_module.frames.lastFrameId());
		Frame& loopFrame = const_cast<Frame&>(m_module.frames.get(frameId()));

		for (uint8_t i = 0; i < 16; ++i)
		{
			if (!loopFrame.changed(0, i))
			{
				loopFrameData = loopFrame.data(0, i);
				lastFrameData = lastFrame.data(0, i);

				if (loopFrameData != lastFrameData)
					loopFrame[i].first.override(loopFrameData);
			}

			if (!loopFrame.changed(1, i))
			{
				loopFrameData = loopFrame.data(1, i);
				lastFrameData = lastFrame.data(1, i);

				if (loopFrameData != lastFrameData)
					loopFrame[i].second.override(loopFrameData);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

Module::Playback::Playback(Module& module)
	: Delegate(module)
	, m_frameRate(0)
{
}

const Frame& Module::Playback::getFrame(FrameId id) const
{
	uint32_t frameCount = m_module.frames.count();
	if (id >= frameCount)
	{
		id = m_module.loop.frameId() + ((id - frameCount) % m_module.loop.framesCount());
	}
	return m_module.frames.get(id);
}

uint32_t Module::Playback::framesCount() const
{
	return m_module.frames.count() + m_module.loop.extraLoops() * m_module.loop.framesCount();
}

FrameId Module::Playback::lastFrameId() const
{
	return framesCount() - 1;
}

void Module::Playback::frameRate(FrameRate frameRate)
{
	m_frameRate = frameRate;
}

FrameRate Module::Playback::frameRate() const
{
	return m_frameRate;
}

void Module::Playback::realDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss, ms);
}

void Module::Playback::realDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(m_module.frames.count(), hh, mm, ss);
}

void Module::Playback::fakeDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(framesCount(), hh, mm, ss, ms);
}

void Module::Playback::fakeDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(framesCount(), hh, mm, ss);
}

void Module::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const
{
	uint32_t duration = (frameCount * 1000) / frameRate();
	ms = duration % 1000; duration /= 1000;
	ss = duration % 60;   duration /= 60;
	mm = duration % 60;   duration /= 60;
	hh = duration;
}

void Module::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const
{
	uint32_t duration = (((frameCount * 1000) / frameRate()) + 500) / 1000;
	ss = duration % 60; duration /= 60;
	mm = duration % 60; duration /= 60;
	hh = duration;
}

////////////////////////////////////////////////////////////////////////////////
