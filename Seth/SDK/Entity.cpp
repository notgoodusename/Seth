#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"

#include "Entity.h"
#include "GlobalVars.h"

std::string Entity::getPlayerName() noexcept
{
	PlayerInfo pi{};
	interfaces->engine->getPlayerInfo(index(), pi);
	return std::string(pi.name);
}