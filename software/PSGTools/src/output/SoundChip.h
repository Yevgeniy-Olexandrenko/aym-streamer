#pragma once

enum class ChipType
{
	Unknown, AY, YM, Compatible
};

enum class ChipFreq
{
	Unknown, F1000000, F1750000, F1773400, F2000000
};

enum class ChipStereo
{
	Unknown, MONO, ABC, ACB
};

enum class ChipConfig
{
	SingleChip, TurboSound
};
