#pragma once

#include <stdint.h>
#include <string>
#include "Property.h"

struct Chip
{
public:
	enum class Count  { OneChip, TwoChips };
	enum class Model  { Unknown, AY8910, YM2149, AY8930, Compatible };
	enum class Clock  { Unknown, F1000000, F1750000, F1773400, F2000000 };
	enum class Output { Unknown, Mono, Stereo };
	enum class Stereo { Unknown, ABC, ACB, BAC, BCA, CAB, CBA };

public:
	Chip();
	std::string toString() const;

	RW_PROP_DEF( Count,  count      );
	RO_PROP_IMP( int,    countValue );
	RW_PROP_DEF( Model,  model      );
	RW_PROP_DEF( Clock,  clock      );
	RW_PROP_IMP( int,    clockValue );
	RW_PROP_DEF( Output, output     );
	RW_PROP_DEF( Stereo, stereo     );

	RO_PROP_DEC( bool, modelKnown   );
	RO_PROP_DEC( bool, clockKnown   );
	RO_PROP_DEC( bool, outputKnown  );
	RO_PROP_DEC( bool, stereoKnown  );
};