#pragma once

struct UserCmd;
class Entity;

namespace TriggerbotMelee
{
    void run(Entity* activeWeapon, UserCmd* cmd, float& lastTime, float& lastContact) noexcept;
}
