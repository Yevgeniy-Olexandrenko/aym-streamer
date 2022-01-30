#pragma once

#include <fstream>

class Module;
class Frame;

class Decoder
{
public:
	virtual bool InitModule(std::ifstream& file, Module& module) = 0;
	virtual bool DecodeFrame(std::ifstream& file, Frame& frame) = 0;
};
