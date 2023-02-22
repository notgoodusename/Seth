#pragma once

#include "../SDK/ClientMode.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Vector.h"

namespace Misc
{
    void bunnyHop(UserCmd*) noexcept;
    void autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept;
    void initHiddenCvars() noexcept;
    void unlockHiddenCvars() noexcept;
    void fixMovement(UserCmd* cmd, float yaw) noexcept;

    void showKeybinds() noexcept;

    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
