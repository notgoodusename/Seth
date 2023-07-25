#pragma once

#include "../SDK/ClientMode.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Vector.h"
#include "../SDK/ViewSetup.h"

namespace Misc
{
    void bunnyHop(UserCmd*) noexcept;
    void autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept;
    void antiAfkKick(UserCmd* cmd) noexcept;
    void edgejump(UserCmd* cmd) noexcept;
    void fastStop(UserCmd* cmd) noexcept;
    void viewModelChanger(Vector& eyePosition, Vector& eyeAngles) noexcept;
    void initHiddenCvars() noexcept;
    void unlockHiddenCvars() noexcept;
    void drawPlayerList() noexcept;
    void fixMovement(UserCmd* cmd, float yaw) noexcept;
    void drawOffscreenEnemies(ImDrawList* drawList) noexcept;

    void showKeybinds() noexcept;

    void spectatorList() noexcept;

    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
