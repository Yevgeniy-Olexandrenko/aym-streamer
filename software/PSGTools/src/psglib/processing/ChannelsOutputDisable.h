#pragma once

#include "Processing.h"

class ChannelsOutputDisable : public Processing
{
    Chip m_chip;

    bool m_enableA;
    bool m_enableB;
    bool m_enableC;
    bool m_enableE;
    bool m_enableN;

public:
    ChannelsOutputDisable(const Chip& dstChip)
        : m_chip(dstChip)
        , m_enableA(true)
        , m_enableB(true)
        , m_enableC(true)
        , m_enableE(true)
        , m_enableN(true)
    {}

    void SetEnableA(bool yes) { m_enableA = yes; }
    void SetEnableB(bool yes) { m_enableB = yes; }
    void SetEnableC(bool yes) { m_enableC = yes; }
    void SetEnableE(bool yes) { m_enableE = yes; }
    void SetEnableN(bool yes) { m_enableN = yes; }

#ifdef Enable_ChannelsOutputDisable
	const Frame& operator()(const Frame& frame) override
    {
        if (!m_enableA || !m_enableB || !m_enableC || !m_enableE || !m_enableN)
        {
            // TODO
        }
        return frame;
    }
#endif
};
