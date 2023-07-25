#include "Animations.h"

#include "../SDK/LocalPlayer.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"

Vector viewAngles{};

void Animations::init() noexcept
{

}

void Animations::updateLocalAngles(UserCmd* cmd) noexcept
{
    viewAngles = cmd->viewangles;
}

void Animations::reset() noexcept
{
    viewAngles = Vector{};
}

Vector Animations::getLocalViewangles() noexcept
{
    return viewAngles;
}