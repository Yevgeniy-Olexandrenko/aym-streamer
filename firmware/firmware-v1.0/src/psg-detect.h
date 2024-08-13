#pragma once

#include "avr-support.h"
#include "psg-access.h"

enum psg_type
{
    PSG_TYPE_NOT_FOUND   = 0x00,
    PSG_TYPE_AY_3_8910   = 0x01,
    PSG_TYPE_YM2149F     = 0x02,
    PSG_TYPE_KC89C72     = 0x03,
    PSG_TYPE_AVR_AY_26   = 0x04,
    PSG_TYPE_BAD_UNKNOWN = 0xFF
};

void PSG_Detect();
psg_type PSG_Type();
