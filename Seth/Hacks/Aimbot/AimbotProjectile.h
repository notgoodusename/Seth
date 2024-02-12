#pragma once

class Entity;
struct UserCmd;

namespace AimbotProjectile
{
    void run(Entity* activeWeapon, UserCmd* cmd) noexcept;

    void reset() noexcept;
}