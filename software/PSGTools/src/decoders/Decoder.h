#pragma once

class Module;
class Frame;

class Decoder
{
public:
	virtual bool Open   (Module& module) = 0;
	virtual bool Decode (Frame&  frame ) = 0;
	virtual void Close  (Module& module) = 0;
};
