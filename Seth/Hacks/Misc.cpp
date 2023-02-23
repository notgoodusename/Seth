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

#include "../SDK/Client.h"
#include "../SDK/ClientMode.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Prediction.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"

#include "../imguiCustom.h"

const bool anyActiveKeybinds() noexcept
{
    const bool rageBot = config->ragebotKey.canShowKeybind();
    const bool minDamageOverride = config->minDamageOverrideKey.canShowKeybind();
    const bool fakeAngle = config->fakeAngle.enabled && config->fakeAngle.invert.canShowKeybind();
    const bool antiAimManualForward = config->rageAntiAim.enabled && config->rageAntiAim.manualForward.canShowKeybind();
    const bool antiAimManualBackward = config->rageAntiAim.enabled && config->rageAntiAim.manualBackward.canShowKeybind();
    const bool antiAimManualRight = config->rageAntiAim.enabled && config->rageAntiAim.manualRight.canShowKeybind();
    const bool antiAimManualLeft = config->rageAntiAim.enabled && config->rageAntiAim.manualLeft.canShowKeybind();
    const bool legitAntiAim = config->legitAntiAim.enabled && config->legitAntiAim.invert.canShowKeybind();
    const bool doubletap = config->tickbase.doubletap.canShowKeybind();
    const bool hideshots = config->tickbase.hideshots.canShowKeybind();
    const bool legitBot = config->legitbotKey.canShowKeybind();
    const bool triggerBot = config->triggerbotKey.canShowKeybind();
    const bool glow = config->glowKey.canShowKeybind();
    const bool chams = config->chamsKey.canShowKeybind();
    const bool esp = config->streamProofESP.key.canShowKeybind();

    const bool zoom = config->visuals.zoom && config->visuals.zoomKey.canShowKeybind();
    const bool thirdperson = config->visuals.thirdperson && config->visuals.thirdpersonKey.canShowKeybind();
    const bool freeCam = config->visuals.freeCam && config->visuals.freeCamKey.canShowKeybind();

    const bool blockbot = config->misc.blockBot && config->misc.blockBotKey.canShowKeybind();
    const bool edgejump = config->misc.edgeJump && config->misc.edgeJumpKey.canShowKeybind();
    const bool minijump = config->misc.miniJump && config->misc.miniJumpKey.canShowKeybind();
    const bool jumpBug = config->misc.jumpBug && config->misc.jumpBugKey.canShowKeybind();
    const bool edgebug = config->misc.edgeBug && config->misc.edgeBugKey.canShowKeybind();
    const bool autoPixelSurf = config->misc.autoPixelSurf && config->misc.autoPixelSurfKey.canShowKeybind();
    const bool slowwalk = config->misc.slowwalk && config->misc.slowwalkKey.canShowKeybind();
    const bool fakeduck = config->misc.fakeduck && config->misc.fakeduckKey.canShowKeybind();
    const bool autoPeek = config->misc.autoPeek.enabled && config->misc.autoPeekKey.canShowKeybind();
    const bool prepareRevolver = config->misc.prepareRevolver && config->misc.prepareRevolverKey.canShowKeybind();

    return rageBot || minDamageOverride || fakeAngle || antiAimManualForward || antiAimManualBackward || antiAimManualRight  || antiAimManualLeft 
        || doubletap || hideshots
        || legitAntiAim || legitBot || triggerBot || chams || glow || esp
        || zoom || thirdperson || freeCam || blockbot || edgejump || minijump || jumpBug || edgebug || autoPixelSurf || slowwalk || fakeduck || autoPeek || prepareRevolver;
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

    config->ragebotKey.showKeybind();
    config->minDamageOverrideKey.showKeybind();
    if (config->fakeAngle.enabled)
        config->fakeAngle.invert.showKeybind();
    if (config->rageAntiAim.enabled)
    {
        config->rageAntiAim.manualForward.showKeybind();
        config->rageAntiAim.manualBackward.showKeybind();
        config->rageAntiAim.manualRight.showKeybind();
        config->rageAntiAim.manualLeft.showKeybind();
    }

    config->tickbase.doubletap.showKeybind();
    config->tickbase.hideshots.showKeybind();

    if (config->legitAntiAim.enabled)
        config->legitAntiAim.invert.showKeybind();

    config->legitbotKey.showKeybind();
    config->triggerbotKey.showKeybind();
    config->chamsKey.showKeybind();
    config->glowKey.showKeybind();
    config->streamProofESP.key.showKeybind();

    if (config->visuals.zoom)
        config->visuals.zoomKey.showKeybind();
    if (config->visuals.thirdperson)
        config->visuals.thirdpersonKey.showKeybind();
    if (config->visuals.freeCam)
        config->visuals.freeCamKey.showKeybind();

    if (config->misc.blockBot)
        config->misc.blockBotKey.showKeybind();
    if (config->misc.edgeJump)
        config->misc.edgeJumpKey.showKeybind();
    if (config->misc.miniJump)
        config->misc.miniJumpKey.showKeybind();
    if (config->misc.jumpBug)
        config->misc.jumpBugKey.showKeybind();
    if (config->misc.edgeBug)
        config->misc.edgeBugKey.showKeybind();
    if (config->misc.autoPixelSurf)
        config->misc.autoPixelSurfKey.showKeybind();
    if (config->misc.jumpBug)
        config->misc.jumpBugKey.showKeybind();
    if (config->misc.slowwalk)
        config->misc.slowwalkKey.showKeybind();
    if (config->misc.fakeduck)
        config->misc.fakeduckKey.showKeybind();
    if (config->misc.autoPeek.enabled)
        config->misc.autoPeekKey.showKeybind();
    if (config->misc.prepareRevolver)
        config->misc.prepareRevolverKey.showKeybind();

    ImGui::End();
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!localPlayer)
        return;

    static auto wasLastTimeOnGround{ localPlayer->isOnGround() };

    if (config->misc.bunnyHop && !localPlayer->isOnGround() 
        && localPlayer->moveType() != MoveType::LADDER && localPlayer->moveType() != MoveType::NOCLIP && localPlayer->moveType() != MoveType::OBSERVER
        && !localPlayer->isInBumperKart() && !localPlayer->isAGhost() && !localPlayer->isSwimming()
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
    if (localPlayer->isOnGround() || localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::LADDER)
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
    config->misc.blockBotKey.handleToggle();
    config->misc.edgeJumpKey.handleToggle();
    config->misc.miniJumpKey.handleToggle();
    config->misc.jumpBugKey.handleToggle();
    config->misc.edgeBugKey.handleToggle();
    config->misc.autoPixelSurfKey.handleToggle();
    config->misc.slowwalkKey.handleToggle();
    config->misc.fakeduckKey.handleToggle();
    config->misc.autoPeekKey.handleToggle();
    config->misc.prepareRevolverKey.handleToggle();
}

void Misc::reset(int resetType) noexcept
{

}
