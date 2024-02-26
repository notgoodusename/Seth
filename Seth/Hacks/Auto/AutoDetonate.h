#pragma once

class Entity;
struct UserCmd;

namespace AutoDetonate
{
	void run(Entity* activeWeapon, UserCmd* cmd) noexcept;
}