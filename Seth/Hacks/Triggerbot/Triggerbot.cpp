#include "../Aimbot/Aimbot.h"
#include "../Backtrack.h"
#include "Triggerbot.h"
#include "TriggerbotHitscan.h"
#include "TriggerbotProjectile.h"
#include "TriggerbotMelee.h"

#include "../Config.h"

#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Math.h"

void Triggerbot::run(UserCmd* cmd) noexcept
{
    if (!config->triggerbotKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponType = activeWeapon->getWeaponType();
    if (weaponType == WeaponType::HITSCAN || weaponType == WeaponType::MELEE)
    {
        if (!canAttack(cmd, activeWeapon))
            return;
    }

    static auto lastTime = 0.0f;
    static auto lastContact = 0.0f;

    switch (weaponType)
    {
    case WeaponType::HITSCAN:
        TriggerbotHitscan::run(activeWeapon, cmd, lastTime, lastContact);
        break;
    case WeaponType::MELEE:
        TriggerbotMelee::run(activeWeapon, cmd, lastTime, lastContact);
        break;
    default:
        break;
    }

    TriggerbotProjectile::run(activeWeapon, cmd);
}

void Triggerbot::updateInput() noexcept
{
	config->triggerbotKey.handleToggle();
}

void Triggerbot::reset() noexcept
{

}