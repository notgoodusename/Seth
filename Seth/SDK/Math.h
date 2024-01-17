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

	Vector getCenterOfHitbox(const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox) noexcept;
	Vector getCenterOfHitbox(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], int hitboxNumber) noexcept;

	Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept;
	bool hitboxIntersection(const matrix3x4 matrix[MAXSTUDIOBONES], int iHitbox, StudioHitboxSet* set, const Vector& start, const Vector& end) noexcept;
}