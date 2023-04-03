#pragma once
#include <cstddef>

#include "UtlVector.h"

class Entity;

namespace AttributeManager
{
	float attributeHookFloat(float value, const char* attributeHook, Entity* entity, void* itemList = NULL, bool isGlobalConstString = false) noexcept;
}

#define ATTRIB_HOOK_FLOAT( retval, hookName ) \
	retval = AttributeManager::attributeHookFloat(retval, #hookName, this, NULL, true); \