#include "AutoVaccinator.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"

#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Math.h"
#include "../SDK/Network.h"

#include "../GUI.h"

bool hasVaccinatorAtHand{ false };

int idealResistanceType{ -1 };
int simulatedResistanceType{ 0 };

bool shouldPop{ false };

static float lastBulletDanger{ 0.0f };
static float lastBlastDanger{ 0.0f };
static float lastFireDanger{ 0.0f };

float bulletDanger(Entity* healTarget) noexcept
{
	float danger = 0.0f;

	const auto network = interfaces->engine->getNetworkChannel();
	if (!network)
		return danger;

	const float latencyTime = max(0.0f, Backtrack::getRealTotalLatency());

	const auto healTargetOrigin = healTarget->origin();
	const auto healTargetCenter = healTargetOrigin + (healTarget->obbMins() + healTarget->obbMaxs()) * 0.5f;
	const auto healTargetHead = healTarget->getEyePosition();

	const float percentHealth = static_cast<float>(healTarget->health()) / static_cast<float>(healTarget->getMaxHealth());

	const auto& enemies = TargetSystem::playerTargets();
	for (const auto& target : enemies)
	{
		if (target.playerData.empty() || !target.isAlive)
			continue;

		auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
		if (!entity || entity->isCloaked() || !entity->isEnemy(localPlayer.get()))
			continue;

		const auto entityActiveWeapon = entity->getActiveWeapon();
		if (!entityActiveWeapon || entityActiveWeapon->getWeaponDamageType() != WeaponDamageType::BULLET)
			continue;

		const auto weaponId = entityActiveWeapon->weaponId();

		const auto& targetTick = target.playerData.back();

		const auto distanceFromHealTarget = healTarget->origin().distTo(targetTick.origin + (targetTick.mins + targetTick.maxs) * 0.5f);

		bool canHeadShot = entityActiveWeapon->canWeaponHeadshot();
		if (canHeadShot || entity->isCritBoosted() || entity->isMiniCritBoosted())
		{
			const auto eyePos = entity->getEyePosition();

			Trace trace;
			Trace trace2;

			interfaces->engineTrace->traceRay({ healTargetCenter, eyePos }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);
			interfaces->engineTrace->traceRay({ healTargetHead, eyePos }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace2);

			if (trace.entity != entity && trace.fraction <= 0.97f
				&& trace2.entity != entity && trace2.fraction <= 0.97f)
				continue;

			switch (weaponId)
			{
			case WeaponId::SNIPERRIFLE:
			case WeaponId::SNIPERRIFLE_DECAP:
				if (entity->isScoped())
					danger += entityActiveWeapon->chargedDamage() * 3.0f;
				else
				{
					danger += 50.0f;
					if (entity->isCritBoosted())
						danger *= 3.0f;
					else if (entity->isMiniCritBoosted())
						danger *= 1.35f;
				}

				break;
			case WeaponId::SNIPERRIFLE_CLASSIC:
				if (entityActiveWeapon->chargedDamage() >= 150.0f)
					danger += entityActiveWeapon->chargedDamage() * 3.0f;
				else
				{
					danger += 45.0f;
					if (entity->isCritBoosted())
						danger *= 3.0f;
					else if (entity->isMiniCritBoosted())
						danger *= 1.35f;
				}
				break;
			default:
				danger += Helpers::lerp(distanceFromHealTarget / 500.0f, 120.0f, 60.0f);
				if (entity->isCritBoosted())
					danger *= 3.0f;
				else if (entity->isMiniCritBoosted())
					danger *= 1.35f;
				break;
			}
		}
		else if (distanceFromHealTarget <= 500.0f)
		{
			danger += Helpers::lerp(distanceFromHealTarget / 500.0f, 120.0f, 70.0f);
			if (entity->isCritBoosted())
				danger *= 3.0f;
			else if (entity->isMiniCritBoosted())
				danger *= 1.35f;
		}
	}

	const auto& projectiles = TargetSystem::projectilesVector();
	for (const auto& projectile : projectiles)
	{
		if (projectile.classId == ClassId::TFProjectile_Jar
			|| projectile.classId == ClassId::TFProjectile_JarGas
			|| projectile.classId == ClassId::TFProjectile_JarMilk
			|| projectile.classId == ClassId::TFProjectile_Cleaver
			|| projectile.classId == ClassId::TFProjectile_Rocket
			|| projectile.classId == ClassId::TFProjectile_SentryRocket
			|| projectile.classId == ClassId::TFProjectile_EnergyBall
			|| projectile.classId == ClassId::TFProjectile_Flare
			|| projectile.classId == ClassId::TFProjectile_BallOfFire
			|| projectile.classId == ClassId::TFGrenadePipebombProjectile)
			continue;

		auto entity{ interfaces->entityList->getEntityFromHandle(projectile.handle) };
		if (!entity || !entity->isEnemy(localPlayer.get()))
			continue;

		Trace trace;
		Trace trace2;

		interfaces->engineTrace->traceRay({ healTargetCenter, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);
		interfaces->engineTrace->traceRay({ healTargetHead, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);

		if (trace.entity != entity && trace.fraction <= 0.97f
			&& trace2.entity != entity && trace2.fraction <= 0.97f)
			continue;

		Vector velocity = entity->getAbsVelocity();
		Vector origin = projectile.origin + velocity * latencyTime;

		const float distanceToHealTarget = origin.distTo(healTargetCenter);

		switch (projectile.classId)
		{
		case ClassId::TFProjectile_Arrow:
			if (velocity.null() || distanceToHealTarget > 150.0f)
				continue;

			danger += 120.0f;
			break;
		case ClassId::TFProjectile_HealingBolt:
			if (velocity.null() || distanceToHealTarget > 150.0f)
				continue;

			if (entity->isArrowCritical())
				danger += 120.0f;

			break;
		case ClassId::TFProjectile_EnergyRing:
			if (distanceToHealTarget > 150.0f)
				continue;

			if (percentHealth <= 0.85f)
				danger += Helpers::lerp(percentHealth / 0.85f, 60.0f, 20.0f);

			break;
		default:
			break;
		}
	}

	const auto& buildings = TargetSystem::buildingTargets();
	for (const auto& building : buildings)
	{
		auto entity{ interfaces->entityList->getEntityFromHandle(building.handle) };
		if (!entity || !entity->isEnemy(localPlayer.get()))
			continue;

		if (building.buildingType != 2)
			continue;

		if ((entity->sentryAutoAimTarget() == healTarget->handle()
			|| entity->sentryEnemy() == healTarget->handle())
			&& percentHealth <= 0.85f)
			danger += Helpers::lerp(percentHealth / 0.85f, 250.0f, 175.0f);
	}

	return danger;
}

float blastDanger(Entity* healTarget) noexcept
{
	float danger = 0.0f;

	const auto network = interfaces->engine->getNetworkChannel();
	if (!network)
		return danger;

	const float latencyTime = max(0.0f, Backtrack::getRealTotalLatency());

	const auto healTargetOrigin = healTarget->origin();
	const auto healTargetCenter = healTargetOrigin + (healTarget->obbMins() + healTarget->obbMaxs()) * 0.5f;
	const auto healTargetHead = healTargetOrigin + Vector{ 0.0f, 0.0f, healTarget->obbMaxs().z };

	const float percentHealth = static_cast<float>(healTarget->health()) / static_cast<float>(healTarget->getMaxHealth());

	const auto& enemies = TargetSystem::playerTargets();
	for (const auto& target : enemies)
	{
		if (target.playerData.empty() || !target.isAlive)
			continue;

		const auto& targetTick = target.playerData.back();

		auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
		if (!entity || entity->isCloaked() || !entity->isEnemy(localPlayer.get()))
			continue;

		const auto entityActiveWeapon = entity->getActiveWeapon();
		if (!entityActiveWeapon || entityActiveWeapon->getWeaponDamageType() != WeaponDamageType::BLAST)
			continue;

		const auto distanceFromHealTarget = healTarget->origin().distTo(targetTick.origin + (targetTick.mins + targetTick.maxs) * 0.5f);
		if (distanceFromHealTarget > 850.0f)
			continue;

		danger += Helpers::lerp(distanceFromHealTarget / 850.0f, 150.0f, 75.0f);

		if (entity->isCritBoosted())
			danger *= 3.0f;
		else if (entity->isMiniCritBoosted())
			danger *= 1.35f;
	}

	int closePipeboms = 0;

	const auto& projectiles = TargetSystem::projectilesVector();
	for (const auto& projectile : projectiles)
	{
		if (projectile.classId == ClassId::TFProjectile_Jar
			|| projectile.classId == ClassId::TFProjectile_JarGas
			|| projectile.classId == ClassId::TFProjectile_JarMilk
			|| projectile.classId == ClassId::TFProjectile_Cleaver
			|| projectile.classId == ClassId::TFProjectile_Arrow
			|| projectile.classId == ClassId::TFProjectile_HealingBolt
			|| projectile.classId == ClassId::TFProjectile_EnergyRing
			|| projectile.classId == ClassId::TFProjectile_Flare
			|| projectile.classId == ClassId::TFProjectile_BallOfFire)
			continue;

		auto entity{ interfaces->entityList->getEntityFromHandle(projectile.handle) };
		if (!entity || !entity->isEnemy(localPlayer.get()))
			continue;

		Trace trace;
		Trace trace2;

		interfaces->engineTrace->traceRay({ healTargetCenter, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);
		interfaces->engineTrace->traceRay({ healTargetHead, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);

		if (trace.entity != entity && trace.fraction <= 0.97f
			&& trace2.entity != entity && trace2.fraction <= 0.97f)
			continue;

		Vector velocity = entity->getAbsVelocity();

		Vector origin = projectile.origin + velocity * latencyTime;

		const float distanceToHealTarget = origin.distTo(healTargetCenter);

		switch (projectile.classId)
		{
		case ClassId::TFProjectile_Rocket:
		case ClassId::TFProjectile_SentryRocket:
		case ClassId::TFProjectile_EnergyBall:
			if (distanceToHealTarget > 250.0f)
				continue;

			if (entity->isRocketCritical())
				danger += 270.0f;
			else
				danger += 90.0f;

			break;
		case ClassId::TFGrenadePipebombProjectile:
			if (distanceToHealTarget > 250.0f)
				continue;

			if (entity->type() == 2)
				continue;

			if (entity->isPipeCritical())
				danger += 300.0f;
			else
				danger += 100.0f;

			closePipeboms++;

			break;
		default:
			break;
		}
	}

	return danger;
}

float fireDanger(Entity* healTarget) noexcept
{
	float danger = 0.0f;

	const auto network = interfaces->engine->getNetworkChannel();
	if (!network)
		return danger;

	const float latencyTime = max(0.0f, Backtrack::getRealTotalLatency());

	const auto healTargetOrigin = healTarget->origin();
	const auto healTargetCenter = healTargetOrigin + (healTarget->obbMins() + healTarget->obbMaxs()) * 0.5f;
	const auto healTargetHead = healTargetOrigin + Vector{ 0.0f, 0.0f, healTarget->obbMaxs().z };

	const float percentHealth = static_cast<float>(healTarget->health()) / static_cast<float>(healTarget->getMaxHealth());

	if (healTarget->isOnFire())
		danger += 50.0f;

	const auto& enemies = TargetSystem::playerTargets();
	for (const auto& target : enemies)
	{
		if (target.playerData.empty() || !target.isAlive)
			continue;

		const auto& targetTick = target.playerData.back();

		auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
		if (!entity || entity->isCloaked() || !entity->isEnemy(localPlayer.get()))
			continue;

		const auto entityActiveWeapon = entity->getActiveWeapon();
		if (!entityActiveWeapon || entityActiveWeapon->getWeaponDamageType() != WeaponDamageType::FIRE)
			continue;

		const auto distanceFromHealTarget = healTarget->origin().distTo(targetTick.origin + (targetTick.mins + targetTick.maxs) * 0.5f);
		if (distanceFromHealTarget > 450.0f)
			continue;

		danger += Helpers::lerp(distanceFromHealTarget / 450.0f, 150.0f, 50.0f);

		if (entity->isCritBoosted())
			danger *= 3.0f;
		else if (entity->isMiniCritBoosted())
			danger *= 1.35f;
	}

	const auto& projectiles = TargetSystem::projectilesVector();
	for (const auto& projectile : projectiles)
	{
		if (projectile.classId == ClassId::TFProjectile_Jar
			|| projectile.classId == ClassId::TFProjectile_JarGas
			|| projectile.classId == ClassId::TFProjectile_JarMilk
			|| projectile.classId == ClassId::TFProjectile_Cleaver
			|| projectile.classId == ClassId::TFProjectile_Arrow
			|| projectile.classId == ClassId::TFProjectile_HealingBolt
			|| projectile.classId == ClassId::TFProjectile_EnergyRing
			|| projectile.classId == ClassId::TFProjectile_Rocket
			|| projectile.classId == ClassId::TFProjectile_SentryRocket
			|| projectile.classId == ClassId::TFProjectile_EnergyBall
			|| projectile.classId == ClassId::TFGrenadePipebombProjectile)
			continue;

		auto entity{ interfaces->entityList->getEntityFromHandle(projectile.handle) };
		if (!entity || !entity->isEnemy(localPlayer.get()))
			continue;

		Trace trace;
		Trace trace2;

		interfaces->engineTrace->traceRay({ healTargetCenter, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);
		interfaces->engineTrace->traceRay({ healTargetHead, projectile.origin + (projectile.mins + projectile.maxs) * 0.5f }, MASK_SOLID, TraceFilterWorldCustom{ entity }, trace);

		if (trace.entity != entity && trace.fraction <= 0.97f
			&& trace2.entity != entity && trace2.fraction <= 0.97f)
			continue;

		Vector velocity = entity->getAbsVelocity();

		Vector origin = projectile.origin + velocity * latencyTime;

		const float distanceToHealTarget = origin.distTo(healTargetCenter);

		switch (projectile.classId)
		{
		case ClassId::TFProjectile_Flare:
			if (distanceToHealTarget > 150.0f)
				continue;

			if (entity->isFlareCritical() || healTarget->isOnFire())
				danger += 90.0f;

			break;

		case ClassId::TFProjectile_BallOfFire:
			if (distanceToHealTarget > 150.0f)
				continue;

			if (healTarget->isOnFire())
				danger += 90.0f;
			else
				danger += 30.0f;

			break;
		default:
			break;
		}
	}

	return danger;
}

int currentUbers(Entity* healTarget) noexcept
{
	int result = 0;

	if (healTarget->hasUberBulletResist())
		result += 1 << 0;

	if (healTarget->hasUberBlastResist())
		result += 1 << 1;

	if (healTarget->hasUberFireResist())
		result += 1 << 2;

	return result;
}

MedigunResistType checkResistType(Entity* healTarget, Entity* activeWeapon) noexcept
{
	if (!healTarget || healTarget->isInvulnerable() || healTarget->getMaxHealth() == 0)
		return NUM_RESISTS;

	const int ubers = currentUbers(healTarget);

	float bullet = bulletDanger(healTarget) * config->autoVaccinator.bulletSensibility;
	float blast = blastDanger(healTarget) * config->autoVaccinator.blastSensibility;
	float fire = fireDanger(healTarget) * config->autoVaccinator.fireSensibility;

	if ((ubers & 1 << 0) == 1 << 0)
		bullet *= 0.0f;

	if ((ubers & 1 << 1) == 1 << 1)
		blast *= 0.0f;

	if ((ubers & 1 << 2) == 1 << 2)
		fire *= 0.0f;

	lastBulletDanger = bullet;
	lastBlastDanger = blast;
	lastFireDanger = fire;

	if (bullet >= blast && bullet >= fire)
	{
		if (bullet >= config->autoVaccinator.bulletThreshold && activeWeapon->chargeLevel() >= 0.25f)
			shouldPop = healTarget == localPlayer.get() ? config->autoVaccinator.preserveSelf : true;

		return BULLET_RESIST;
	}

	if (blast >= bullet && blast >= fire)
	{
		if (blast >= config->autoVaccinator.blastThreshold && activeWeapon->chargeLevel() >= 0.25f)
			shouldPop = healTarget == localPlayer.get() ? config->autoVaccinator.preserveSelf : true;

		return BLAST_RESIST;
	}

	if (fire >= blast && fire >= bullet)
	{
		if (fire >= config->autoVaccinator.fireThreshold && activeWeapon->chargeLevel() >= 0.25f)
			shouldPop = healTarget == localPlayer.get() ? config->autoVaccinator.preserveSelf : true;

		return FIRE_RESIST;
	}

	return NUM_RESISTS;
}

void AutoVaccinator::run(UserCmd* cmd) noexcept
{
	static bool resetSimulated = false;
	if (!config->autoKey.isActive() || !config->autoVaccinator.enabled)
	{
		resetSimulated = true;
		shouldPop = false;

		idealResistanceType = -1;

		lastBulletDanger = 0.0f;
		lastBlastDanger = 0.0f;
		lastFireDanger = 0.0f;

		hasVaccinatorAtHand = false;
		return;
	}

	if (!localPlayer || !localPlayer->isAlive())
	{
		resetSimulated = true;
		shouldPop = false;

		idealResistanceType = -1;

		lastBulletDanger = 0.0f;
		lastBlastDanger = 0.0f;
		lastFireDanger = 0.0f;

		hasVaccinatorAtHand = false;
		return;
	}

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon || activeWeapon->weaponId() != WeaponId::MEDIGUN || activeWeapon->getMedigunChargeType() < 3)
	{
		resetSimulated = true;
		shouldPop = false;

		idealResistanceType = -1;

		lastBulletDanger = 0.0f;
		lastBlastDanger = 0.0f;
		lastFireDanger = 0.0f;
		hasVaccinatorAtHand = false;
		return;
	}

	hasVaccinatorAtHand = true;

	auto currentResistanceType = activeWeapon->getMedigunResistType();
	if (resetSimulated)
	{
		simulatedResistanceType = currentResistanceType;
		resetSimulated = false;
	}

	//this desyncs every once in a while  :C
	if (simulatedResistanceType != idealResistanceType && idealResistanceType != -1)
	{
		cmd->buttons &= ~UserCmd::IN_RELOAD;

		if (cmd->tickCount % 2 == 0)
		{
			cmd->buttons |= UserCmd::IN_RELOAD;

			simulatedResistanceType++;

			if (simulatedResistanceType > 2)
				simulatedResistanceType = 0;

		}
		return;
	}
	else
	{
		cmd->buttons &= ~UserCmd::IN_RELOAD;
		idealResistanceType = -1;
	}

	if (shouldPop)
	{
		shouldPop = false;
		cmd->buttons |= UserCmd::IN_ATTACK2;
		return;
	}

	auto healTarget{ interfaces->entityList->getEntityFromHandle(activeWeapon->healingTarget()) };

	if (!healTarget)
		healTarget = localPlayer.get();

	const int resistType = checkResistType(healTarget, activeWeapon);
	if (resistType != NUM_RESISTS)
		idealResistanceType = resistType;
}

void AutoVaccinator::draw(ImDrawList* drawList) noexcept
{
	if (!config->autoKey.isActive() || !config->autoVaccinator.enabled || !config->autoVaccinator.indicator)
		return;

	if (!hasVaccinatorAtHand && !gui->isOpen())
		return;

	if (config->autoVaccinator.indicatorPos != ImVec2{}) {
		ImGui::SetNextWindowPos(config->autoVaccinator.indicatorPos);
		config->autoVaccinator.indicatorPos = {};
	}

	ImGui::SetNextWindowSize({ 250.f, 0.f }, ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints({ 250.f, 0.f }, { 250.f, 1000.f });
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (!gui->isOpen())
		windowFlags |= ImGuiWindowFlags_NoInputs;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
	ImGui::Begin("Auto vaccinator indicator", nullptr, windowFlags);
	ImGui::PopStyleVar();

	ImGui::TextWrapped("Bullet danger: %.2f", lastBulletDanger);
	ImGui::TextWrapped("Blast danger: %.2f", lastBlastDanger);
	ImGui::TextWrapped("Fire danger: %.2f", lastFireDanger);

	ImGui::End();
}

void AutoVaccinator::reset() noexcept
{
	idealResistanceType = -1;
	simulatedResistanceType = 0;

	lastBulletDanger = 0.0f;
	lastBlastDanger = 0.0f;
	lastFireDanger = 0.0f;

	hasVaccinatorAtHand = false;
}