#include "AttributeManager.h"
#include "Entity.h"

#include "../Memory.h"

float AttributeManager::attributeHookFloat(float value, const char* attributeHook, Entity* entity, void* itemList, bool isGlobalConstString) noexcept
{
	return memory->attributeHookValue(value, attributeHook, entity, itemList, isGlobalConstString);
}