#pragma once

namespace PSG
{
    enum TYPE
    {
        TYPE_NOT_FOUND   = 0x00,
        TYPE_AY8910      = 0x01,
        TYPE_AY8910A     = 0x02,
        TYPE_AY8912      = 0x03,
        TYPE_AY8930      = 0x04,
        TYPE_YM2149      = 0x05,
        TYPE_COMPATIBLE  = 0x06,
        TYPE_AVRAY_FW26  = 0x07,
        TYPE_BAD_UNKNOWN = 0xFF
    };

    void Detect();
    TYPE Type();
}