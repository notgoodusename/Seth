#include <cstring>
#include <functional>

#include "../Config.h"
#include "../Helpers.h"
#include "../Hooks.h"
#include "../Interfaces.h"

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

    const auto entity = interfaces->entityList->getEntity(info.entityIndex);
    if (entity && !entity->isDormant())
    {
        if (entity->isPlayer())
            renderPlayer(entity);
    }

    return appliedChams;
}

void Chams::renderPlayer(Entity* player) noexcept
{
    if (!localPlayer)
        return;

    const auto health = player->health();
    
    if (player == localPlayer.get()) {
        applyChams(config->chams["Local player"].materials, health);
    } else if (player->isEnemy(localPlayer.get())) {
        applyChams(config->chams["Enemies"].materials, health);
    } else {
        applyChams(config->chams["Allies"].materials, health);
    }
}

void Chams::applyChams(const std::array<Config::Chams::Material, 7>& chams, int health, const matrix3x4* customMatrix) noexcept
{
    for (const auto& cham : chams) {
        if (!cham.enabled || !cham.ignorez)
            continue;

        const auto material = dispatchMaterial(cham.material);
        if (!material)
            continue;
        
        float r, g, b;
        if (cham.healthBased && health) {
            Helpers::healthColor(std::clamp(health / 100.0f, 0.0f, 1.0f), r, g, b);
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
            Helpers::healthColor(std::clamp(health / 100.0f, 0.0f, 1.0f), r, g, b);
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
