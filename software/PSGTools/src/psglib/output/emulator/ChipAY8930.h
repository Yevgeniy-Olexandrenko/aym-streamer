#pragma once

#include "ChipAY8910.h"

class ChipAY8930 : public ChipAY8910
{
protected:
	void InternalWrite(byte reg, byte data) override;
	void InternalUpdate(double& outL, double& outR) override;
};