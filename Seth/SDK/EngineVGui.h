#pragma once

#include "VirtualMethod.h"

class EngineVGui
{
public:
	VIRTUAL_METHOD(unsigned int, getPanel, 1, (int type), (this, type))
	VIRTUAL_METHOD(bool, isGameUIVisible, 2, (), (this))
};