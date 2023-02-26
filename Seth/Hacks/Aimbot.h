#pragma once

class Entity;
struct UserCmd;

namespace Aimbot
{
    void run(UserCmd* cmd) noexcept;

    void runHitscan(Entity* activeWeapon, UserCmd* cmd) noexcept;
    void runProjectile(Entity* activeWeapon, UserCmd* cmd) noexcept;
    void runMelee(Entity* activeWeapon, UserCmd* cmd) noexcept;

    void updateInput() noexcept;

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
}