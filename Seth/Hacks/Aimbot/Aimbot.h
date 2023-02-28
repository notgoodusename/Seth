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

    struct Enemy {
        int id;
        int health;
        float distance;
        float fov;

        bool operator<(const Enemy& enemy) const noexcept {
            if (health != enemy.health)
                return health < enemy.health;
            if (fov != enemy.fov)
                return fov < enemy.fov;
            return distance < enemy.distance;
        }

        Enemy(int id, int health, float distance, float fov) noexcept : id(id), health(health), distance(distance), fov(fov) { }
    };

    std::vector<Vector> multiPoint(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox, Vector localEyePos, int _hitbox, bool doNotRunMultipoint = false) noexcept;
    std::vector<Aimbot::Enemy>& getEnemies() noexcept;
}