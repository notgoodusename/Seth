#include "AutoDetonate.h"
#include "../TargetSystem.h"

#include "../../SDK/AttributeManager.h"
#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"

void AutoDetonate::run(Entity* activeWeapon, UserCmd* cmd) noexcept
{
	const auto& cfg = config->autoDetonate;
	if (!cfg.enabled)
		return;

	static auto grenadeLauncherLiveTime = interfaces->cvar->findVar("tf_grenadelauncher_livetime");
	static auto stickyRadiusRampTime = interfaces->cvar->findVar("tf_sticky_radius_ramp_time");
	static auto stickyAirdetRadius = interfaces->cvar->findVar("tf_sticky_airdet_radius");

	Entity* stickyLauncher = nullptr;

	auto& weapons = localPlayer->weapons();

	for (auto weaponHandle : weapons)
	{
		if (weaponHandle == -1)
			continue;

		auto weapon = interfaces->entityList->getEntityFromHandle(weaponHandle);
		if (!weapon)
			continue;

		const auto weapondId = weapon->weaponId();

		switch (weapon->weaponId())
		{
		case WeaponId::PIPEBOMBLAUNCHER:
			stickyLauncher = weapon;
			break;
		default:
			break;
		}
	}

	if (!stickyLauncher)
		return;

	const float armTime = grenadeLauncherLiveTime->getFloat();
	const float radiusRampTime = stickyRadiusRampTime->getFloat();
	const float airdetRadius = stickyAirdetRadius->getFloat();
	const float stickyArmTime = AttributeManager::attributeHookFloat(grenadeLauncherLiveTime->getFloat(), "sticky_arm_time", localPlayer.get());

	const auto& localStickiesHandles = TargetSystem::localStickiesHandles();
	if (localStickiesHandles.empty())
		return;

	const auto& enemies = TargetSystem::playerTargets();

	const bool ignoreCloaked = (cfg.ignore & 1 << 0) == 1 << 0;
	const bool ignoreInvulnerable = (cfg.ignore & 1 << 1) == 1 << 1;
	for (const auto& stickyHandle : localStickiesHandles)
	{
		auto sticky{ interfaces->entityList->getEntityFromHandle(stickyHandle) };
		if (!sticky)
			continue;

		if (sticky->type() == 2)
			continue;

		const float creationTime = sticky->creationTime();

		if (memory->globalVars->serverTime() <= creationTime + stickyArmTime)
			continue;

		float radius = AttributeManager::attributeHookFloat(sticky->damageRadius(), "mult_explosion_radius", stickyLauncher);
		if (!sticky->touched())
			radius *= Helpers::remapValClamped(memory->globalVars->serverTime() - creationTime, armTime, armTime + radiusRampTime, airdetRadius, 1.0f);

		for (const auto& target : enemies)
		{
			if (target.playerData.empty() || !target.isAlive || target.priority == 0)
				continue;

			auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
			if (!entity ||
				(entity->isCloaked() && ignoreCloaked) ||
				(entity->isInvulnerable() && ignoreInvulnerable) ||
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
				if (!cfg.silent)
					interfaces->engine->setViewAngles(cmd->viewangles);
			}
			cmd->buttons |= UserCmd::IN_ATTACK2;
			break;
		}
	}
}