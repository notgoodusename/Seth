#pragma once

#include "VirtualMethod.h"

class Panel
{
public:
	VIRTUAL_METHOD(void, setMouseInputEnabled, 32, (unsigned int panel, bool allowForce), (this, panel, allowForce))
	VIRTUAL_METHOD(const char*, getName, 36, (unsigned int panel), (this, panel))
};