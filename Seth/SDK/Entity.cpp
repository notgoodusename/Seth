#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../StrayElements.h"

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