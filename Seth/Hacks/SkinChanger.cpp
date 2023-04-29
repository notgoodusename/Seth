#include "SkinChanger.h"

#include "../SDK/Attribute.h"
#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"

void SkinChanger::run() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto myWeapons = reinterpret_cast<size_t*>(localPlayer.get() + 0xCF8);
    for (int n = 0; myWeapons[n]; n++)
    {
        auto weapon = reinterpret_cast<Entity*>(interfaces->entityList->getEntityFromHandle(myWeapons[n]));
        if (!weapon)
            continue;

        auto attributeList = reinterpret_cast<AttributeList*>(weapon + 0x9C4);
        if (!attributeList || attributeList->attributes.size > 0)
            continue;
        for (auto& item : config->skinChanger.attributes)
        {
            if (weapon->itemDefinitionIndex() != item.fromIndex)
                continue;

            weapon->itemDefinitionIndex() = item.toIndex;

            if (item.ancientPowers)
                attributeList->addAttribute(AttributeId::ancientPowers, true);

            if (item.styleOverride)
                attributeList->addAttribute(AttributeId::itemStyleOverride, true);

            if (item.effect)
                attributeList->addAttribute(AttributeId::unusualEffect, static_cast<float>(item.effect));

            if (item.particleEffect)
                attributeList->addAttribute(AttributeId::particleEffect, static_cast<float>(item.particleEffect));

            if (item.sheen)
                attributeList->addAttribute(AttributeId::sheen, static_cast<float>(item.sheen));

            //Add random attribute so it doesnt overwrite other weapons
            attributeList->addAttribute(55, 0.0f);
        }
    }
}

void SkinChanger::forceFullUpdate() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    memory->clientState->deltaTick = -1;
}