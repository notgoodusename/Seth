#pragma once

#include "VirtualMethod.h"

#include <functional>

class PhysicsCollide;

class PhysicsCollision
{
public:
	VIRTUAL_METHOD(PhysicsCollide*, bboxToCollide, 29, (const Vector& mins, const Vector& maxs), (this, std::cref(mins), std::cref(maxs)))
};