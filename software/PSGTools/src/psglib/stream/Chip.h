#pragma once

#include <stdint.h>
#include <string>
#include "Property.h"

struct Chip
{
public:
	enum class Count
		{ SingleChip, TurboSound };

	enum class Model
		{ Unknown, AY, YM, Compatible };

	enum class Frequency
		{ Unknown, F1000000, F1750000, F1773400, F2000000 };

	enum class Channels
		{ Unknown, MONO, ABC, ACB };

public:
	Chip();
	std::string toString() const;

	RW_PROP_DEF( Count,     count     );
	RW_PROP_DEF( Model,     model     );
	RW_PROP_DEF( Frequency, frequency );
	RW_PROP_IMP( uint32_t,  freqValue );
	RW_PROP_DEF( Channels,  channels  );

	RO_PROP_DEC( bool, modelKnown     );
	RO_PROP_DEC( bool, frequencyKnown );
	RO_PROP_DEC( bool, channelsKnown  );
};