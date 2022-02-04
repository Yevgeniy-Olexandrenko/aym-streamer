#pragma once

#include "../Output.h"

class AY38910 : public Output
{
public:
	void Open() override;
	bool IsOpened() const override;
	bool OutFrame(const Frame& frame, bool force) override;
	void Close() override;
};
