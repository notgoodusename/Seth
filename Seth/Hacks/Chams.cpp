#include <cstring>
#include <functional>

#include "../Config.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../Interfaces.h"

#include "Backtrack.h"
#include "Chams.h"
#include "TargetSystem.h"

#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/KeyValues.h"

static Material* shaded;
static Material* flat;
static Material* glow;

static constexpr auto dispatchMaterial(int id) noexcept
{
    switch (id) {
    default:
    case 0: return shaded;
    case 1: return flat;
    case 2: return glow;
    }
}

static void initializeMaterials() noexcept
{
    {
        const auto kv = new KeyValues("VertexLitGeneric");
        kv->setString("$basetexture", "vgui/white_additive");
        kv->setString("$bumpmap", "vgui/white_additive");
        kv->setString("$selfillum", "1");
        kv->setString("$selfillumfresnel", "1");
        kv->setString("$selfillumfresnelminmaxexp", "[-0.25 1 1]");
        shaded = interfaces->materialSystem->createMaterial("shaded", kv);
    }

    {
        const auto kv = new KeyValues("UnlitGeneric");
        kv->setString("$basetexture", "vgui/white_additive");
        flat = interfaces->materialSystem->createMaterial("flat", kv);
    }

    {
        const auto kv = new KeyValues("UnlitGeneric");
        kv->setString("$additive", "1");
        kv->setString("$bumpmap", "models/player/shared/shared_normal");
        kv->setString("$envmap", "skybox/sky_dustbowl_01");
        kv->setString("$envmapfresnel", "1");
        kv->setString("$phong", "1");
        kv->setString("$phongfresnelranges", "[0 0.05 0.1]");
        kv->setString("$alpha", ".8");
        glow = interfaces->materialSystem->createMaterial("glow", kv);
    }
}

static bool areAnyMaterialsEnabled(Config::Chams chams)
{
    for (size_t i = 0; i < chams.materials.size(); i++)
    {
        if (chams.materials.at(i).enabled)
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

    const auto entity = interfaces->entityList->getEntity(info.entityIndex);
    if (localPlayer && entity)
    {
        auto modelNameOriginal = interfaces->modelInfo->getModelName(info.model);
        std::string_view modelName(modelNameOriginal);
        if (modelName.starts_with("models/buildables/"))
        {
            if (!entity->isDormant() && entity->isObject())
                renderBuilding(entity);
        }
        else if (modelName.starts_with("models/halloween/") || modelName.starts_with("models/items/"))
        {
            if (!entity->isDormant())
                renderWorld(entity);
        }
        else if (modelName.starts_with("models/bots/"))
        {
            if (!entity->isDormant() && !entity->isPlayer() && entity->isNPC())
                renderNPCs(entity);
        }
        else
        {
            if (!entity->isDormant() && entity->isPlayer())
                renderPlayer(entity);
        }
    }

    return appliedChams;
}

void Chams::renderBuilding(Entity* building) noexcept
{
    if (!building || building->objectCarried() || building->objectHealth() <= 0)
        return;

    if (!areAnyMaterialsEnabled(config->buildingChams.all))
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
    if (!areAnyMaterialsEnabled(config->worldChams.all) &&
        !areAnyMaterialsEnabled(config->worldChams.ammoPacks) &&
        !areAnyMaterialsEnabled(config->worldChams.healthPacks) &&
        !areAnyMaterialsEnabled(config->worldChams.other))
        return;

    if (!areAnyMaterialsEnabled(config->worldChams.all))
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
    applyChams(config->chams["NPCs"].materials);
}

void Chams::renderPlayer(Entity* player) noexcept
{
    const auto health = player->health();
    const auto maxHealth = player->getMaxHealth();
    
    if (player == localPlayer.get()) {
        applyChams(config->chams["Local player"].materials, health, maxHealth);
    } else if (player->isEnemy(localPlayer.get())) {
        applyChams(config->chams["Enemies"].materials, health, maxHealth);
        if (config->backtrack.enabled)
        {
            if (const auto& playerTarget = TargetSystem::playerByHandle(player->handle()))
            {
                const auto& records = playerTarget->playerData;
                if (!records.empty() && records.size() >= 4U)
                {
                    int lastTick = -1;

                    for (int i = static_cast<int>(records.size() - 1U); i >= 3; i--)
                    {
                        const auto& targetTick = records[i];
                        if (player->simulationTime() > targetTick.simulationTime
                            && Backtrack::valid(targetTick.simulationTime)
                            && targetTick.origin != player->origin())
                        {
                            if (!appliedChams)
                                hooks->modelRender.callOriginal<void, 19>(state, info, customBoneToWorld);
                            applyChams(config->chams["Backtrack"].materials, health, maxHealth, targetTick.matrix.data());
                            interfaces->modelRender->forcedMaterialOverride(nullptr);
                        }
                    }
                }
            }
        }
    } else {
        applyChams(config->chams["Allies"].materials, health, maxHealth);
    }
}

void Chams::applyChams(const std::array<Config::Chams::Material, 7>& chams, int health, int maxHealth, const matrix3x4* customMatrix) noexcept
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

        if (material == glow)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            interfaces->renderView->setColorModulation(r, g, b);

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currentTime * 5) * 0.5f + 0.5f : 1.0f);

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

        const auto pulse = cham.color[3] * (cham.blinking ? std::sin(memory->globalVars->currentTime * 5) * 0.5f + 0.5f : 1.0f);

        if (material == glow)
            material->findVar("$envmaptint")->setVectorValue(r, g, b);
        else
            interfaces->renderView->setColorModulation(r, g, b);

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
