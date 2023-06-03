#include "../Aimbot/Aimbot.h"
#include "../Animations.h"
#include "../Backtrack.h"
#include "Triggerbot.h"
#include "TriggerbotHitscan.h"
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
    if (weaponType != WeaponType::HITSCAN && weaponType != WeaponType::MELEE)
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

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
}

void Triggerbot::updateInput() noexcept
{
	config->triggerbotKey.handleToggle();
}

void Triggerbot::reset() noexcept
{

}