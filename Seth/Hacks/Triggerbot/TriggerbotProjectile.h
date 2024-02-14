#pragma once

class Entity;
struct UserCmd;

namespace TriggerbotProjectile
{
	void run(Entity* activeWeapon, UserCmd* cmd) noexcept;
}