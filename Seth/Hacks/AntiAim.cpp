#include "../Interfaces.h"

#include "AimbotFunctions.h"
#include "AntiAim.h"
#include "Tickbase.h"

#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/UserCmd.h"

bool updateLby(bool update = false) noexcept
{
    static float timer = 0.f;
    static bool lastValue = false;

    if (!update)
        return lastValue;

    if (!(localPlayer->flags() & 1) || !localPlayer->getAnimstate())
    {
        lastValue = false;
        return false;
    }

    if (localPlayer->velocity().length2D() > 0.1f || fabsf(localPlayer->velocity().z) > 100.f)
        timer = memory->globalVars->serverTime() + 0.22f;

    if (timer < memory->globalVars->serverTime())
    {
        timer = memory->globalVars->serverTime() + 1.1f;
        lastValue = true;
        return true;
    }
    lastValue = false;
    return false;
}

void AntiAim::updateInput() noexcept
{

}