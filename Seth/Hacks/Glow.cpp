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

bool renderingGlow = false;

std::vector<GlowEntity> customGlowEntities;
std::unordered_map<int, bool> drawnGlowEntities;

Material* glowColor = nullptr;

Material* blurX = nullptr;
Material* blurY = nullptr;
Material* haloAddToScreen = nullptr;

Texture* fullFrame;
Texture* quarterSize;
Texture* renderBuffer1;
Texture* renderBuffer2;

static void initializeGlowMaterials() noexcept
{
    if(!glowColor)
    {
        glowColor = interfaces->materialSystem->findMaterial("dev/glow_color", "Other textures");
        glowColor->incrementReferenceCount();
    }

    if(!blurX)
    {
        static KeyValues* blurXKeyValue = nullptr;
        if (!blurXKeyValue)
        {
            blurXKeyValue = new KeyValues("BlurFilterX");
            blurXKeyValue->setString("$basetexture", "glow_buffer_1");
            blurXKeyValue->setString("$wireframe", "0");
        }
        blurX = interfaces->materialSystem->createMaterial("BlurX", blurXKeyValue);
    }

    if (!blurY)
    {
        static KeyValues* blurYKeyValue = nullptr;
        if (!blurYKeyValue)
        {
            blurYKeyValue = new KeyValues("BlurFilterY");
            blurYKeyValue->setString("$basetexture", "glow_buffer_2");
            blurYKeyValue->setString("$wireframe", "0");
        }
        blurY = interfaces->materialSystem->createMaterial("BlurY", blurYKeyValue);
    }

    if (!haloAddToScreen)
    {
        static KeyValues* haloAddToScreenKeyValue = nullptr;;
        if (!haloAddToScreenKeyValue)
        {
            haloAddToScreenKeyValue = new KeyValues("UnlitGeneric");
            haloAddToScreenKeyValue->setString("$basetexture", "glow_buffer_1");
            haloAddToScreenKeyValue->setString("$wireframe", "0");
            haloAddToScreenKeyValue->setString("$additive", "1");
        }
        haloAddToScreen = interfaces->materialSystem->createMaterial("HaloAddToScreen", haloAddToScreenKeyValue);
    }

    fullFrame = interfaces->materialSystem->findTexture("_rt_FullFrameFB", "RenderTargets");
    fullFrame->incrementReferenceCount();
    quarterSize = interfaces->materialSystem->findTexture("_rt_SmallFB1", "RenderTargets");
    quarterSize->incrementReferenceCount();

    renderBuffer1 = interfaces->materialSystem->createNamedRenderTargetTextureEx(
        "glow_buffer_1", fullFrame->getActualWidth(), fullFrame->getActualHeight(),
        8, 2, 0, 4 | 8 | 8192, 0x00000001);
    renderBuffer1->incrementReferenceCount();

    renderBuffer2 = interfaces->materialSystem->createNamedRenderTargetTextureEx(
        "glow_buffer_2", fullFrame->getActualWidth(), fullFrame->getActualHeight(),
        8, 2, 0, 4 | 8 | 8192, 0x00000001);
    renderBuffer2->incrementReferenceCount();
}

static void drawModel(Entity* entity, bool isDrawingModels = true) noexcept
{
    if (!isDrawingModels)
        renderingGlow = true;

    entity->drawModel(0x00000001 | 0x00000080);

    if (isDrawingModels)
        drawnGlowEntities[entity->handle()] = true;

    if (!isDrawingModels)
        renderingGlow = false;
}

bool Glow::hasDrawn(int handle) noexcept
{
    return drawnGlowEntities.find(handle) != drawnGlowEntities.end();
}

bool Glow::isRenderingGlow() noexcept
{
    return renderingGlow;
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

    std::array<float, 3> originalColor;
    interfaces->renderView->getColorModulation(originalColor.data());
    const float originalBlend = interfaces->renderView->getBlend();

    ShaderStencilState stencilState = { };
    stencilState.enabled = true;
    stencilState.referenceValue = 1;
    stencilState.compareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
    stencilState.passOp = STENCILOPERATION_REPLACE;
    stencilState.failOp = STENCILOPERATION_KEEP;
    stencilState.ZfailOp = STENCILOPERATION_REPLACE;
    stencilState.referenceValue = 1;
    stencilState.writeMask = 0xFF;
    stencilState.testMask = 0x0;
    stencilState.setStencilState(renderContext);

    interfaces->renderView->setBlend(1.0f);
    interfaces->renderView->setColorModulation(1.0f, 1.0f, 1.0f);

    const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
    for (int i = 1; i <= highestEntityIndex; ++i) {
        auto applyGlow = [](const Config::GlowItem& glow, Entity* entity, int health = 0, int maxHealth = 0) noexcept
        {
            if (!glow.enabled)
                return;

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

            if (!entity->isPlayer())
                return;

            auto attachment = entity->moveChild();
            for (int i = 0; i < 32; i++)
            {
                if (!attachment || attachment->isDormant())
                    continue;

                if (attachment->getClassId() == ClassId::TFWearable)
                {
                    GlowEntity glowAttachement;
                    glowAttachement.entity = attachment;
                    glowAttachement.color[3] = glow.color[3];

                    if (glow.healthBased && health) {
                        Helpers::healthColor(std::clamp(static_cast<float>(health) / static_cast<float>(maxHealth), 0.0f, 1.0f), glowAttachement.color[0], glowAttachement.color[1], glowAttachement.color[2]);
                    }
                    else if (glow.rainbow) {
                        const auto [r, g, b] { rainbowColor(glow.rainbowSpeed) };
                        glowAttachement.color[0] = r;
                        glowAttachement.color[1] = g;
                        glowAttachement.color[2] = b;
                    }
                    else {
                        glowAttachement.color[0] = glow.color[0];
                        glowAttachement.color[1] = glow.color[1];
                        glowAttachement.color[2] = glow.color[2];
                    }

                    customGlowEntities.push_back(glowAttachement);

                    if (!hasDrawn(attachment->handle()))
                        drawModel(attachment);
                }

                attachment = attachment->nextMovePeer();
            }

            const auto& activeWeapon = entity->getActiveWeapon();
            if (!activeWeapon || activeWeapon->isDormant())
                return;

            GlowEntity glowWeapon;
            glowWeapon.entity = activeWeapon;
            glowWeapon.color[3] = glow.color[3];

            if (glow.healthBased && health) {
                Helpers::healthColor(std::clamp(static_cast<float>(health) / static_cast<float>(maxHealth), 0.0f, 1.0f), glowWeapon.color[0], glowWeapon.color[1], glowWeapon.color[2]);
            }
            else if (glow.rainbow) {
                const auto [r, g, b] { rainbowColor(glow.rainbowSpeed) };
                glowWeapon.color[0] = r;
                glowWeapon.color[1] = g;
                glowWeapon.color[2] = b;
            }
            else {
                glowWeapon.color[0] = glow.color[0];
                glowWeapon.color[1] = glow.color[1];
                glowWeapon.color[2] = glow.color[2];
            }

            customGlowEntities.push_back(glowWeapon);

            if (!hasDrawn(activeWeapon->handle()))
                drawModel(activeWeapon);
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

    const int w = static_cast<int>(ImGui::GetIO().DisplaySize.x);
    const int h = static_cast<int>(ImGui::GetIO().DisplaySize.y);

    renderContext->setStencilEnable(false);

    if (customGlowEntities.empty())
        return;

    interfaces->modelRender->forcedMaterialOverride(glowColor);

    renderContext->pushRenderTargetAndViewport();
    {
        renderContext->setRenderTarget(renderBuffer1);
        renderContext->viewport(0, 0, w, h);
        renderContext->clearColor4ub(0, 0, 0, 0);
        renderContext->clearBuffers(true, false, false);

        for (const auto& glowEntity : customGlowEntities)
        {
            if (!glowEntity.entity)
                continue;

            interfaces->renderView->setBlend(glowEntity.color[3]);
            interfaces->renderView->setColorModulation(glowEntity.color[0], glowEntity.color[1], glowEntity.color[2]);
            
            drawModel(glowEntity.entity, false);
        }
    }
    renderContext->popRenderTargetAndViewport();

    renderContext->pushRenderTargetAndViewport(); {
        renderContext->viewport(0, 0, w, h);

        renderContext->setRenderTarget(renderBuffer2);
        renderContext->drawScreenSpaceRectangle(blurX, 0, 0, w, h, 0.0f, 0.0f, static_cast<float>(w - 1), static_cast<float>(h - 1), w, h);

        renderContext->setRenderTarget(renderBuffer1);
        renderContext->drawScreenSpaceRectangle(blurY, 0, 0, w, h, 0.0f, 0.0f, static_cast<float>(w - 1), static_cast<float>(h - 1), w, h);
    }
    renderContext->popRenderTargetAndViewport();

    stencilState = { };
    stencilState.enabled = true;
    stencilState.writeMask = 0x0;
    stencilState.testMask = 0xFF;
    stencilState.referenceValue = 0;
    stencilState.compareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
    stencilState.passOp = STENCILOPERATION_KEEP;
    stencilState.failOp = STENCILOPERATION_KEEP;
    stencilState.ZfailOp = STENCILOPERATION_KEEP;
    stencilState.referenceValue =01;
    stencilState.writeMask = 0x0;
    stencilState.testMask = 0xFF;
    stencilState.setStencilState(renderContext);

    renderContext->drawScreenSpaceRectangle(haloAddToScreen, 0, 0, w, h, 0.0f, 0.0f, static_cast<float>(w - 1), static_cast<float>(h - 1), w, h);

    renderContext->setStencilEnable(false);
    
    interfaces->renderView->setColorModulation(1.0f, 1.0f, 1.0f);
    interfaces->renderView->setBlend(1.0f);
    interfaces->modelRender->forcedMaterialOverride(nullptr);
}

void Glow::updateInput() noexcept
{
    config->glowKey.handleToggle();
}