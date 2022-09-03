#pragma once

#include <vector>
#include <filesystem>
#include "Property.h"
#include "Frame.h"
#include "Chip.h"

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

	struct Play : public Delegate
	{
		Play(Stream& stream);

		RO_PROP_DEC( size_t,    framesCount );
		RO_PROP_DEC( FrameId,   lastFrameId );
		RW_PROP_DEF( FrameRate, frameRate   );

	public:
		const Frame& GetFrame(FrameId frameId) const;

		void GetRealDuration(int& hh, int& mm, int& ss, int& ms) const;
		void GetRealDuration(int& hh, int& mm, int& ss) const;

		void GetFakeDuration(int& hh, int& mm, int& ss, int& ms) const;
		void GetFakeDuration(int& hh, int& mm, int& ss) const;

		void ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss, int& ms) const;
		void ComputeDuration(size_t frameCount, int& hh, int& mm, int& ss) const;
	};

////////////////////////////////////////////////////////////////////////////////

public:
	enum class Property
	{
		Title, Artist, Comment, Type, Chip, Frames, Duration
	};

	Stream();
	std::string ToString(Property property) const;

	RO_PROP_DEC( size_t,  framesCount );
	RO_PROP_DEC( FrameId, lastFrameId );
	
public:
	void AddFrame(const Frame& frame);
	const Frame& GetFrame(FrameId frameId) const;

	bool IsSecondChipUsed() const;
	bool IsExpandedModeUsed() const;
	bool IsExpandedModeUsed(int chip) const;

public:
	File file;
	Info info;
	Chip chip;
	Loop loop;
	Play play;

private:
	Frames m_frames;
	bool m_isSecondChipUsed;
	bool m_isExpandedModeUsed[2];
};
