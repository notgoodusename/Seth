#include <cstring>
#include <functional>

#include "../Config.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../Interfaces.h"

#include "Animations.h"
#include "Backtrack.h"
#include "Chams.h"

#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/KeyValues.h"

static Material* normal;
static Material* flat;

static constexpr auto dispatchMaterial(int id) noexcept
{
    switch (id) {
    default:
    case 0: return normal;
    case 1: return flat;
    }
}

static void initializeMaterials() noexcept
{
    {
        const auto kv = new KeyValues("VertexLitGeneric");
        normal = interfaces->materialSystem->createMaterial("normal", kv);
    }

    {
        const auto kv = new KeyValues("UnlitGeneric");
        flat = interfaces->materialSystem->createMaterial("flat", kv);
    }
}

static bool areAnyMaterialsEnabled(std::array<Config::Chams::Material, 7> materials)
{
    for (size_t i = 0; i < materials.size(); i++)
    {
        if (materials.at(i).enabled)
            return true;
    }
    return false;
}

void Chams::updateInput() noexcept
{
    config->chamsKey.handleToggle();
}

bool Chams::render(void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    if (!config->chamsKey.isActive())
        return false;

    static bool materialsInitialized = false;
    if (!materialsInitialized) {
        initializeMaterials();
        materialsInitialized = true;
    }

    appliedChams = false;
    this->state = state;
    this->info = &info;
    this->customBoneToWorld = customBoneToWorld;


    auto modelNameOriginal = interfaces->modelInfo->getModelName(info.model);
    std::string_view modelName(modelNameOriginal);

    if (modelName.starts_with("models/buildables/"))
    {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant())
            renderBuilding(entity);
    }
    else if(modelName.starts_with("models/halloween/") || modelName.starts_with("models/items/"))
    {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant())
            renderWorld(entity);
    }
    else if (modelName.starts_with("models/bots/"))
    {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant() && !entity->isPlayer())
            renderNPCs(entity);
    }
    else
    {
        const auto entity = interfaces->entityList->getEntity(info.entityIndex);
        if (entity && !entity->isDormant() && entity->isPlayer())
            renderPlayer(entity);
    }

    return appliedChams;
}

void Chams::renderBuilding(Entity* building) noexcept
{
    if (!localPlayer)
        return;

    if (building->objectCarried() || building->objectHealth() <= 0)
        return;

    if (!areAnyMaterialsEnabled(config->buildingChams.all.materials))
    {
        if (building->isEnemy(localPlayer.get()))
            applyChams(config->buildingChams.enemies.materials, building->objectHealth(), building->objectMaxHealth());
        else
            applyChams(config->buildingChams.allies.materials, building->objectHealth(), building->objectMaxHealth());
    }
    else
    {
        applyChams(config->buildingChams.all.materials, building->objectHealth(), building->objectMaxHealth());
    }
}

void Chams::renderWorld(Entity* worldEntity) noexcept
{
    if (!localPlayer)
        return;

    if (!areAnyMaterialsEnabled(config->worldChams.all.materials) &&
        !areAnyMaterialsEnabled(config->worldChams.ammoPacks.materials) &&
        !areAnyMaterialsEnabled(config->worldChams.healthPacks.materials) &&
        !areAnyMaterialsEnabled(config->worldChams.other.materials))
        return;

    if (!areAnyMaterialsEnabled(config->worldChams.all.materials))
    {
        switch (fnv::hashRuntime(worldEntity->getModelName()))
        {
            case fnv::hash("models/items/medkit_small.mdl"):
            case fnv::hash("models/items/medkit_small_bday.mdl"):
            case fnv::hash("models/props_halloween/halloween_medkit_small.mdl"):
            case fnv::hash("models/items/medkit_medium.mdl"):
            case fnv::hash("models/items/medkit_medium_bday.mdl"):
            case fnv::hash("models/props_halloween/halloween_medkit_medium.mdl"):
            case fnv::hash("models/items/medkit_large.mdl"):
            case fnv::hash("models/items/medkit_large_bday.mdl"):
            case fnv::hash("models/props_halloween/halloween_medkit_large.mdl"):
                applyChams(config->worldChams.healthPacks.materials);
                break;
            case fnv::hash("models/items/ammopack_small.mdl"):
            case fnv::hash("models/items/ammopack_small_bday.mdl"):
            case fnv::hash("models/items/ammopack_medium.mdl"):
            case fnv::hash("models/items/ammopack_medium_bday.mdl"):
            case fnv::hash("models/items/ammopack_large.mdl"):
            case fnv::hash("models/items/ammopack_large_bday.mdl"):
                applyChams(config->worldChams.ammoPacks.materials);
                break;
            default:
                applyChams(config->worldChams.other.materials);
                break;
        }
    }
    else
    {
        applyChams(config->worldChams.all.materials);
    }
}

void Chams::renderNPCs(Entity* npc) noexcept
{
    if (!localPlayer)
        return;

    applyChams(config->chams["NPCs"].materials);
}

void Chams::renderPlayer(Entity* player) noexcept
{
    if (!localPlayer)
        return;

    const auto health = player->health();
    const auto maxHealth = player->getMaxHealth();
    
    if (player == localPlayer.get()) {
        applyChams(config->chams["Local player"].materials, health, maxHealth);
    } else if (player->isEnemy(localPlayer.get())) {
        applyChams(config->chams["Enemies"].materials, health, maxHealth);
        if (config->backtrack.enabled)
        {
            const auto records = Animations::getBacktrackRecords(player->index());
            if (records && !records->empty() && records->size() >= 4U)
            {
                int lastTick = -1;

                for (int i = static_cast<int>(records->size() - 1U); i >= 3; i--)
                {
                    if (player->simulationTime() > records->at(i).simulationTime && Backtrack::valid(records->at(i).simulationTime) && records->at(i).origin != player->origin())
                    {
                        if (!appliedChams)
                            hooks->modelRender.callOriginal<void, 19>(state, info, customBoneToWorld);
                        applyChams(config->chams["Backtrack"].materials, health, maxHealth, records->at(i).matrix);
                        interfaces->modelRender->forcedMaterialOverride(nullptr);
                    }
                }
            }
        }
    } else {
        applyChams(config->chams["Allies"].materials, health, maxHealth);
    }
}

void Chams::applyChams(const std::array<Config::Chams::Material, 7>& chams, int health, int maxHealth,const matrix3x4* customMatrix) noexcept
{
    for (const auto& cham : chams) {
        if (!cham.enabled || !cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;
        
        float r, g, b;
        if (cham.healthBased && health) {
            Helpers::healthColor(std::clamp(health / static_cast<float>(maxHealth), 0.0f, 1.0f), r, g, b);
        } else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        } else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        interfaces->renderView->setColorModulation(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);

        interfaces->renderView->setBlend(pulse);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, true);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->modelRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 19>(state, info, customMatrix ? customMatrix : customBoneToWorld);
        interfaces->modelRender->forcedMaterialOverride(nullptr);
    }

    for (const auto& cham : chams) {
        if (!cham.enabled || cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;

        float r, g, b;
        if (cham.healthBased && health) {
            Helpers::healthColor(std::clamp(health / static_cast<float>(maxHealth), 0.0f, 1.0f), r, g, b);
        } else if (cham.rainbow) {
            std::tie(r, g, b) = rainbowColor(cham.rainbowSpeed);
        } else {
            r = cham.color[0];
            g = cham.color[1];
            b = cham.color[2];
        }

        interfaces->renderView->setColorModulation(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currenttime * 5) * 0.5f + 0.5f : 1.0f);

        interfaces->renderView->setBlend(pulse);

        if (cham.cover && !appliedChams)
            hooks->modelRender.callOriginal<void, 19>(state, info, customMatrix ? customMatrix : customBoneToWorld);

        material->setMaterialVarFlag(MaterialVarFlag::IGNOREZ, false);
        material->setMaterialVarFlag(MaterialVarFlag::WIREFRAME, cham.wireframe);
        interfaces->modelRender->forcedMaterialOverride(material);
        hooks->modelRender.callOriginal<void, 19>(state, info, customMatrix ? customMatrix : customBoneToWorld);
        appliedChams = true;
    }
}
