#include "TriggerbotProjectile.h"
#include "../TargetSystem.h"

#include "../../SDK/AttributeManager.h"
#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"

void TriggerbotProjectile::run(UserCmd* cmd) noexcept
{
	const auto& cfg = config->projectileTriggerbot.autoDetonate;
	if (!cfg.enabled)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	bool noStickyLauncher = true;

	auto& weapons = localPlayer->weapons();

	for (auto weaponHandle : weapons)
	{
		if (weaponHandle == -1)
			break;

		auto weapon = reinterpret_cast<Entity*>(interfaces->entityList->getEntityFromHandle(weaponHandle));
		if (!weapon)
			continue;

		if (weapon->weaponId() == WeaponId::PIPEBOMBLAUNCHER)
			noStickyLauncher = false;
	}

	if (noStickyLauncher)
		return;

	const float stickyArmTime = AttributeManager::attributeHookFloat(0.8f, "sticky_arm_time", localPlayer.get());

	const auto& localStickiesHandles = TargetSystem::localStickiesHandles();
	if (localStickiesHandles.empty())
		return;

	const bool ignoreCloaked = (cfg.ignore & 1 << 1) == 1 << 1;
	const bool ignoreInvulnerable = (cfg.ignore & 1 << 2) == 1 << 2;

	const auto& enemies = TargetSystem::playerTargets();

	for (const auto& stickyHandle : localStickiesHandles)
	{
		auto sticky{ interfaces->entityList->getEntityFromHandle(stickyHandle) };
		if (!sticky)
			continue;

		if (sticky->type() == 2)
			continue;

		float a = sticky->creationTime();
		float b = memory->globalVars->serverTime();

		if (memory->globalVars->serverTime() < a + stickyArmTime)
			continue;

		float radius = sticky->touched() ? 150.0f : 100.0f;

		for (const auto& target : enemies)
		{
			if (target.playerData.empty() || !target.isAlive || target.priority == 0)
				continue;

			auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
			if (!entity || 
				(entity->isCloaked() && ignoreCloaked) ||
				(entity->isUbered() && ignoreInvulnerable) ||
				(!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
				continue;

			const auto& targetTick = target.playerData.back();

			const auto stickyMins = sticky->obbMins();
			const auto stickyMaxs = sticky->obbMaxs();

			Vector worldSpaceCenterSticky = sticky->origin();
			worldSpaceCenterSticky.z += (stickyMins.z + stickyMaxs.z) * 0.5f;

			Vector worldSpaceCenterPlayer = targetTick.origin;
			worldSpaceCenterPlayer.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

			if (worldSpaceCenterSticky.distTo(worldSpaceCenterPlayer) >= radius)
				continue;

			Trace trace;
			interfaces->engineTrace->traceRay
			({ worldSpaceCenterSticky, worldSpaceCenterPlayer }, MASK_SOLID, TraceFilterWorldCustom{ localPlayer.get() }, trace);

			if (trace.entity != entity && trace.fraction <= 0.99f)
				continue;

			if (sticky->defensiveBomb())
			{
				const auto angle = Math::calculateRelativeAngle(localPlayer->getEyePosition(), worldSpaceCenterSticky, cmd->viewangles);

				cmd->viewangles += angle;
				if(!cfg.silent)
					interfaces->engine->setViewAngles(cmd->viewangles);
			}
			cmd->buttons |= UserCmd::IN_ATTACK2;
			break;
		}
	}
}