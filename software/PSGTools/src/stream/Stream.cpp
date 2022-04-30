#include <iomanip>
#include <sstream>
#include "Stream.h"

////////////////////////////////////////////////////////////////////////////////

namespace
{
	const int k_prefDurationMM = 4;
	const int k_prefDurationSS = 0;
	const int k_maxExtraLoops  = 3;
}

Stream::Stream()
	: info(*this)
	, frames(*this)
	, loop(*this)
	, playback(*this)
{
}

std::string Stream::property(Property property) const
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

	case Property::Comment:
		stream << info.comment();
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
		playback.realDuration(hh0, mm0, ss0);
		playback.fakeDuration(hh1, mm1, ss1);

		stream <<
			std::setfill('0') << std::setw(2) << hh0 << ':' <<
			std::setfill('0') << std::setw(2) << mm0 << ':' <<
			std::setfill('0') << std::setw(2) << ss0;

		if (hh0 != hh1 || mm0 != mm1 || ss0 != ss1)
		{
			stream << " (";
			stream <<
				std::setfill('0') << std::setw(2) << hh1 << ':' <<
				std::setfill('0') << std::setw(2) << mm1 << ':' <<
				std::setfill('0') << std::setw(2) << ss1;
			stream << ')';
		}
		break;
	}
	return stream.str();
}

////////////////////////////////////////////////////////////////////////////////

Stream::Info::Info(Stream& stream)
	: Delegate(stream)
{
}

void Stream::Info::title(const std::string& title)
{
	m_title = title;
}

const std::string& Stream::Info::title() const
{
	return m_title;
}

bool Stream::Info::titleKnown() const
{
	return !m_title.empty();
}

void Stream::Info::artist(const std::string& artist)
{
	m_artist = artist;
}

const std::string& Stream::Info::artist() const
{
	return m_artist;
}

bool Stream::Info::artistKnown() const
{
	return !m_artist.empty();
}

void Stream::Info::comment(const std::string& comment)
{
	m_comment = comment;
}

const std::string& Stream::Info::comment() const
{
	return m_comment;
}

bool Stream::Info::commentKnown() const
{
	return !m_comment.empty();
}

void Stream::Info::type(const std::string& type)
{
	m_type = type;
}

const std::string& Stream::Info::type() const
{
	return m_type;
}

////////////////////////////////////////////////////////////////////////////////

Stream::Frames::Frames(Stream& stream)
	: Delegate(stream)
{
}

void Stream::Frames::add(const Frame& frame)
{
	if (count() < 100000) m_frames.push_back(frame);
}

const Frame& Stream::Frames::get(FrameId id) const
{
	return m_frames[id];
}

uint32_t Stream::Frames::count() const
{
	return (uint32_t)m_frames.size();
}

FrameId Stream::Frames::lastFrameId() const
{
	return count() - 1;
}

bool Stream::Frames::available() const
{
	return (count() > 0);
}

////////////////////////////////////////////////////////////////////////////////

Stream::Loop::Loop(Stream& stream)
	: Delegate(stream)
	, m_frameId(0)
	, m_extraLoops(0)
{
}

void Stream::Loop::frameId(FrameId id)
{
	m_frameId = id;

	UpdateLoopFrameChanges();
	ComputeExtraLoops();
}

FrameId Stream::Loop::frameId() const
{
	return (m_frameId > 2 ? m_frameId : m_stream.frames.count());
}

uint32_t Stream::Loop::framesCount() const
{
	return m_stream.frames.count() - frameId();
}

int Stream::Loop::extraLoops() const
{
	return m_extraLoops;
}

bool Stream::Loop::available() const
{
	return (framesCount() > (m_stream.frames.count() / 2));
}

void Stream::Loop::ComputeExtraLoops()
{
	m_extraLoops = 0;
	if (available())
	{
		uint32_t frameCount = m_stream.frames.count();
		uint32_t maxPlaybackFrames = (k_prefDurationMM * 60 + k_prefDurationSS) * m_stream.playback.frameRate();
		if (maxPlaybackFrames > frameCount)
		{
			m_extraLoops = (maxPlaybackFrames - frameCount) / framesCount();
			m_extraLoops = std::min(m_extraLoops, k_maxExtraLoops);
		}
	}
}

void Stream::Loop::UpdateLoopFrameChanges()
{
	if (available())
	{
		uint8_t loopFrameData, lastFrameData;
		const Frame& lastFrame = m_stream.frames.get(m_stream.frames.lastFrameId());
		Frame& loopFrame = const_cast<Frame&>(m_stream.frames.get(frameId()));

		for (uint8_t reg = 0; reg < 16; ++reg)
		{
			if (!loopFrame.IsChanged(0, reg))
			{
				loopFrameData = loopFrame.Read(0, reg);
				lastFrameData = lastFrame.Read(0, reg);

				if (loopFrameData != lastFrameData)
					loopFrame.Write(0, reg, loopFrameData);
			}

			if (!loopFrame.IsChanged(1, reg))
			{
				loopFrameData = loopFrame.Read(1, reg);
				lastFrameData = lastFrame.Read(1, reg);

				if (loopFrameData != lastFrameData)
					loopFrame.Write(1, reg, loopFrameData);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

Stream::Playback::Playback(Stream& stream)
	: Delegate(stream)
	, m_frameRate(0)
{
}

const Frame& Stream::Playback::getFrame(FrameId id) const
{
	uint32_t frameCount = m_stream.frames.count();
	if (id >= frameCount)
	{
		id = m_stream.loop.frameId() + ((id - frameCount) % m_stream.loop.framesCount());
	}
	return m_stream.frames.get(id);
}

uint32_t Stream::Playback::framesCount() const
{
	return m_stream.frames.count() + m_stream.loop.extraLoops() * m_stream.loop.framesCount();
}

FrameId Stream::Playback::lastFrameId() const
{
	return framesCount() - 1;
}

void Stream::Playback::frameRate(FrameRate frameRate)
{
	m_frameRate = frameRate;
}

FrameRate Stream::Playback::frameRate() const
{
	return m_frameRate;
}

void Stream::Playback::realDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(m_stream.frames.count(), hh, mm, ss, ms);
}

void Stream::Playback::realDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(m_stream.frames.count(), hh, mm, ss);
}

void Stream::Playback::fakeDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(framesCount(), hh, mm, ss, ms);
}

void Stream::Playback::fakeDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(framesCount(), hh, mm, ss);
}

void Stream::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss, int& ms) const
{
	uint32_t duration = (frameCount * 1000) / frameRate();
	ms = duration % 1000; duration /= 1000;
	ss = duration % 60;   duration /= 60;
	mm = duration % 60;   duration /= 60;
	hh = duration;
}

void Stream::Playback::ComputeDuration(uint32_t frameCount, int& hh, int& mm, int& ss) const
{
	uint32_t duration = (((frameCount * 1000) / frameRate()) + 500) / 1000;
	ss = duration % 60; duration /= 60;
	mm = duration % 60; duration /= 60;
	hh = duration;
}

////////////////////////////////////////////////////////////////////////////////
