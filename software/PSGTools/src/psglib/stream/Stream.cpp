#include <iomanip>
#include <sstream>
#include "Stream.h"

namespace
{
	const int k_prefDurationMM = 4;
	const int k_prefDurationSS = 0;
	const int k_maxExtraLoops  = 3;
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Stream::Info::Info(Stream& stream)
	: Delegate(stream)
{
}

bool Stream::Info::titleKnown() const
{
	return !m_title.empty();
}

bool Stream::Info::artistKnown() const
{
	return !m_artist.empty();
}

bool Stream::Info::commentKnown() const
{
	return !m_comment.empty();
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Stream::Loop::Loop(Stream& stream)
	: Delegate(stream)
	, m_frameId(0)
	, m_extraLoops(0)
{
}

void Stream::Loop::frameId(const FrameId& frameId)
{
	m_frameId = frameId;

	UpdateLoopFrameChanges();
	ComputeExtraLoops();
}

FrameId Stream::Loop::frameId() const
{
	return (m_frameId > 2 ? m_frameId : FrameId(m_stream.framesCount()));
}

bool Stream::Loop::available() const
{
	return (framesCount() >= (m_stream.framesCount() / 2));
}

size_t Stream::Loop::framesCount() const
{
	return (m_stream.framesCount() - frameId());
}

void Stream::Loop::ComputeExtraLoops()
{
	m_extraLoops = 0;
	if (available())
	{
		auto frameCount = m_stream.framesCount();
		auto maxPlaybackFrames = size_t((k_prefDurationMM * 60 + k_prefDurationSS) * m_stream.play.frameRate());
		if (maxPlaybackFrames > frameCount)
		{
			m_extraLoops = int((maxPlaybackFrames - frameCount) / framesCount());
			m_extraLoops = std::min(m_extraLoops, k_maxExtraLoops);
		}
	}
}

void Stream::Loop::UpdateLoopFrameChanges()
{
	if (available())
	{
		//uint8_t loopFrameData, lastFrameData;
		//const Frame& lastFrame = m_stream.GetFrame(m_stream.lastFrameId());
		//Frame& loopFrame = const_cast<Frame&>(m_stream.GetFrame(frameId()));

		//for (uint8_t reg = 0; reg < 16; ++reg)
		//{
		//	if (!loopFrame.IsChanged(0, reg))
		//	{
		//		loopFrameData = loopFrame.Read(0, reg);
		//		lastFrameData = lastFrame.Read(0, reg);

		//		if (loopFrameData != lastFrameData)
		//			loopFrame.Write(0, reg, loopFrameData);
		//	}

		//	if (!loopFrame.IsChanged(1, reg))
		//	{
		//		loopFrameData = loopFrame.Read(1, reg);
		//		lastFrameData = lastFrame.Read(1, reg);

		//		if (loopFrameData != lastFrameData)
		//			loopFrame.Write(1, reg, loopFrameData);
		//	}
		//}
	}
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Stream::Play::Play(Stream& stream)
	: Delegate(stream)
	, m_frameRate(0)
{
}

size_t Stream::Play::framesCount() const
{
	return m_stream.framesCount() + m_stream.loop.extraLoops() * m_stream.loop.framesCount();
}

FrameId Stream::Play::lastFrameId() const
{
	return FrameId(framesCount() - 1);
}

const Frame& Stream::Play::GetFrame(FrameId frameId) const
{
	auto frameCount = m_stream.framesCount();
	if (frameId >= frameCount)
	{
		frameId = m_stream.loop.frameId() + FrameId((frameId - frameCount) % m_stream.loop.framesCount());
	}
	return m_stream.GetFrame(frameId);
}

void Stream::Play::GetRealDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(m_stream.framesCount(), hh, mm, ss, ms);
}

void Stream::Play::GetRealDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(m_stream.framesCount(), hh, mm, ss);
}

void Stream::Play::GetFakeDuration(int& hh, int& mm, int& ss, int& ms) const
{
	ComputeDuration(framesCount(), hh, mm, ss, ms);
}

void Stream::Play::GetFakeDuration(int& hh, int& mm, int& ss) const
{
	ComputeDuration(framesCount(), hh, mm, ss);
}

void Stream::Play::ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss, int& ms) const
{
	auto duration = (frameCount * 1000) / frameRate();
	ms = int(duration % 1000); duration /= 1000;
	ss = int(duration % 60);   duration /= 60;
	mm = int(duration % 60);   duration /= 60;
	hh = int(duration);
}

void Stream::Play::ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss) const
{
	auto duration = (((frameCount * 1000) / frameRate()) + 500) / 1000;
	ss = int(duration % 60); duration /= 60;
	mm = int(duration % 60); duration /= 60;
	hh = int(duration);
}

/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

Stream::Stream()
	: info(*this)
	, loop(*this)
	, play(*this)
{
}

std::string Stream::ToString(Property property) const
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
		stream << framesCount();
		if (loop.available()) stream << " -> " << loop.frameId();
		stream << " @ " << play.frameRate() << " Hz";
		break;

	case Property::Duration:
		int hh0 = 0, mm0 = 0, ss0 = 0, ms0 = 0;
		int hh1 = 0, mm1 = 0, ss1 = 0, ms1 = 0;
		play.GetRealDuration(hh0, mm0, ss0);
		play.GetFakeDuration(hh1, mm1, ss1);

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

size_t Stream::framesCount() const
{
	return m_frames.size();
}

FrameId Stream::lastFrameId() const
{
	return FrameId(framesCount() - 1);
}

void Stream::AddFrame(const Frame& frame)
{
	if (framesCount() < 100000)
	{
		FrameId id = m_frames.size();
		m_frames.push_back(frame);
		m_frames.back().SetId(id);
	}
}

const Frame& Stream::GetFrame(FrameId frameId) const
{
	return m_frames[frameId];
}
