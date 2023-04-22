#pragma once

#include "Math.h"
#include "ModelInfo.h"
#include "matrix3x4.h"
#include "Vector.h"

class Entity;

namespace Math
{
	bool canBackstab(Entity* entity, Vector angles, Vector entityAngles) noexcept;
    bool doesMeleeHit(Entity* activeWeapon, int index, const Vector angles) noexcept;

	Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept;
	bool hitboxIntersection(const matrix3x4 matrix[MAXSTUDIOBONES], int iHitbox, StudioHitboxSet* set, const Vector& start, const Vector& end) noexcept;
}