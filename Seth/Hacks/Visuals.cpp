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
