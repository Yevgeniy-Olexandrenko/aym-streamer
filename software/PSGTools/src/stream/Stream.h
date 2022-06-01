#pragma once

#include <vector>
#include <filesystem>

#include "Frame.h"
#include "output/Chip.h"
#include "Property.h"

using FrameId = uint32_t;
using FrameRate = uint16_t;

class Stream
{
	struct Delegate
	{
		Delegate(Stream& stream) : m_stream(stream) {}

	protected:
		Stream& m_stream;
	};

	using File = std::filesystem::path;
	using Frames = std::vector<Frame>;

public:

	/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

	struct Info : public Delegate
	{
		Info(Stream& stream);

		RW_PROP_DEF( std::string, title   );
		RW_PROP_DEF( std::string, artist  );
		RW_PROP_DEF( std::string, comment );
		RW_PROP_DEF( std::string, type    );

		RO_PROP_DEC( bool, titleKnown   );
		RO_PROP_DEC( bool, artistKnown  );
		RO_PROP_DEC( bool, commentKnown );
	};

	/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

	struct Loop : public Delegate
	{
		Loop(Stream& stream);

		RW_PROP_IMP( FrameId, frameId     );
		RO_PROP_DEF( int,     extraLoops  );
		RO_PROP_DEC( bool,    available   );
		RO_PROP_DEC( size_t,  framesCount );
		
	private:
		void ComputeExtraLoops();
		void UpdateLoopFrameChanges();
	};

	/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

	struct Playback : public Delegate
	{
		Playback(Stream& stream);

		RO_PROP_DEC( size_t,    framesCount );
		RO_PROP_DEC( FrameId,   lastFrameId );
		RW_PROP_DEF( FrameRate, frameRate   );

	public:
		const Frame& getFrame(FrameId frameId) const;
		void realDuration(int& hh, int& mm, int& ss, int& ms) const;
		void realDuration(int& hh, int& mm, int& ss) const;
		void fakeDuration(int& hh, int& mm, int& ss, int& ms) const;
		void fakeDuration(int& hh, int& mm, int& ss) const;

	private:
		void ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss, int& ms) const;
		void ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss) const;
	};

	/// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ ///

public:
	enum class Property
	{
		Title, Artist, Comment, Type, Chip, Frames, Duration
	};

	Stream();

	RO_PROP_DEC( size_t,  framesCount );
	RO_PROP_DEC( FrameId, lastFrameId );
	
public:
	void addFrame(const Frame& frame);
	const Frame& getFrame(FrameId frameId) const;
	std::string property(Property property) const;

public:
	File file;
	Info info;
	Chip chip;
	Loop loop;
	Playback playback;

private:
	Frames m_frames;
};
