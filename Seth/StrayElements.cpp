#include "StrayElements.h"

#include "../Interfaces.h"

#include "SDK/ConVar.h"
#include "SDK/Cvar.h"

TFPlayerResource* playerResource = nullptr;

TFPlayerResource*& StrayElements::getPlayerResource() noexcept
{
	return playerResource;
}

bool StrayElements::friendlyFire() noexcept
{
	static auto friendlyFireConvar = interfaces->cvar->findVar("mp_friendlyfire");
	return friendlyFireConvar->getInt() != 0;
}

void StrayElements::clear() noexcept
{
	playerResource = nullptr;
}