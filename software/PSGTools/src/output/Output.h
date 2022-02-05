#pragma once

class Module;
class Frame;

class Output
{
public:
	Output(const Module& module)
		: m_isOpened(false) 
	{}

public:
	virtual void Open() = 0;
	virtual bool IsOpened() const { return m_isOpened; }
	virtual bool OutFrame(const Frame& frame, bool force) = 0;
	virtual void Close() = 0;

protected:
	bool m_isOpened;
};
