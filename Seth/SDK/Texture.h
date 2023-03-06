#pragma once

#include "VirtualMethod.h"

class Texture
{
public:
	VIRTUAL_METHOD(int, getActualWidth, 3, (), (this))
	VIRTUAL_METHOD(int, getActualHeight, 4, (), (this))
	VIRTUAL_METHOD(void, incrementReferenceCount, 10, (), (this))
};