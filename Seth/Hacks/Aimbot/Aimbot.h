#pragma once
#include <vector>

#include "../../SDK/matrix3x4.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

class Entity;
class matrix3x4;
struct UserCmd;

namespace Aimbot
{
    void run(UserCmd* cmd) noexcept;
    void updateInput() noexcept;
    void reset() noexcept;

    std::vector<Vector> multiPoint(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox, Vector localEyePos, int _hitbox, bool doNotRunMultipoint = false) noexcept;
}