#include <mutex>
#include <numeric>
#include <sstream>
#include <string>

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"
#include "../GUI.h"
#include "../Helpers.h"

#include "Misc.h"
#include "EnginePrediction.h"

#include "../SDK/Client.h"
#include "../SDK/ClientMode.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Prediction.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"

#include "../imguiCustom.h"

const bool anyActiveKeybinds() noexcept
{
    const bool antiAimManualForward = config->antiAim.enabled && config->antiAim.manualForward.canShowKeybind();
    const bool antiAimManualBackward = config->antiAim.enabled && config->antiAim.manualBackward.canShowKeybind();
    const bool antiAimManualRight = config->antiAim.enabled && config->antiAim.manualRight.canShowKeybind();
    const bool antiAimManualLeft = config->antiAim.enabled && config->antiAim.manualLeft.canShowKeybind();
    const bool doubletap = config->tickbase.doubletap.canShowKeybind();
    const bool hideshots = config->tickbase.hideshots.canShowKeybind();
    const bool aimbot = config->aimbotKey.canShowKeybind();
    const bool triggerBot = config->triggerbot.enabled && config->triggerbotKey.canShowKeybind();
    const bool glow = config->glowKey.canShowKeybind();
    const bool chams = config->chamsKey.canShowKeybind();
    const bool esp = config->streamProofESP.key.canShowKeybind();

    const bool thirdperson = config->visuals.thirdperson && config->visuals.thirdpersonKey.canShowKeybind();
    const bool freeCam = config->visuals.freeCam && config->visuals.freeCamKey.canShowKeybind();

    const bool edgejump = config->misc.edgeJump && config->misc.edgeJumpKey.canShowKeybind();

    return aimbot || antiAimManualForward || antiAimManualBackward || antiAimManualRight  || antiAimManualLeft 
        || doubletap || hideshots
        || triggerBot || chams || glow || esp
        || thirdperson || freeCam || edgejump;
}

void Misc::showKeybinds() noexcept
{
    if (!config->misc.keybindList.enabled)
        return;

    if (!anyActiveKeybinds() && !gui->isOpen())
        return;

    if (config->misc.keybindList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.keybindList.pos);
        config->misc.keybindList.pos = {};
    }

    ImGui::SetNextWindowSize({ 250.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints({ 250.f, 0.f }, { 250.f, FLT_MAX });

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;

    if (config->misc.keybindList.noTitleBar)
        windowFlags |= ImGuiWindowFlags_NoTitleBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Keybind list", nullptr, windowFlags);
    ImGui::PopStyleVar();

    if (config->antiAim.enabled)
    {
        config->antiAim.manualForward.showKeybind();
        config->antiAim.manualBackward.showKeybind();
        config->antiAim.manualRight.showKeybind();
        config->antiAim.manualLeft.showKeybind();
    }

    config->tickbase.doubletap.showKeybind();
    config->tickbase.hideshots.showKeybind();

    config->aimbotKey.showKeybind();
    if (config->triggerbot.enabled)
        config->triggerbotKey.showKeybind();
    config->chamsKey.showKeybind();
    config->glowKey.showKeybind();
    config->streamProofESP.key.showKeybind();

    if (config->visuals.thirdperson)
        config->visuals.thirdpersonKey.showKeybind();
    if (config->visuals.freeCam)
        config->visuals.freeCamKey.showKeybind();

    if (config->misc.edgeJump)
        config->misc.edgeJumpKey.showKeybind();

    ImGui::End();
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer->isOnGround() };

    if (config->misc.bunnyHop && !localPlayer->isOnGround() 
        && localPlayer->moveType() != MoveType::NOCLIP
        && !localPlayer->isInBumperKart() && !localPlayer->isAGhost() && !localPlayer->isSwimming() && localPlayer->isAlive()
        && !wasLastTimeOnGround)
        cmd->buttons &= ~UserCmd::IN_JUMP;

    wasLastTimeOnGround = localPlayer->isOnGround();
}

void Misc::autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept
{
    if (!config->misc.autoStrafe)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const float speed = localPlayer->velocity().length2D();
    if (speed < 5.0f)
        return;

    static float angle = 0.f;

    const bool back = cmd->buttons & UserCmd::IN_BACK;
    const bool forward = cmd->buttons & UserCmd::IN_FORWARD;
    const bool right = cmd->buttons & UserCmd::IN_MOVERIGHT;
    const bool left = cmd->buttons & UserCmd::IN_MOVELEFT;

    if (back) {
        angle = -180.f;
        if (left)
            angle -= 45.f;
        else if (right)
            angle += 45.f;
    }
    else if (left) {
        angle = 90.f;
        if (back)
            angle += 45.f;
        else if (forward)
            angle -= 45.f;
    }
    else if (right) {
        angle = -90.f;
        if (back)
            angle -= 45.f;
        else if (forward)
            angle += 45.f;
    }
    else {
        angle = 0.f;
    }

    //If we are on ground, noclip or in a ladder return
    if (localPlayer->isOnGround() || localPlayer->moveType() == MoveType::NOCLIP)
        return;

    if (localPlayer->isInBumperKart() || localPlayer->isAGhost() || localPlayer->isSwimming())
        return;

    currentViewAngles.y += angle;

    cmd->forwardmove = 0.f;
    cmd->sidemove = 0.f;

    const auto delta = Helpers::normalizeYaw(currentViewAngles.y - Helpers::rad2deg(std::atan2(localPlayer->velocity().y, localPlayer->velocity().x)));

    cmd->sidemove = delta > 0.f ? -450.f : 450.f;

    currentViewAngles.y = Helpers::normalizeYaw(currentViewAngles.y - delta);
}

void Misc::antiAfkKick(UserCmd* cmd) noexcept
{
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= 1 << 27;
}

void Misc::edgejump(UserCmd* cmd) noexcept
{
    if (!config->misc.edgeJump || !config->misc.edgeJumpKey.isActive())
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isOnGround() || localPlayer->isSwimming())
        return;

    if (const auto mt = localPlayer->moveType(); mt == MoveType::NOCLIP)
        return;

    if (EnginePrediction::wasOnGround())
        cmd->buttons |= UserCmd::IN_JUMP;
}

void Misc::fastStop(UserCmd* cmd) noexcept
{
    if (!config->misc.fastStop)
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() ||
        localPlayer->isInBumperKart() || localPlayer->isAGhost() || localPlayer->isSwimming() || localPlayer->isCharging())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || !localPlayer->isOnGround() || cmd->buttons & UserCmd::IN_JUMP)
        return;

    if (cmd->buttons & (UserCmd::IN_MOVELEFT | UserCmd::IN_MOVERIGHT | UserCmd::IN_FORWARD | UserCmd::IN_BACK))
        return;

    const auto velocity = localPlayer->velocity();
    const auto speed = velocity.length2D();
    if (speed < 15.0f)
        return;

    Vector direction = velocity.toAngle();
    direction.y = cmd->viewangles.y - direction.y;

    const auto negatedDirection = Vector::fromAngle(direction) * -speed;
    cmd->forwardmove = negatedDirection.x;
    cmd->sidemove = negatedDirection.y;
}

void Misc::viewModelChanger(Vector& eyePosition, Vector& eyeAngles) noexcept
{
    if (!config->visuals.viewModel.enabled)
        return;

    if (!localPlayer)
        return;

    auto setViewmodel = [](Vector& origin, Vector& angles) noexcept
    {
        Vector forward = Vector::fromAngle(angles);
        Vector up = Vector::fromAngle(angles - Vector{ 90.0f, 0.0f, 0.0f });
        Vector side = forward.cross(up);
        Vector offset = side * config->visuals.viewModel.x + forward * config->visuals.viewModel.y + up * config->visuals.viewModel.z;
        origin += offset;
        angles += Vector{ 0.0f, 0.0f, config->visuals.viewModel.roll };
    };

    setViewmodel(eyePosition, eyeAngles);
}

std::vector<ConCommandBase*> dev;
std::vector<ConCommandBase*> hidden;

void Misc::initHiddenCvars() noexcept {
    ConCommandBase* cmdBase = interfaces->cvar->getCommands();

    while(cmdBase != nullptr)
    {
        if (cmdBase->flags & CvarFlags::DEVELOPMENTONLY)
            dev.push_back(cmdBase);

        if (cmdBase->flags & CvarFlags::HIDDEN)
            hidden.push_back(cmdBase);

        cmdBase = cmdBase->next;
    }
}

void Misc::unlockHiddenCvars() noexcept {

    static bool toggle = true;

    if (config->misc.unhideConvars == toggle)
        return;

    if (config->misc.unhideConvars) {
        for (unsigned x = 0; x < dev.size(); x++)
            dev.at(x)->flags &= ~CvarFlags::DEVELOPMENTONLY;

        for (unsigned x = 0; x < hidden.size(); x++)
            hidden.at(x)->flags &= ~CvarFlags::HIDDEN;

    }
    if (!config->misc.unhideConvars) {
        for (unsigned x = 0; x < dev.size(); x++)
            dev.at(x)->flags |= CvarFlags::DEVELOPMENTONLY;

        for (unsigned x = 0; x < hidden.size(); x++)
            hidden.at(x)->flags |= CvarFlags::HIDDEN;
    }

    toggle = config->misc.unhideConvars;
}

void Misc::fixMovement(UserCmd* cmd, float yaw) noexcept
{
    float oldYaw = yaw + (yaw < 0.0f ? 360.0f : 0.0f);
    float newYaw = cmd->viewangles.y + (cmd->viewangles.y < 0.0f ? 360.0f : 0.0f);
    float yawDelta = newYaw < oldYaw ? fabsf(newYaw - oldYaw) : 360.0f - fabsf(newYaw - oldYaw);
    yawDelta = 360.0f - yawDelta;

    const float forwardmove = cmd->forwardmove;
    const float sidemove = cmd->sidemove;
    cmd->forwardmove = std::cos(Helpers::deg2rad(yawDelta)) * forwardmove + std::cos(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
    cmd->sidemove = std::sin(Helpers::deg2rad(yawDelta)) * forwardmove + std::sin(Helpers::deg2rad(yawDelta + 90.0f)) * sidemove;
}

void Misc::updateInput() noexcept
{
    config->misc.edgeJumpKey.handleToggle();
}

void Misc::reset(int resetType) noexcept
{

}
