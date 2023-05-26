#pragma once

struct UserCmd;
class Entity;

namespace TriggerbotHitscan
{
    void run(Entity* activeWeapon, UserCmd* cmd, float& lastTime, float& lastContact) noexcept;
}