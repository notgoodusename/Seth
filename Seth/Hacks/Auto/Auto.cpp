#include "Auto.h"

#include "AutoDetonate.h"

#include "../../SDK/Entity.h"
#include "../../SDK/LocalPlayer.h"

void Auto::run(UserCmd* cmd) noexcept
{
	if (!config->autoKey.isActive())
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();

	AutoDetonate::run(activeWeapon, cmd);
}

void Auto::updateInput() noexcept
{
	config->autoKey.handleToggle();
}