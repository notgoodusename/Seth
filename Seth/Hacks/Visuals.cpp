#include <array>
#include <cstring>
#include <string.h>
#include <deque>
#include <sys/stat.h>

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../fnv.h"
#include "../GameData.h"
#include "../Helpers.h"

#include "Animations.h"
#include "Backtrack.h"
#include "Visuals.h"

#include "../SDK/ConVar.h"
#include "../SDK/DebugOverlay.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/Material.h"
#include "../SDK/MaterialSystem.h"
#include "../SDK/ModelInfo.h"

#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"

#include "../SDK/ViewSetup.h"
/*
void Visuals::drawAimbotFov(ImDrawList* drawList) noexcept
{
    if (!config->legitbotFov.enabled || !config->legitbotKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    GameData::Lock lock;
    const auto& local = GameData::local();

    if (!local.exists || !local.alive || local.aimPunch.null())
        return;

    if (memory->input->isCameraInThirdPerson())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    auto weaponIndex = getWeaponIndex(activeWeapon->itemDefinitionIndex2());
    if (!weaponIndex)
        return;

    const auto& cfg = config->legitbot;

    auto weaponClass = getWeaponClass(activeWeapon->itemDefinitionIndex2());
    if (!cfg[weaponIndex].enabled)
        weaponIndex = weaponClass;

    if (!cfg[weaponIndex].enabled)
        weaponIndex = 0;

    if (ImVec2 pos; Helpers::worldToScreen(local.aimPunch, pos))
    {
        const auto& displaySize = ImGui::GetIO().DisplaySize;
        const auto radius = std::tan(Helpers::deg2rad(cfg[weaponIndex].fov) / (16.0f/6.0f)) / std::tan(Helpers::deg2rad(localPlayer->isScoped() ? localPlayer->fov() : (config->visuals.fov + 90.0f)) / 2.0f) * displaySize.x;
        if (radius > displaySize.x || radius > displaySize.y || !std::isfinite(radius))
            return;

        const auto color = Helpers::calculateColor(config->legitbotFov);
        drawList->AddCircleFilled(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color);
        if (config->legitbotFov.outline)
            drawList->AddCircle(localPlayer->shotsFired() > 1 ? pos : displaySize / 2.0f, radius, color | IM_COL32_A_MASK, 360);
    }
}
*/

void Visuals::thirdperson() noexcept
{
    if (!interfaces->engine->isInGame())
        return;

    if (!config->visuals.thirdperson && !config->visuals.freeCam && !memory->input->isCameraInThirdPerson())
        return;

    const bool freeCamming = config->visuals.freeCam && config->visuals.freeCamKey.isActive() && localPlayer && localPlayer->isAlive();
    const bool thirdPerson = config->visuals.thirdperson && config->visuals.thirdpersonKey.isActive() && localPlayer && localPlayer->isAlive();

    if (freeCamming || thirdPerson)
        localPlayer->forceTauntCam() = 1;
    else
        localPlayer->forceTauntCam() = 0;
}

void Visuals::disablePostProcessing(FrameStage stage) noexcept
{
    if (stage != FrameStage::RENDER_START && stage != FrameStage::RENDER_END)
        return;

    //*memory->disablePostProcessing = stage == FrameStage::RENDER_START && config->visuals.disablePostProcessing;
}

void Visuals::updateInput() noexcept
{
    config->visuals.freeCamKey.handleToggle();
    config->visuals.thirdpersonKey.handleToggle();
}

void Visuals::reset(int resetType) noexcept
{
    if (!localPlayer)
        return;

    if (resetType == 1)
    {
        //Disable thirdperson/freecam
        const bool freeCamming = config->visuals.freeCam && config->visuals.freeCamKey.isActive() && localPlayer && localPlayer->isAlive();
        const bool thirdPerson = config->visuals.thirdperson && config->visuals.thirdpersonKey.isActive() && localPlayer && localPlayer->isAlive();
        if (freeCamming || thirdPerson)
            localPlayer->forceTauntCam() = 0;
    }
}
