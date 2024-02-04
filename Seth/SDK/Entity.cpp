#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../StrayElements.h"

#include "AttributeManager.h"
#include "Entity.h"
#include "GlobalVars.h"

std::string Entity::getPlayerName() noexcept
{
	std::string playerName = "unknown";

	PlayerInfo playerInfo;
	if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
		return playerName;

	playerName = playerInfo.name;

	if (wchar_t wide[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, 128, wide, 128)) {
		if (wchar_t wideNormalized[128]; NormalizeString(NormalizationKC, wide, -1, wideNormalized, 128)) {
			if (char nameNormalized[128]; WideCharToMultiByte(CP_UTF8, 0, wideNormalized, -1, nameNormalized, 128, nullptr, nullptr))
				playerName = nameNormalized;
		}
	}

	playerName.erase(std::remove(playerName.begin(), playerName.end(), '\n'), playerName.cend());
	return playerName;
}

std::uint64_t Entity::getSteamId() noexcept
{
    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
        return 0;

    if (playerInfo.fakeplayer)
        return 0;

    return 0x0110000100000000ULL + playerInfo.friendsId;
}

bool Entity::isEnemy(Entity* entity) noexcept
{
    return StrayElements::friendlyFire() ? true : entity->teamNumber() != teamNumber();
}

//categorize position
Entity* Entity::calculateGroundEntity() noexcept
{
	auto tryTouchGroundInQuadrants =
		[](Entity* player, Vector minsSrc, Vector maxsSrc, const Vector& start, const Vector& end, unsigned int mask, int collisionGroup, Trace& pm)
	{
		Vector mins, maxs;

		float fraction = pm.fraction;
		Vector endpos = pm.endpos;

		// Check the -x, -y quadrant
		mins = minsSrc;
		maxs = Vector{ min(0.0f, maxsSrc.x), min(0.0f, maxsSrc.y), maxsSrc.z };
		interfaces->engineTrace->traceRay({ start, end, mins, maxs }, mask, TraceFilterIgnorePlayers{ player, collisionGroup }, pm);
		if (pm.entity && pm.plane.normal[2] >= 0.7)
		{
			pm.fraction = fraction;
			pm.endpos = endpos;
			return;
		}

		// Check the +x, +y quadrant
		mins = Vector{ max(0.0f, minsSrc.x), max(0.0f, minsSrc.y), minsSrc.z };
		maxs = maxsSrc;
		interfaces->engineTrace->traceRay({ start, end, mins, maxs }, mask, TraceFilterIgnorePlayers{ player, collisionGroup }, pm);
		if (pm.entity && pm.plane.normal[2] >= 0.7)
		{
			pm.fraction = fraction;
			pm.endpos = endpos;
			return;
		}

		// Check the -x, +y quadrant
		mins = Vector{ minsSrc.x, max(0.0f, minsSrc.y), minsSrc.z };
		maxs = Vector{ min(0.0f, maxsSrc.x), maxsSrc.y, maxsSrc.z };
		interfaces->engineTrace->traceRay({ start, end, mins, maxs }, mask, TraceFilterIgnorePlayers{ player, collisionGroup }, pm);
		if (pm.entity && pm.plane.normal[2] >= 0.7)
		{
			pm.fraction = fraction;
			pm.endpos = endpos;
			return;
		}

		// Check the +x, -y quadrant
		mins = Vector{ max(0.0f, minsSrc.x), minsSrc.y, minsSrc.z };
		maxs = Vector{ maxsSrc.x, min(0.0f, maxsSrc.y), maxsSrc.z };
		interfaces->engineTrace->traceRay({ start, end, mins, maxs }, mask, TraceFilterIgnorePlayers{ player, collisionGroup }, pm);
		if (pm.entity && pm.plane.normal[2] >= 0.7)
		{
			pm.fraction = fraction;
			pm.endpos = endpos;
			return;
		}

		pm.fraction = fraction;
		pm.endpos = endpos;
	};

	Trace pm;

	Vector playerMins = obbMins() * modelScale();
	Vector playerMaxs = obbMaxs() * modelScale();
	Vector position = getAbsOrigin();

	Vector point = position;
	point.z -= 2.0f;

	Vector bumpOrigin = position;

	float zvel = velocity()[2];
	bool movingUp = zvel > 0.0f;
	bool movingUpRapidly = zvel > 140.0f;
	float groundEntityVelZ = 0.0f;
	if (movingUpRapidly)
	{
		auto ground = getGroundEntity();
		if (ground)
		{
			groundEntityVelZ = ground->absVelocity().z;
			movingUpRapidly = (zvel - groundEntityVelZ) > 140.0f;
		}
	}

	// Was on ground, but now suddenly am not
	if (movingUpRapidly)
	{
		return nullptr;
	}

	// Try and move down.
	interfaces->engineTrace->traceRay({ bumpOrigin, point, playerMins, playerMaxs }, MASK_PLAYERSOLID, TraceFilterIgnorePlayers{ this, 8 }, pm);

	// Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
	if (!pm.entity || pm.plane.normal[2] < 0.7)
	{
		// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
		tryTouchGroundInQuadrants(this, playerMins, playerMaxs, bumpOrigin, point, MASK_PLAYERSOLID, 8, pm);

		if (!pm.entity || pm.plane.normal[2] < 0.7)
		{
			return nullptr;
		}
		else
		{
			return pm.entity;
		}
	}
	else
	{
		return pm.entity;
	}
}

bool Entity::canWeaponRandomCrit() noexcept
{
	float critChange = AttributeManager::attributeHookFloat(1, "mult_crit_chance", this);
	if (critChange == 0)
		return false;

	switch (weaponId())
	{
		case WeaponId::SNIPERRIFLE:
		case WeaponId::SNIPERRIFLE_DECAP:
		case WeaponId::SNIPERRIFLE_CLASSIC:
		case WeaponId::KNIFE:
		case WeaponId::MEDIGUN:
		case WeaponId::PDA:
		case WeaponId::PDA_ENGINEER_BUILD:
		case WeaponId::PDA_ENGINEER_DESTROY:
		case WeaponId::PDA_SPY:
		case WeaponId::BUILDER:
		case WeaponId::PDA_SPY_BUILD:
		case WeaponId::LUNCHBOX:
		case WeaponId::JAR:
		case WeaponId::BUFF_ITEM:
		case WeaponId::JAR_GAS:
		case WeaponId::JAR_MILK:
		case WeaponId::COMPOUND_BOW:
			return false;
		default:
			break;
	}

	//yes this is from ateris
	switch (itemDefinitionIndex())
	{
		case Spy_m_FestiveAmbassador:
		case Spy_m_TheAmbassador:
		case Scout_s_MadMilk:
		case Scout_s_MutatedMilk:
		case Engi_p_PDA:
		case Engi_p_ConstructionPDA:
		case Engi_p_DestructionPDA:
		case Engi_p_ConstructionPDAR:
		case Heavy_s_Sandvich:
		case Heavy_s_SecondBanana:
		case Heavy_s_TheDalokohsBar:
		case Heavy_s_FestiveSandvich:
		case Heavy_s_Fishcake:
		case Heavy_s_RoboSandvich:
		case Engi_m_FestiveFrontierJustice:
		case Engi_m_TheFrontierJustice:
		case Soldier_m_RocketJumper:
		case Soldier_t_TheHalfZatoichi:
		case Demoman_s_StickyJumper:
		case Demoman_t_TheEyelander:
		case Demoman_t_TheClaidheamhMor:
		case Demoman_t_ThePersianPersuader:
		case Demoman_t_UllapoolCaber:
		case Pyro_m_ThePhlogistinator:
		case Pyro_m_DragonsFury:
		case Pyro_s_ThermalThruster:
		case Pyro_s_TheManmelter:
		case Pyro_s_GasPasser:
		case Pyro_t_TheAxtinguisher:
		case Soldier_t_TheMarketGardener:
		case Spy_m_TheDiamondback:
		case Scout_s_TheFlyingGuillotine:
		case Scout_s_TheFlyingGuillotineG:
			return false;
		default:
			break;
	}

	return true;
}