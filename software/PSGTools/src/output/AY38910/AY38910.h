#pragma once

#include "../Output.h"
#include "ayumi.h"

class AY38910 : public Output
{
public:
	AY38910();
	~AY38910();

public:
	void Open() override;
	bool IsOpened() const override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;

private:
	struct ayumi m_ay;
	bool m_isOpened;
};
