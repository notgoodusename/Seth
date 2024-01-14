#include <cmath>

#include "../Memory.h"

#include "GlobalVars.h"
#include "Entity.h"
#include "matrix3x4.h"
#include "Utils.h"
#include "Vector.h"

std::tuple<float, float, float> rainbowColor(float speed) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::make_tuple(std::sin(speed * memory->globalVars->realTime) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realTime + 2 * pi / 3) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realTime + 4 * pi / 3) * 0.5f + 0.5f);
}

int getMaxUserCmdProcessTicks() noexcept
{
    return 22;
}

void applyMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector eyeAngle, Vector mins, Vector maxs) noexcept
{
    entity->invalidateBoneCache();
    memcpy(entity->getBoneCache().memory, boneCacheData, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
    memory->setAbsOrigin(entity, origin);
    entity->eyeAngles() = eyeAngle;
    memory->setCollisionBounds(entity->getCollideable(), mins, maxs);
}

bool canAttack(UserCmd* cmd, Entity* activeWeapon) noexcept
{
	if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
		|| localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
		return false;

	switch (activeWeapon->weaponId())
	{
		case WeaponId::PDA:
		case WeaponId::PDA_ENGINEER_BUILD:
		case WeaponId::PDA_ENGINEER_DESTROY:
		case WeaponId::PDA_SPY:
		case WeaponId::MEDIGUN:
			return false;
		default:
			break;
	}

	switch (activeWeapon->itemDefinitionIndex())
	{
	case Soldier_m_TheBeggarsBazooka:
		return activeWeapon->clip() && !activeWeapon->isInReload() && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime();
	case Sniper_m_TheMachina:
	case Sniper_m_ShootingStar:
	{
		if (!localPlayer->isScoped())
			return false;
		break;
	}
	default:
		break;
	}

	switch (activeWeapon->getWeaponType())
	{
		case WeaponType::HITSCAN:
			if (!activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() || activeWeapon->isInReload())
				return false;
			break;
		case WeaponType::PROJECTILE:
			if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
				return false;
			break;
		case WeaponType::MELEE:
			if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
				return false;
			break;
		default:
			return false;
	}

	return true;
}

//TODO: Account for cleaver, jarate and sandman

bool isAttacking(UserCmd* cmd, Entity* activeWeapon) noexcept
{
	if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
		|| localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
		return false;

	switch (activeWeapon->weaponId())
	{
		case WeaponId::PDA:
		case WeaponId::PDA_ENGINEER_BUILD:
		case WeaponId::PDA_ENGINEER_DESTROY:
		case WeaponId::PDA_SPY:
		case WeaponId::MEDIGUN:
			return false;
		default:
			break;
	}

	switch (activeWeapon->itemDefinitionIndex())
	{
	case Soldier_m_TheBeggarsBazooka:
		if (!(cmd->buttons & UserCmd::IN_ATTACK) && activeWeapon->clip() && activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime())
			return true;
		return false;
	case Sniper_m_TheMachina:
	case Sniper_m_ShootingStar:
	{
		if (!localPlayer->isScoped())
			return false;
		break;
	}
	default:
		break;
	}

	if (activeWeapon->weaponId() == WeaponId::COMPOUND_BOW || activeWeapon->weaponId() == WeaponId::PIPEBOMBLAUNCHER 
		|| activeWeapon->weaponId() == WeaponId::CANNON)
	{
		if (!(cmd->buttons & UserCmd::IN_ATTACK) && activeWeapon->chargeTime() > 0.0f)
			return true;
	}
	else
	{
		switch (activeWeapon->getWeaponType())
		{
		case WeaponType::HITSCAN:
			if (!activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() || activeWeapon->isInReload())
				return false;
			break;
		case WeaponType::PROJECTILE:
			if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
				return false;
			break;
		case WeaponType::MELEE:
			if (!activeWeapon->isKnife())
				return fabsf(activeWeapon->smackTime() - memory->globalVars->serverTime()) < memory->globalVars->intervalPerTick * 2.0f;
			if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
				return false;
			break;
		default:
			return false;
		}
		return cmd->buttons & UserCmd::IN_ATTACK;
	}

	return false;
}