#pragma once

class Entity;
struct UserCmd;

namespace AimbotHitscan
{
    void run(Entity* activeWeapon, UserCmd* cmd) noexcept;

    void reset() noexcept;
}