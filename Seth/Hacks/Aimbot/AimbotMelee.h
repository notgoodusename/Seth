#pragma once

class Entity;
struct UserCmd;

namespace AimbotMelee
{
    void run(Entity* activeWeapon, UserCmd* cmd) noexcept;

    void reset() noexcept;
}