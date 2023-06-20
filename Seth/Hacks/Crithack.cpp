#include "Crithack.h"

#include "../SDK/AttributeManager.h"
#include "../SDK/Cvar.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/MD5.h"
#include "../SDK/MemAlloc.h"

#include "../Config.h"
#include "../Interfaces.h"

//A mix of these 2 sources
//https://github.com/nullworks/cathook/blob/master/src/crits.cpp
//https://www.unknowncheats.me/forum/team-fortress-2-a/430169-perfect-crithack-engine-manipulation.html

std::unordered_map<int, std::deque<int>> critTicks;
std::unordered_map<int, std::deque<int>> skipTicks;

bool protect{ false };
bool outOfSync{ false };
int cachedDamage{ 0 };
int critDamage{ 0 };
int meleeDamage{ 0 };
int roundDamage{ 0 };

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

int decryptOrEncryptSeed(Entity* activeWeapon, const int seed) {

	if (!activeWeapon)
		return 0;

	int extra = activeWeapon->index() << 8 | localPlayer->index();

	if (activeWeapon->slot() == WeaponSlots::SLOT_MELEE)
		extra <<= 8;

	return extra ^ seed;
}

bool isPureCritCommand(Entity* activeWeapon, int commandNumber, int range, bool lower) noexcept
{
	const auto randomSeed = MD5_PseudoRandom(commandNumber) & MASK_SIGNED;

	memory->randomSeed(decryptOrEncryptSeed(activeWeapon, randomSeed));

	return lower ? memory->randomInt(0, 9999) < range : memory->randomInt(0, 9999) > range;
}	

int maxPossibleCritsSize() noexcept
{
	//TODO: complete this
	return 3;
}

//https://github.com/nullworks/cathook/blob/master/src/crits.cpp#L500
void updateCmds(UserCmd* cmd, Entity* activeWeapon) noexcept
{
	//dont have this weapon in anywhere :/ - so its invalid
	bool invalidCritCmds = critTicks.find(activeWeapon->index()) == critTicks.end();
	bool invalidSkipCmds = skipTicks.find(activeWeapon->index()) == skipTicks.end();

	//clear old cmds
	std::erase_if(critTicks[activeWeapon->index()],
		[&](const auto& commandNumber) { return commandNumber < cmd->commandNumber; });
	std::erase_if(skipTicks[activeWeapon->index()],
		[&](const auto& commandNumber) { return commandNumber < cmd->commandNumber; });

	if (invalidCritCmds || invalidSkipCmds || static_cast<int>(critTicks.size()) < maxPossibleCritsSize())
	{
		//fill at max 3 crits
		for (int i = cmd->commandNumber + 200, limitTillEnd = 3; i < cmd->commandNumber + 100000 + 200 && limitTillEnd > 0; i++)
		{
			if (isPureCritCommand(activeWeapon, i, 1, true))
			{
				critTicks[activeWeapon->index()].push_back(i);
				limitTillEnd--;
			}
			else if (isPureCritCommand(activeWeapon, i, 9998, false))
				skipTicks[activeWeapon->index()].push_back(i);

			if (limitTillEnd == 0)
				break;
		}
	}
}

int nextTick(UserCmd* cmd, Entity* activeWeapon, bool skipTick = false, int loops = 4096) noexcept
{
	const auto seedBackup = *memory->predictionRandomSeed;

	const auto backupStart = 0xA54/*m_flCritTokenBucket*/;
	const auto backupEnd = 0xB60/*m_flLastRapidFireCritCheckTime*/ + sizeof(float);
	const auto backupSize = backupEnd - backupStart;

	static void* backup = memory->memAlloc->Alloc(backupSize);

	//backup weapon state
	memcpy(backup, static_cast<void*>(activeWeapon + backupStart), backupSize);

	int bestCommandNumber = -1;

	protect = true;
	for (int i = 1; i <= loops; i++)
	{
		int commandNumber = cmd->commandNumber + i;
		*memory->predictionRandomSeed = MD5_PseudoRandom(commandNumber) & MASK_SIGNED;

		if (!isCrit(activeWeapon) && skipTick)
			bestCommandNumber = commandNumber;
		else if (isCrit(activeWeapon))
			bestCommandNumber = commandNumber;

		//restore weapon state
		memcpy(static_cast<void*>(activeWeapon + backupStart), backup, backupSize);

		if (bestCommandNumber != -1)
			break;
	}
	protect = false;
	*memory->predictionRandomSeed = seedBackup;

	return bestCommandNumber;
}

void Crithack::run(UserCmd* cmd) noexcept
{
	static auto weaponCriticals = interfaces->cvar->findVar("tf_weapon_criticals");
	if (!config->misc.critHack || weaponCriticals->getInt() <= 0)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (!canWeaponRandomCrit(activeWeapon))
		return;

	updateCmds(cmd, activeWeapon);
	
	if (!canAttack(cmd, activeWeapon) || !isAttacking(cmd, activeWeapon))
		return;

	if (localPlayer->isCritBoosted())
		return;

	if (config->misc.forceCritHack.isActive())
	{
		if (activeWeapon->getWeaponType() == WeaponType::MELEE || activeWeapon->weaponId() == WeaponId::PIPEBOMBLAUNCHER || critTicks[activeWeapon->index()].empty())
		{
			if (auto critCommandNumber = nextTick(cmd, activeWeapon) != -1)
			{
				cmd->commandNumber = critCommandNumber;
				cmd->randomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
			}
		}
		else
		{
			auto critCommandNumber = critTicks[activeWeapon->index()].front();
			cmd->commandNumber = critCommandNumber;
			cmd->randomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
			critTicks[activeWeapon->index()].pop_front();
		}
	}
	else
	{
		if (skipTicks[activeWeapon->index()].empty())
		{
			if (auto skipCommandNumber = nextTick(cmd, activeWeapon, true, 512) != -1)
			{
				cmd->commandNumber = skipCommandNumber;
				cmd->randomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
			}
		}
		else
		{
			cmd->commandNumber = skipTicks[activeWeapon->index()].front();
			cmd->randomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
			skipTicks[activeWeapon->index()].pop_front();
		}
	}
}

bool Crithack::protectData() noexcept
{
	return protect;
}

void Crithack::reset() noexcept
{
	critTicks.clear();
	skipTicks.clear();
	protect = false;
}