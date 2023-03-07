#include "Glow.h"

#include "../Config.h"

#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialRenderContext.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/Stencil.h"
#include "../SDK/Texture.h"
#include "../SDK/KeyValues.h"

struct GlowEntity
{
    Entity* entity;
    std::array<float, 4> color;
};

static std::vector<GlowEntity> customGlowEntities;
static std::unordered_map<int, bool> drawnGlowEntities;

static Material* normal;


static Texture* fullFrame;
static Texture* quarterSize;
static Texture* renderBuffer1;

static constexpr auto dispatchGlowMaterial(int id) noexcept
{
    switch (id) {
    default:
    case 0: return normal;
    }
}

static void initializeGlowMaterials() noexcept
{
    {
        normal = interfaces->materialSystem->findMaterial("dev/glow_color", "Other textures");
        normal->incrementReferenceCount();
    }


    fullFrame = interfaces->materialSystem->findTexture("_rt_FullFrameFB", "RenderTargets");
    fullFrame->incrementReferenceCount();
    quarterSize = interfaces->materialSystem->findTexture("_rt_SmallFB1", "RenderTargets");
    quarterSize->incrementReferenceCount();

    renderBuffer1 = interfaces->materialSystem->createNamedRenderTargetTextureEx(
        "glow_buffer_1", fullFrame->getActualWidth(), fullFrame->getActualHeight(),
        8, 2, 0, 4 | 8 | 8192, 0x00000001);
    renderBuffer1->incrementReferenceCount();
}

static void drawModel(Entity* entity, int flags = 0x00000001, bool isDrawingModels = true) noexcept
{
    entity->drawModel(flags);
    if (isDrawingModels)
        drawnGlowEntities[entity->handle()] = true;
}

bool hasDrawn(int handle) noexcept
{
    return drawnGlowEntities.find(handle) != drawnGlowEntities.end();
}

void Glow::render() noexcept
{
    if (!localPlayer)
        return;

    auto& glow = config->glow;

    customGlowEntities.clear();
    drawnGlowEntities.clear();

    if (!config->glowKey.isActive())
        return;

    static bool materialsInitialized = false;
    if (!materialsInitialized) {
        initializeGlowMaterials();
        materialsInitialized = true;
    }

    MaterialRenderContext* renderContext = interfaces->materialSystem->getRenderContext();
    if (!renderContext)
        return;

    ShaderStencilState StencilStateDisable = {};
    StencilStateDisable.enabled = false;

    std::array<float, 3> originalColor;
    interfaces->renderView->getColorModulation(originalColor.data());
    const float originalBlend = interfaces->renderView->getBlend();

    ShaderStencilState StencilState = {};
    StencilState.enabled = true;
    StencilState.referenceValue = 1;
    StencilState.compareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
    StencilState.passOp = STENCILOPERATION_REPLACE;
    StencilState.failOp = STENCILOPERATION_KEEP;
    StencilState.ZfailOp = STENCILOPERATION_REPLACE;
    StencilState.setStencilState(renderContext);

    interfaces->renderView->setBlend(1.0f);
    interfaces->renderView->setColorModulation(1.0f, 1.0f, 1.0f);

    const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
    for (int i = 1; i <= highestEntityIndex; ++i) {
        auto applyGlow = [](const Config::GlowItem& glow, Entity* entity, int health = 0, int maxHealth = 0) noexcept
        {
            if (glow.enabled) {
                GlowEntity glowEntity;
                glowEntity.entity = entity;
                glowEntity.color[3] = glow.color[3];
                if (glow.healthBased && health) {
                    Helpers::healthColor(std::clamp(static_cast<float>(health) / static_cast<float>(maxHealth), 0.0f, 1.0f), glowEntity.color[0], glowEntity.color[1], glowEntity.color[2]);
                }
                else if (glow.rainbow) {
                    const auto [r, g, b] { rainbowColor(glow.rainbowSpeed) };
                    glowEntity.color[0] = r;
                    glowEntity.color[1] = g;
                    glowEntity.color[2] = b;
                }
                else {
                    glowEntity.color[0] = glow.color[0];
                    glowEntity.color[1] = glow.color[1];
                    glowEntity.color[2] = glow.color[2];
                }
                if (!hasDrawn(entity->handle()))
                    drawModel(entity);

                customGlowEntities.push_back(glowEntity);
            }
        };

        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity)
            continue;

        if (entity->isPlayer()) {
            if (entity == localPlayer.get())
                applyGlow(glow["Local Player"], entity, entity->health(), entity->getMaxHealth());
            else if (entity->isEnemy(localPlayer.get()))
                applyGlow(glow["Enemies"], entity, entity->health(), entity->getMaxHealth());
            else
                applyGlow(glow["Allies"], entity, entity->health(), entity->getMaxHealth());
        }
        else
        {
            switch (entity->getClassId())
            {
            case ClassId::ObjectSentrygun:
            case ClassId::ObjectDispenser:
            case ClassId::ObjectTeleporter:

                break;
            case ClassId::BaseAnimating:

                break;
            case ClassId::TFAmmoPack:

                break;
            case ClassId::HeadlessHatman:
            case ClassId::TFTankBoss:
            case ClassId::Merasmus:
            case ClassId::Zombie:
            case ClassId::EyeballBoss:

                break;
            default:
                break;
            }
        }
    }

    const int w =static_cast<int>(ImGui::GetIO().DisplaySize.x);
    const int h = static_cast<int>(ImGui::GetIO().DisplaySize.y);

    StencilState = {};
    StencilState.enabled = true;
    StencilState.writeMask = 0x0;
    StencilState.testMask = 0xFF;
    StencilState.referenceValue = 0;
    StencilState.compareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
    StencilState.passOp = STENCILOPERATION_KEEP;
    StencilState.failOp = STENCILOPERATION_KEEP;
    StencilState.ZfailOp = STENCILOPERATION_KEEP;
    StencilState.setStencilState(renderContext);

    interfaces->modelRender->forcedMaterialOverride(normal);

    renderContext->pushRenderTargetAndViewport();
    {
        renderContext->setRenderTarget(renderBuffer1);
        renderContext->viewport(0, 0, w, h);
        renderContext->clearColor4ub(0, 0, 0, 0);
        renderContext->clearBuffers(true, false, false);

        for (const auto& glowEntity : customGlowEntities)
        {
            interfaces->renderView->setBlend(glowEntity.color[3]);
            interfaces->renderView->setColorModulation(glowEntity.color[0], glowEntity.color[1], glowEntity.color[2]);
            drawModel(glowEntity.entity, 0x00000001 | 0x00000080, false);
        }
    }
    renderContext->popRenderTargetAndViewport();

    StencilStateDisable.setStencilState(renderContext);

    interfaces->modelRender->forcedMaterialOverride(nullptr);
    interfaces->renderView->setColorModulation(originalColor.data());
    interfaces->renderView->setBlend(originalBlend);
}

void Glow::updateInput() noexcept
{
    config->glowKey.handleToggle();
}