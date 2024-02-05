#pragma once

#include "Math.h"
#include "ModelInfo.h"
#include "matrix3x4.h"
#include "Vector.h"

class Entity;

namespace Math
{
	bool canBackstab(Vector angles, Vector entityAngles, Vector entityWorldSpaceCenter) noexcept;
    bool doesMeleeHit(Entity* activeWeapon, int index, const Vector angles) noexcept;

	void vectorTransform(const Vector& in1, const matrix3x4* in2, Vector& out) noexcept;

	Vector getCenterOfHitbox(const matrix3x4* matrix, StudioBbox* hitbox) noexcept;
	Vector getCenterOfHitbox(Entity* entity, const matrix3x4* matrix, int hitboxNumber) noexcept;

	Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept;
	bool hitboxIntersection(const matrix3x4* matrix, int hitboxNumber, StudioHitboxSet* set, const Vector& start, const Vector& end) noexcept;
}