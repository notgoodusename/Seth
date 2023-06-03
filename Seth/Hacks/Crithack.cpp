#include "Crithack.h"

#include "../SDK/AttributeManager.h"
#include "../SDK/Cvar.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/MD5.h"

#include "../Config.h"
#include "../Interfaces.h"

//A mix of these 2 sources
//https://github.com/nullworks/cathook/blob/master/src/crits.cpp
//https://www.unknowncheats.me/forum/team-fortress-2-a/430169-perfect-crithack-engine-manipulation.html

std::deque<int> critTicks;
std::deque<int> skipTicks;

bool protectData{ false };

bool canWeaponRandomCrit(Entity* activeWeapon) noexcept
{
	float critChange = AttributeManager::attributeHookFloat(1, "mult_crit_chance", activeWeapon, 0, 1);
	if (critChange == 0)
		return false;
	return true;
}

bool isCrit(Entity* activeWeapon) noexcept
{
	return activeWeapon->getWeaponType() == WeaponType::MELEE ? memory->calcIsAttackCriticalHelperMelee(activeWeapon) : memory->calcIsAttackCriticalHelper(activeWeapon);
}

bool isPureCritCommand(int commandNumber) noexcept
{
	return true;
}

void saveWeaponData(Entity* activeWeapon) noexcept
{
	//Allocate memory fuck it

}

void restoreWeaponData(Entity* activeWeapon) noexcept
{

}

void nextCritTicks(UserCmd* cmd, Entity* activeWeapon, int amountToAdd) noexcept
{
	//Reset

	if (critTicks.size() >= timeToTicks(4.0f))
		return;

	static int startingNumber = cmd->commandNumber;

	const auto seedBackup = *memory->predictionRandomSeed;
	for (int i = 1; i <= amountToAdd; i++)
	{
		int commandNumber = startingNumber + i - 1;
		*memory->predictionRandomSeed = MD5_PseudoRandom(commandNumber) & MASK_SIGNED;

		saveWeaponData(activeWeapon);
		
		if (isCrit(activeWeapon))
			critTicks.push_back(commandNumber);
		else
			skipTicks.push_back(commandNumber);

		restoreWeaponData(activeWeapon);
	}

	startingNumber += amountToAdd + 1;
	 
	*memory->predictionRandomSeed = seedBackup;
}

void Crithack::run(UserCmd* cmd) noexcept
{
	static auto weaponCriticals = interfaces->cvar->findVar("tf_weapon_criticals");
	if (!config->misc.critHack || !config->misc.forceCritHack.isActive() || weaponCriticals->getInt() <= 0)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (!isAttacking(cmd, activeWeapon) || localPlayer->isCritBoosted())
		return;

	if (!canWeaponRandomCrit(activeWeapon))
		return;
	
	nextCritTicks(cmd, activeWeapon, timeToTicks(1.0f));

	if (config->misc.forceCritHack.isActive())
	{
		cmd->commandNumber = critTicks.front();
		cmd->commandNumber = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
		critTicks.pop_front();
	}
	else
	{
		cmd->commandNumber = skipTicks.front();
		cmd->commandNumber = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
		skipTicks.pop_front();
	}
}

void Crithack::reset() noexcept
{

}