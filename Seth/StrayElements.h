#pragma once
#include "SDK/PlayerResource.h"

//I have pointers that should belong elsewhere but couldnt do it cuz i cant get the proper pattern
namespace StrayElements
{
	TFPlayerResource*& getPlayerResource() noexcept;
	bool friendlyFire() noexcept;
	void clear() noexcept;
}