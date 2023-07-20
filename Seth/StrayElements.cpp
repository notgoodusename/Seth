#include "StrayElements.h"

TFPlayerResource* playerResource = nullptr;

TFPlayerResource*& StrayElements::getPlayerResource() noexcept
{
	return playerResource;
}

void StrayElements::clear() noexcept
{
	playerResource = nullptr;
}