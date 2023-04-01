#pragma once

class Entity;
struct UserCmd;

namespace AimbotProjectile
{
    void run(Entity* activeWeapon, UserCmd* cmd) noexcept;

    struct ProjectileWeaponInfo
    {
        float speed;
        float gravity;
        float maxTime;
        Vector offset;
    };

    void reset() noexcept;
}