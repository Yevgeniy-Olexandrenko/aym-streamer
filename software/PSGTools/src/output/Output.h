#pragma once

class Frame;

class Output
{
public:
	virtual void Open() = 0;
	virtual bool IsOpened() const = 0;
	virtual bool OutFrame(const Frame& frame, bool force) = 0;
	virtual void Close() = 0;
};
