#include "SkinChanger.h"

#include "../SDK/Attribute.h"
#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/ItemSchema.h"
#include "../SDK/LocalPlayer.h"

itemSetting* getByDefinitionIndex(int index) noexcept
{
    const auto it = std::find_if(config->skinChanger.begin(), config->skinChanger.end(),
        [index](const itemSetting& e) { return e.enabled && e.itemId == index; });
    return it == config->skinChanger.end() ? nullptr : &*it;
}

static void applyConfigOnAttributableItem(Entity* item, const itemSetting& config) noexcept
{
    auto attributeList = reinterpret_cast<AttributeList*>(item + 0x9C4);
    if (!attributeList || attributeList->attributes.size > 0)
        return;

    if (config.festivizied)
        attributeList->addAttribute(AttributeId::isFestive, true);

    if (config.australium)
        attributeList->addAttribute(AttributeId::itemStyleOverride, true);

    if (config.unusualEffect)
        attributeList->addAttribute(AttributeId::unusualEffect,
            static_cast<float>(config.unusualEffect == 1 ? 4 : config.unusualEffect + 699));

    if (config.particleEffect)
        attributeList->addAttribute(AttributeId::particleEffect,
            static_cast<float>(config.particleEffect == 1 ? 4 : config.particleEffect + 699));

    if (config.killStreak)
        attributeList->addAttribute(AttributeId::killStreak, static_cast<float>(config.killStreak + 2001));

    if (config.killStreakTier)
        attributeList->addAttribute(AttributeId::killStreakTier, static_cast<float>(config.killStreakTier));

    if (config.sheen)
        attributeList->addAttribute(AttributeId::sheen, static_cast<float>(config.sheen));

    if (auto& definitionIndex = item->itemDefinitionIndex(); config.definitionOverrideIndex) {
        definitionIndex = config.definitionOverrideIndex;
        if (const auto definition = memory->itemSchema()->getItemDefinition(definitionIndex)) {
            item->preDataUpdate(0);
        }
    }
}

void SkinChanger::run() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    auto& weapons = localPlayer->weapons();

    for (auto weaponHandle : weapons) {
        if (weaponHandle == -1)
            continue;

        auto weapon = interfaces->entityList->getEntityFromHandle(weaponHandle);
        if (!weapon)
            continue;

        auto& index = weapon->itemDefinitionIndex();

        if (const auto activeConfig = getByDefinitionIndex(index))
            applyConfigOnAttributableItem(weapon, *activeConfig);
    }
}