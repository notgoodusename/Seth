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
#include "TargetSystem.h"

#include "../SDK/Client.h"
#include "../SDK/ClientMode.h"
#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameEvent.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Input.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Prediction.h"
#include "../SDK/Surface.h"
#include "../SDK/UserCmd.h"

#include "../GameData.h"
#include "../imguiCustom.h"

const bool anyActiveKeybinds() noexcept
{
    const bool antiAimManualForward = config->antiAim.enabled && config->antiAim.manualForward.canShowKeybind();
    const bool antiAimManualBackward = config->antiAim.enabled && config->antiAim.manualBackward.canShowKeybind();
    const bool antiAimManualRight = config->antiAim.enabled && config->antiAim.manualRight.canShowKeybind();
    const bool antiAimManualLeft = config->antiAim.enabled && config->antiAim.manualLeft.canShowKeybind();
    const bool recharge = config->misc.tickBase.enabled && config->misc.tickBase.rechargeKey.canShowKeybind();
    const bool doubletap = config->misc.tickBase.enabled && config->misc.tickBase.doubleTapKey.canShowKeybind();
    const bool warp = config->misc.tickBase.enabled && config->misc.tickBase.warpKey.canShowKeybind();
    const bool aimbot = config->aimbotKey.canShowKeybind();
    const bool triggerbot = (config->hitscanTriggerbot.enabled || config->meleeTriggerbot.enabled) && config->triggerbotKey.canShowKeybind();
    const bool auto_ = config->autoKey.canShowKeybind();
    const bool glow = config->glowKey.canShowKeybind();
    const bool chams = config->chamsKey.canShowKeybind();
    const bool esp = config->espKey.canShowKeybind();

    const bool thirdperson = config->visuals.thirdperson && config->visuals.thirdpersonKey.canShowKeybind();
    const bool freecam = config->visuals.freeCam && config->visuals.freeCamKey.canShowKeybind();

    const bool crithack = config->misc.critHack.enabled && config->misc.forceCritKey.canShowKeybind();
    const bool edgejump = config->misc.edgeJump && config->misc.edgeJumpKey.canShowKeybind();

    return antiAimManualForward || antiAimManualBackward || antiAimManualRight  || antiAimManualLeft 
        || recharge || doubletap || warp
        || aimbot || triggerbot || auto_
        || chams || glow || esp
        || thirdperson || freecam || crithack || edgejump;
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

    if (config->misc.tickBase.enabled)
    {
        config->misc.tickBase.rechargeKey.showKeybind();
        config->misc.tickBase.doubleTapKey.showKeybind();
        config->misc.tickBase.warpKey.showKeybind();
    }

    config->aimbotKey.showKeybind();
    if (config->hitscanTriggerbot.enabled || config->meleeTriggerbot.enabled)
        config->triggerbotKey.showKeybind();
    config->autoKey.showKeybind();
    config->chamsKey.showKeybind();
    config->glowKey.showKeybind();
    config->espKey.showKeybind();

    if (config->visuals.thirdperson)
        config->visuals.thirdpersonKey.showKeybind();
    if (config->visuals.freeCam)
        config->visuals.freeCamKey.showKeybind();

    if (config->misc.critHack.enabled)
        config->misc.forceCritKey.showKeybind();

    if (config->misc.edgeJump)
        config->misc.edgeJumpKey.showKeybind();

    ImGui::End();
}

void Misc::drawAimbotFov(ImDrawList* drawList) noexcept
{
    if (!config->aimbotFov.enabled)
        return;

    if (!interfaces->engine->isInGame() || !localPlayer)
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    float aimFov = 0.0f;

    const auto weaponType = activeWeapon->getWeaponType();
    switch (weaponType)
    {
    case WeaponType::HITSCAN:
        aimFov = config->aimbot.hitscan.fov;
        break;
    case WeaponType::PROJECTILE:
        aimFov = config->aimbot.projectile.fov;
        break;
    case WeaponType::MELEE:
        aimFov = config->aimbot.melee.fov;
        break;
    default:
        return;
    }

    const auto& displaySize = ImGui::GetIO().DisplaySize;
   
    const auto radius = std::tan(Helpers::deg2rad(aimFov) / (16.0f / 6.0f)) / std::tan(Helpers::deg2rad(localPlayer->isScoped() ? localPlayer->fov() : (config->visuals.fov + 90.0f)) / 2.0f) * displaySize.x;

    drawList->AddCircle(displaySize / 2, radius, Helpers::calculateColor(config->aimbotFov));
}

void Misc::spectatorList() noexcept
{
    if (!config->misc.spectatorList.enabled)
        return;

    if (config->misc.spectatorList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.spectatorList.pos);
        config->misc.spectatorList.pos = {};
    }

    ImGui::SetNextWindowSize({ 250.f, 0.f }, ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints({ 250.f, 0.f }, { 250.f, 1000.f });
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->isOpen())
        windowFlags |= ImGuiWindowFlags_NoInputs;
    if (config->misc.spectatorList.noTitleBar)
        windowFlags |= ImGuiWindowFlags_NoTitleBar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
    ImGui::Begin("Spectator list", nullptr, windowFlags);
    ImGui::PopStyleVar();

    GameData::Lock lock;
    if ((GameData::players().empty()))
    {
        ImGui::End();
        return;
    }

    const auto& local = GameData::local();

    if (!local.exists)
    {
        ImGui::End();
        return;
    }

    if (local.alive)
    {
        for (const auto& player : GameData::players())
        {
            if (player.dormant || player.alive || player.observerTargetHandle == 0 || player.observerTargetHandle != local.handle)
                continue;

            auto obsMode{ "" };

            switch (player.observerMode) 
            {
                case ObsMode::InEye:
                    obsMode = "1st";
                    break;
                case ObsMode::Chase:
                    obsMode = "3rd";
                    break;
                case ObsMode::Roaming:
                    obsMode = "Freecam";
                    break;
                default:
                    continue;
            }

            ImGui::TextWrapped("%s - %s", player.name.c_str(), obsMode);
        }
    }
    else if (local.observerTargetHandle)
    {
        for (const auto& player : GameData::players())
        {
            if (player.dormant || player.alive || player.observerTargetHandle == 0 || player.observerTargetHandle != local.observerTargetHandle)
                continue;

            auto obsMode{ "" };

            switch (player.observerMode)
            {
            case ObsMode::InEye:
                obsMode = "1st";
                break;
            case ObsMode::Chase:
                obsMode = "3rd";
                break;
            case ObsMode::Roaming:
                obsMode = "Freecam";
                break;
            default:
                continue;
            }

            ImGui::TextWrapped("%s - %s", player.name.c_str(), obsMode);
        }
    }

    ImGui::End();
}

void Misc::bunnyHop(UserCmd* cmd) noexcept
{
    if (!config->misc.bunnyHop)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::OBSERVER
        || localPlayer->isInBumperKart() || localPlayer->isAGhost() || localPlayer->isSwimming())
        return;

    const bool isOnGround = localPlayer->isOnGround();
    static bool jumpState = false;

    if (cmd->buttons & UserCmd::IN_JUMP) {
        if (!jumpState && !isOnGround)
            cmd->buttons &= ~UserCmd::IN_JUMP;
        else if (jumpState)
            jumpState = false;
    }
    else if (!jumpState)
        jumpState = true;
}

void Misc::autoStrafe(UserCmd* cmd, Vector& currentViewAngles) noexcept
{
    if (!config->misc.autoStrafe)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    static float angle = 0.f;

    const float speed = localPlayer->velocity().length2D();

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

    if (localPlayer->isInBumperKart() || localPlayer->isAGhost() || localPlayer->isSwimming()
        || localPlayer->moveType() == MoveType::NOCLIP || localPlayer->moveType() == MoveType::OBSERVER)
        return;

    const bool isOnGround = localPlayer->isOnGround();
    static bool jumpState = false;
    static bool recentltyJumped = false;

    if (cmd->buttons & UserCmd::IN_JUMP) {
        if (!jumpState && !isOnGround)
            cmd->buttons &= ~UserCmd::IN_JUMP;
        else if (jumpState)
        {
            jumpState = false;
            recentltyJumped = true;
            return;
        }
    }
    else if (!jumpState)
        jumpState = true;

    if (isOnGround)
    {
        recentltyJumped = false;
        return;
    }

    if (speed < 5.0f && !recentltyJumped)
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
    //StAC detects any value above/equal to (1 << 26)
    //https://github.com/sapphonie/StAC-tf2/blob/51b3eb3862331e428c90234f491a951a6c638375/scripting/stac/stac_onplayerruncmd.sp#L1081
    if (config->misc.antiAfkKick && cmd->commandNumber % 2)
        cmd->buttons |= UserCmd::IN_ALT1;
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

void Misc::drawPlayerList() noexcept
{
    if (!config->misc.playerList.enabled)
        return;

    if (config->misc.playerList.pos != ImVec2{}) {
        ImGui::SetNextWindowPos(config->misc.playerList.pos);
        config->misc.playerList.pos = {};
    }

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (!gui->isOpen())
    {
        windowFlags |= ImGuiWindowFlags_NoInputs;
        return;
    }

    GameData::Lock lock;
    if ((GameData::players().empty()) && !gui->isOpen())
        return;

    const auto& displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(displaySize.x, displaySize.x / 2.5f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, 300.0f), ImGuiCond_Once);

    if (ImGui::Begin("Player List", nullptr, windowFlags)) {
        if (ImGui::beginTable("", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 120.0f);
            ImGui::TableSetupColumn("Steam ID", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Health", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
            ImGui::TableSetupColumn("Extra", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetColumnEnabled(2, config->misc.playerList.steamID);
            ImGui::TableSetColumnEnabled(3, config->misc.playerList.className);
            ImGui::TableSetColumnEnabled(4, config->misc.playerList.health);

            ImGui::TableHeadersRow();

            std::vector<std::reference_wrapper<const PlayerData>> playersOrdered{ GameData::players().begin(), GameData::players().end() };
            std::ranges::sort(playersOrdered, [](const PlayerData& a, const PlayerData& b) {
                // enemies first
                if (a.enemy != b.enemy)
                    return a.enemy && !b.enemy;

                return a.handle < b.handle;
                });

            ImGui::PushFont(gui->getUnicodeFont());

            for (const PlayerData& player : playersOrdered) {
                ImGui::TableNextRow();
                ImGui::PushID(ImGui::TableGetRowIndex());

                if (ImGui::TableNextColumn())
                    ImGui::Text("%d", player.userId);

                if (ImGui::TableNextColumn()) {
                    ImGui::Image(player.getAvatarTexture(), { ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() });
                    ImGui::SameLine();
                    ImGui::textEllipsisInTableCell(player.name.c_str());
                }

                if (ImGui::TableNextColumn() && ImGui::smallButtonFullWidth("Copy", player.steamID == 0))
                    ImGui::SetClipboardText(std::to_string(player.steamID).c_str());

                if (ImGui::TableNextColumn()) {
                    std::string className = "unknown";
                    switch (player.classID) {
                    case TFClass::SCOUT:
                        className = "Scout";
                        break;
                    case TFClass::SNIPER:
                        className = "Sniper";
                        break;
                    case TFClass::SOLDIER:
                        className = "Soldier";
                        break;
                    case TFClass::DEMOMAN:
                        className = "Demoman";
                        break;
                    case TFClass::MEDIC:
                        className = "Medic";
                        break;
                    case TFClass::HEAVY:
                        className = "Heavy";
                        break;
                    case TFClass::PYRO:
                        className = "Pyro";
                        break;
                    case TFClass::SPY:
                        className = "Spy";
                        break;
                    case TFClass::ENGINEER:
                        className = "Engineer";
                        break;
                    }
                    ImGui::Text(className.c_str());
                }

                if (ImGui::TableNextColumn()) {
                    if (!player.alive)
                        ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, "%s", "Dead");
                    else
                        ImGui::Text("%d HP", player.health);
                }

                if (ImGui::TableNextColumn()) {
                    const auto targetPlayer = TargetSystem::playerByHandle(player.handle);
                    if (!targetPlayer)
                        ImGui::Text("unknown");
                    else {
                        switch (targetPlayer->priority) {
                        case 0:
                            ImGui::Text("Na");
                            break;
                        case 1:
                            ImGui::Text("Base");
                            break;
                        case 2:
                            ImGui::Text("Low");
                            break;
                        case 3:
                            ImGui::Text("High");
                            break;
                        default:
                            ImGui::Text("unknown");
                            break;
                        }
                    }
                }

                if (ImGui::TableNextColumn()) {
                    if (ImGui::smallButtonFullWidth("...", false))
                        ImGui::OpenPopup("##options");

                    if (ImGui::BeginPopup("##options", ImGuiWindowFlags_AlwaysUseWindowPadding)) {
                        if (GameData::local().exists) {
                            if (ImGui::Button("Set to high priority"))
                                TargetSystem::setPriority(player.handle, 3);

                            if (ImGui::Button("Set to low priority"))
                                TargetSystem::setPriority(player.handle, 2);

                            if (ImGui::Button("Set to no priority"))
                                TargetSystem::setPriority(player.handle, 0);

                            if (ImGui::Button("Reset priority"))
                                TargetSystem::setPriority(player.handle, 1);

                            if (player.steamID != 0 && player.team == GameData::local().team 
                                && ImGui::Button("Kick")) {
                                const std::string cmd = "callvote kick " + std::to_string(player.userId);
                                interfaces->engine->clientCmdUnrestricted(cmd.c_str());
                            }
                        }

                        ImGui::EndPopup();
                    }
                }

                ImGui::PopID();
            }

            ImGui::PopFont();
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

// ImGui::ShadeVertsLinearColorGradientKeepAlpha() modified to do interpolation in HSV
static void shadeVertsHSVColorGradientKeepAlpha(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImU32 col0, ImU32 col1)
{
    ImVec2 gradient_extent = gradient_p1 - gradient_p0;
    float gradient_inv_length2 = 1.0f / ImLengthSqr(gradient_extent);
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;

    ImVec4 col0HSV = ImGui::ColorConvertU32ToFloat4(col0);
    ImVec4 col1HSV = ImGui::ColorConvertU32ToFloat4(col1);
    ImGui::ColorConvertRGBtoHSV(col0HSV.x, col0HSV.y, col0HSV.z, col0HSV.x, col0HSV.y, col0HSV.z);
    ImGui::ColorConvertRGBtoHSV(col1HSV.x, col1HSV.y, col1HSV.z, col1HSV.x, col1HSV.y, col1HSV.z);
    ImVec4 colDelta = col1HSV - col0HSV;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        float d = ImDot(vert->pos - gradient_p0, gradient_extent);
        float t = ImClamp(d * gradient_inv_length2, 0.0f, 1.0f);

        float h = col0HSV.x + colDelta.x * t;
        float s = col0HSV.y + colDelta.y * t;
        float v = col0HSV.z + colDelta.z * t;

        ImVec4 rgb;
        ImGui::ColorConvertHSVtoRGB(h, s, v, rgb.x, rgb.y, rgb.z);
        vert->col = (ImGui::ColorConvertFloat4ToU32(rgb) & ~IM_COL32_A_MASK) | (vert->col & IM_COL32_A_MASK);
    }
}

void Misc::drawOffscreenEnemies(ImDrawList* drawList) noexcept
{
    if (!config->misc.offscreenEnemies.enabled)
        return;

    //TODO: Take pitch into account
    const auto yaw = Helpers::deg2rad(interfaces->engine->getViewAngles().y);

    GameData::Lock lock;
    for (auto& player : GameData::players()) {
        if ((player.dormant && player.fadingAlpha() == 0.0f) || !player.alive || !player.enemy || player.inViewFrustum || (config->misc.offscreenEnemies.disableOnCloaked && player.isCloaked))
            continue;

        const auto positionDiff = GameData::local().origin - player.origin;

        auto x = std::cos(yaw) * positionDiff.y - std::sin(yaw) * positionDiff.x;
        auto y = std::cos(yaw) * positionDiff.x + std::sin(yaw) * positionDiff.y;
        if (const auto len = std::sqrt(x * x + y * y); len != 0.0f) {
            x /= len;
            y /= len;
        }

        constexpr auto avatarRadius = 13.0f;
        constexpr auto triangleSize = 10.0f;

        const auto pos = ImGui::GetIO().DisplaySize / 2 + ImVec2{ x, y } *200;
        const auto trianglePos = pos + ImVec2{ x, y } *(avatarRadius + (config->misc.offscreenEnemies.healthBar.enabled ? 5 : 3));

        Helpers::setAlphaFactor(player.fadingAlpha());
        const auto white = Helpers::calculateColor(255, 255, 255, 255);
        const auto background = Helpers::calculateColor(0, 0, 0, 80);
        const auto color = Helpers::calculateColor(config->misc.offscreenEnemies);
        const auto healthBarColor = config->misc.offscreenEnemies.healthBar.type == HealthBar::HealthBased ? Helpers::healthColor(std::clamp(player.health / 100.0f, 0.0f, 1.0f)) : Helpers::calculateColor(config->misc.offscreenEnemies.healthBar);
        Helpers::setAlphaFactor(1.0f);

        const ImVec2 trianglePoints[]{
            trianglePos + ImVec2{  0.4f * y, -0.4f * x } *triangleSize,
            trianglePos + ImVec2{  1.0f * x,  1.0f * y } *triangleSize,
            trianglePos + ImVec2{ -0.4f * y,  0.4f * x } *triangleSize
        };

        drawList->AddConvexPolyFilled(trianglePoints, 3, color);
        drawList->AddCircleFilled(pos, avatarRadius + 1, white & IM_COL32_A_MASK, 40);

        const auto texture = player.getAvatarTexture();

        const bool pushTextureId = drawList->_TextureIdStack.empty() || texture != drawList->_TextureIdStack.back();
        if (pushTextureId)
            drawList->PushTextureID(texture);

        const int vertStartIdx = drawList->VtxBuffer.Size;
        drawList->AddCircleFilled(pos, avatarRadius, white, 40);
        const int vertEndIdx = drawList->VtxBuffer.Size;
        ImGui::ShadeVertsLinearUV(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ avatarRadius, avatarRadius }, pos + ImVec2{ avatarRadius, avatarRadius }, { 0, 0 }, { 1, 1 }, true);

        if (pushTextureId)
            drawList->PopTextureID();

        if (config->misc.offscreenEnemies.healthBar.enabled) {
            const auto radius = avatarRadius + 2;
            const auto healthFraction = std::clamp(player.health / 100.0f, 0.0f, 1.0f);

            drawList->AddCircle(pos, radius, background, 40, 3.0f);

            const int vertStartIdx = drawList->VtxBuffer.Size;
            if (healthFraction == 1.0f) { // sometimes PathArcTo is missing one top pixel when drawing a full circle, so draw it with AddCircle
                drawList->AddCircle(pos, radius, healthBarColor, 40, 2.0f);
            }
            else {
                constexpr float pi = std::numbers::pi_v<float>;
                drawList->PathArcTo(pos, radius - 0.5f, pi / 2 - pi * healthFraction, pi / 2 + pi * healthFraction, 40);
                drawList->PathStroke(healthBarColor, false, 2.0f);
            }
            const int vertEndIdx = drawList->VtxBuffer.Size;

            if (config->misc.offscreenEnemies.healthBar.type == HealthBar::Gradient)
                shadeVertsHSVColorGradientKeepAlpha(drawList, vertStartIdx, vertEndIdx, pos - ImVec2{ 0.0f, radius }, pos + ImVec2{ 0.0f, radius }, IM_COL32(0, 255, 0, 255), IM_COL32(255, 0, 0, 255));
        }
    }
}

void Misc::revealVotes(GameEvent* event) noexcept
{
    if (!config->misc.revealVotes)
        return;

    const auto entity = interfaces->entityList->getEntity(event->getInt("entityid"));
    if (!entity || !entity->isPlayer())
        return;

    static std::string red({ '\x7', 'F', 'F', '0', '0', '0', '0' }); // \x7 RRGGBB
    static std::string green({ '\x7', '0', '0', 'F', 'F', '0', '0' }); // \x7 RRGGBB
    static std::string blue({ '\x7', '7', '2', 'B', 'C', 'D', '4' }); // \x7 RRGGBB

    const auto votedYes = event->getInt("vote_option") == 0;
    const auto isLocal = localPlayer && entity == localPlayer.get();

    memory->clientMode->getHudChat()->printf(
        "%s-Seth-%s %s\x01 voted %s%s\x01",
        blue.c_str(), isLocal ? "\x01" : (votedYes ? green.c_str() : red.c_str()), isLocal ? "You" : entity->getPlayerName().c_str(),
        votedYes ? green.c_str() : red.c_str(),
        votedYes ? "Yes" : "No");
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

static int buttons = 0;

void Misc::runFreeCam(UserCmd* cmd, Vector viewAngles) noexcept
{
    static Vector currentViewAngles = Vector{ };
    static Vector realViewAngles = Vector{ };
    static bool wasCrouching = false;
    static bool wasHoldingAttack = false;
    static bool wasHoldingUse = false;
    static bool hasSetAngles = false;

    buttons = cmd->buttons;
    if (!config->visuals.freeCam || !config->visuals.freeCamKey.isActive())
    {
        if (hasSetAngles)
        {
            interfaces->engine->setViewAngles(realViewAngles);
            cmd->viewangles = currentViewAngles;
            if (wasCrouching)
                cmd->buttons |= UserCmd::IN_DUCK;
            if (wasHoldingAttack)
                cmd->buttons |= UserCmd::IN_ATTACK;
            if (wasHoldingUse)
                cmd->buttons |= UserCmd::IN_USE;
            wasCrouching = false;
            wasHoldingAttack = false;
            wasHoldingUse = false;
            hasSetAngles = false;
        }
        currentViewAngles = Vector{};
        return;
    }

    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (currentViewAngles.null())
    {
        currentViewAngles = cmd->viewangles;
        realViewAngles = viewAngles;
        wasCrouching = cmd->buttons & UserCmd::IN_DUCK;
    }

    cmd->forwardmove = 0;
    cmd->sidemove = 0;
    cmd->buttons = 0;
    if (wasCrouching)
        cmd->buttons |= UserCmd::IN_DUCK;
    if (wasHoldingAttack)
        cmd->buttons |= UserCmd::IN_ATTACK;
    if (wasHoldingUse)
        cmd->buttons |= UserCmd::IN_USE;
    cmd->viewangles = currentViewAngles;
    hasSetAngles = true;
}

void Misc::freeCam(ViewSetup* setup) noexcept
{
    static Vector newOrigin = Vector{ };

    if (!config->visuals.freeCam || !config->visuals.freeCamKey.isActive())
    {
        newOrigin = Vector{ };
        return;
    }

    if (!localPlayer || !localPlayer->isAlive())
        return;

    float freeCamSpeed = fabsf(static_cast<float>(config->visuals.freeCamSpeed));

    if (newOrigin.null())
        newOrigin = setup->origin;

    Vector forward{ }, right{ }, up{ };

    Vector::fromAngleAll(setup->angles, &forward, &right, &up);

    const bool backBtn = buttons & UserCmd::IN_BACK;
    const bool forwardBtn = buttons & UserCmd::IN_FORWARD;
    const bool rightBtn = buttons & UserCmd::IN_MOVERIGHT;
    const bool leftBtn = buttons & UserCmd::IN_MOVELEFT;
    const bool shiftBtn = buttons & UserCmd::IN_SPEED;
    const bool duckBtn = buttons & UserCmd::IN_DUCK;
    const bool jumpBtn = buttons & UserCmd::IN_JUMP;

    if (duckBtn)
        freeCamSpeed *= 0.45f;

    if (shiftBtn)
        freeCamSpeed *= 1.65f;

    if (forwardBtn)
        newOrigin += forward * freeCamSpeed;

    if (rightBtn)
        newOrigin += right * freeCamSpeed;

    if (leftBtn)
        newOrigin -= right * freeCamSpeed;

    if (backBtn)
        newOrigin -= forward * freeCamSpeed;

    if (jumpBtn)
        newOrigin += up * freeCamSpeed;

    setup->origin = newOrigin;
}


void Misc::updateInput() noexcept
{
    config->misc.edgeJumpKey.handleToggle();
}

void Misc::reset(int resetType) noexcept
{

}
