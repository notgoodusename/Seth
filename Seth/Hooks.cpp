#include <functional>
#include <string>

#include "imgui/imgui.h"

#include <intrin.h>
#include <Windows.h>
#include <Psapi.h>

#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "MinHook/MinHook.h"

#include "Config.h"
#include "GameData.h"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"
#include "SteamInterfaces.h"

#include "Hacks/Aimbot/Aimbot.h"
#include "Hacks/Animations.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/Crithack.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/Fakelag.h"
#include "Hacks/Glow.h"
#include "Hacks/Misc.h"
#include "Hacks/MovementRebuild.h"
#include "Hacks/ProjectileTrajectory.h"
#include "Hacks/SkinChanger.h"
#include "Hacks/StreamProofESP.h"
#include "Hacks/TargetSystem.h"
#include "Hacks/Tickbase.h"
#include "Hacks/Triggerbot/Triggerbot.h"
#include "Hacks/Visuals.h"

#include "SDK/ClassId.h"
#include "SDK/Client.h"
#include "SDK/ClientState.h"
#include "SDK/ConVar.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/FrameStage.h"
#include "SDK/GameEvent.h"
#include "SDK/GlobalVars.h"
#include "SDK/Input.h"
#include "SDK/InputSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/Panel.h"
#include "SDK/Platform.h"
#include "SDK/Prediction.h"
#include "SDK/RenderView.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"
#include "SDK/ViewSetup.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    [[maybe_unused]] static const auto once = [](HWND window) noexcept {
        Netvars::init();
        Misc::initHiddenCvars();

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(window);
        config = std::make_unique<Config>();
        gui = std::make_unique<GUI>();

        hooks->install();

        return true;
    }(window);

    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);

    interfaces->inputSystem->enableInput(!gui->isOpen());

    return CallWindowProcW(hooks->originalWndProc, window, msg, wParam, lParam);
}

static HRESULT __stdcall present(IDirect3DDevice9* device, const RECT* src, const RECT* dest, HWND windowOverride, const RGNDATA* dirtyRegion) noexcept
{
    [[maybe_unused]] static bool imguiInit{ ImGui_ImplDX9_Init(device) };

    if (config->loadScheduledFonts())
        ImGui_ImplDX9_DestroyFontsTexture();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (const auto& displaySize = ImGui::GetIO().DisplaySize; displaySize.x > 0.0f && displaySize.y > 0.0f) {
        StreamProofESP::render();

        Misc::drawOffscreenEnemies(ImGui::GetBackgroundDrawList());
        Misc::showKeybinds();
        Misc::spectatorList();

        Aimbot::updateInput();
        Visuals::updateInput();
        StreamProofESP::updateInput();
        Triggerbot::updateInput();
        Misc::updateInput();
        Chams::updateInput();
        Crithack::updateInput();

        Misc::drawPlayerList();
        ProjectileTrajectory::draw(ImGui::GetBackgroundDrawList());
        Crithack::draw(ImGui::GetForegroundDrawList());
        Misc::drawAimbotFov(ImGui::GetBackgroundDrawList());

        gui->handleToggle();

        if (gui->isOpen())
            gui->render();
    }

    ImGui::EndFrame();
    ImGui::Render();

    if (device->BeginScene() == D3D_OK) {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }

    GameData::clearUnusedAvatars();

    return hooks->originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    GameData::clearTextures();
    return hooks->originalReset(device, params);
}

static bool __fastcall createMove(void* thisPointer, void*, float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 21>(inputSampleTime, cmd);

    Backtrack::updateSequences();

    if (!cmd || !cmd->commandNumber)
        return result;

    uintptr_t framePointer;
    __asm mov framePointer, ebp;
    bool& sendPacket = *reinterpret_cast<bool*>(***reinterpret_cast<uintptr_t***>(framePointer) - 0x1);
    
    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };

    memory->globalVars->serverTime(cmd);
    Misc::runFreeCam(cmd, currentViewAngles);
    Tickbase::start(cmd);
    Misc::antiAfkKick(cmd);
    Misc::fastStop(cmd);
    Misc::bunnyHop(cmd);
    Misc::autoStrafe(cmd, currentViewAngles);

    EnginePrediction::update();
    EnginePrediction::run(cmd);
    ProjectileTrajectory::calculate(cmd);

    Backtrack::run(cmd);
    Aimbot::run(cmd);
    Triggerbot::run(cmd);
    Tickbase::end(cmd);

    Misc::edgejump(cmd);

    Fakelag::run(sendPacket);

    Crithack::run(cmd);

    cmd->viewangles.normalize();

    if ((currentViewAngles != cmd->viewangles
        || cmd->forwardmove != currentCmd.forwardmove
        || cmd->sidemove != currentCmd.sidemove) && (cmd->sidemove != 0 || cmd->forwardmove != 0))
    {
        Misc::fixMovement(cmd, currentViewAngles.y);
    }

    cmd->viewangles.x = std::clamp(cmd->viewangles.x, -89.0f, 89.0f);
    cmd->viewangles.y = std::clamp(cmd->viewangles.y, -180.0f, 180.0f);
    cmd->viewangles.z = 0.0f;
    cmd->forwardmove = std::clamp(cmd->forwardmove, -450.0f, 450.0f);
    cmd->sidemove = std::clamp(cmd->sidemove, -450.0f, 450.0f);
    cmd->upmove = std::clamp(cmd->upmove, -320.0f, 320.0f);
    Animations::updateLocalAngles(cmd);
    return false;
}

static void __fastcall frameStageNotify(void* thisPointer, void*, FrameStage stage) noexcept
{
    static auto original = hooks->client.getOriginal<void, 35>(stage);

    static auto animationInit = (Animations::init(), false);
    static auto backtrackInit = (Backtrack::init(), false);
    static auto movementRebuildInit = (MovementRebuild::init(), false);

    if (stage == FrameStage::START)
    {
        GameData::update();
        SkinChanger::run();
    }

    if (stage == FrameStage::NET_UPDATE_END)
    {
        TargetSystem::updateFrame();
        Crithack::updatePlayers();
    }

    if (stage == FrameStage::RENDER_START) {
        Misc::unlockHiddenCvars();
    }
    
    original(thisPointer, stage);
}

static bool __fastcall dispatchUserMessage(void* thisPointer, void*, int messageType, bufferRead* data) noexcept
{
    static auto original = hooks->client.getOriginal<bool, 36>(messageType, data);

    return original(thisPointer, messageType, data);
}

static bool __fastcall doPostScreenEffects(void* thisPointer, void*, const ViewSetup* setup) noexcept
{
    if (interfaces->engine->isInGame())
    {
        //Glow::render();
    }
    return hooks->clientMode.callOriginal<bool, 39>(setup);
}

static void __fastcall drawModelExecute(void* thisPointer, void*, void* state, const ModelRenderInfo& info, matrix3x4* customBoneToWorld) noexcept
{
    static Chams chams;
    if (!chams.render(state, info, customBoneToWorld))
        hooks->modelRender.callOriginal<void, 19>(state, std::cref(info), customBoneToWorld);

    interfaces->renderView->setColorModulation(1.0f, 1.0f, 1.0f);
    interfaces->renderView->setBlend(1.0f);
    interfaces->modelRender->forcedMaterialOverride(nullptr);
}

static void __stdcall overrideView(ViewSetup* setup) noexcept
{
    if (localPlayer && !localPlayer->isScoped())
        setup->fov += config->visuals.fov;

    hooks->clientMode.callOriginal<void, 16>(setup);
    Visuals::thirdperson();
    Misc::freeCam(setup);
}

static unsigned int vguiFocusOverlayPanel = NULL;

static void __fastcall paintTraverse(void* thisPointer, void*, unsigned int vguiPanel, bool forceRepaint, bool allowForce) noexcept
{
    hooks->panel.callOriginal<void, 41>(vguiPanel, forceRepaint, allowForce);

    if (vguiFocusOverlayPanel == NULL)
    {
        const char* name = interfaces->panel->getName(vguiPanel);
        if (name[0] == 'F' && name[5] == 'O' && name[12] == 'P')
            vguiFocusOverlayPanel = vguiPanel;
    }

    if (vguiFocusOverlayPanel == vguiPanel)
        interfaces->panel->setMouseInputEnabled(vguiPanel, gui->isOpen());
}

static void __stdcall lockCursor() noexcept
{
    if (gui->isOpen())
        return interfaces->surface->unlockCursor();
    return hooks->surface.callOriginal<void, 62>();
}

static bool __fastcall fireEventClientSide(void* thisPointer, void*, GameEvent* event) noexcept
{
    static auto original = hooks->eventManager.getOriginal<bool, 8>(event);

    if (!event)
        return original(thisPointer, event);

    switch (fnv::hashRuntime(event->getName())) 
    {
        case fnv::hash("teamplay_round_start"):
        case fnv::hash("player_hurt"):
            Crithack::handleEvent(event);
            break;
        case fnv::hash("vote_cast"):
            Misc::revealVotes(event);
            break;
    }

    return original(thisPointer, event);
}

static UserCmd* __stdcall getUserCmd(int sequenceNumber) noexcept
{
    const auto commands = *reinterpret_cast<UserCmd**>(reinterpret_cast<uintptr_t>(memory->input) + 0xFC);
    if (commands)
        if (UserCmd* cmd = &commands[sequenceNumber % 90])
            return cmd;

    return nullptr;
}

static void __fastcall addToCritBucketHook(void* thisPointer, void*, const float amount) noexcept
{
    static auto original = hooks->addToCritBucket.getOriginal<void>(amount);
    if (Crithack::protectData())
        return;

    original(thisPointer, amount);
}

static void __fastcall calcIsAttackCriticalHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->calcIsAttackCritical.getOriginal<void>();

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (!entity || !localPlayer || entity != localPlayer->getActiveWeapon())
        return original(thisPointer);

    if (!Crithack::isAttackCriticalHandler(entity))
        return;

    const auto previousWeaponMode = entity->weaponMode();
    entity->weaponMode() = 0;
    original(thisPointer);
    entity->weaponMode() = previousWeaponMode;
}

static float __fastcall calculateChargeCapHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->calculateChargeCap.getOriginal<float>();

    const float backupFrameTime = memory->globalVars->frameTime;
    memory->globalVars->frameTime = FLT_MAX;
    float finalValue = original(thisPointer);
    memory->globalVars->frameTime = backupFrameTime;
    return finalValue;
}

static void __fastcall calcViewModelViewHook(void* thisPointer, void*, Entity* owner, Vector* eyePosition, Vector* eyeAngles) noexcept
{
    static auto original = hooks->calcViewModelView.getOriginal<void>(owner, eyePosition, eyeAngles);

    auto eyePositionCopy = eyePosition;
    auto eyeAnglesCopy = eyeAngles;

    Misc::viewModelChanger(*eyePositionCopy, *eyeAnglesCopy);

    original(thisPointer, owner, eyePositionCopy, eyeAnglesCopy);
}

static bool __fastcall canFireRandomCriticalShotHook(void* thisPointer, void*, float critChance) noexcept
{
    static auto original = hooks->canFireRandomCriticalShot.getOriginal<bool>(critChance);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (entity && localPlayer && entity == localPlayer->getActiveWeapon())
        Crithack::handleCanFireRandomCriticalShot(critChance, entity);

    return original(thisPointer, critChance);
}

static void __fastcall checkForSequenceChangeHook(void* thisPointer, void*, CStudioHdr* hdr, int currentSequence, bool forceSequence, bool interpolate) noexcept
{
    static auto original = hooks->checkForSequenceChange.getOriginal<void>(hdr, currentSequence, forceSequence, interpolate);
    original(thisPointer, hdr, currentSequence, forceSequence, false);
}

static void* __cdecl clLoadWhitelistHook(void* whitelist, const char* name) noexcept
{
    static auto original = reinterpret_cast<void*(__cdecl*)(void*, const char*)>(hooks->enableWorldFog.getDetour());
    if(config->misc.svPureBypass)
        return NULL;
    return original(whitelist, name);
}

static void __cdecl clMoveHook(float accumulatedExtraSamples, bool isFinalTick) noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)(float, bool)>(hooks->clMove.getDetour());

    if (!Tickbase::canRun())
        return;

    original(accumulatedExtraSamples, isFinalTick);

    if (!Tickbase::getTickshift())
        return;

    int remainToShift = 0;
    int tickShift = Tickbase::getTickshift();

    Tickbase::isShifting() = true;

    for (int shiftAmount = 0; shiftAmount < tickShift; shiftAmount++)
    {
        Tickbase::isFinalTick() = (tickShift - shiftAmount) == 1;
        original(accumulatedExtraSamples, isFinalTick);
    }
    Tickbase::isShifting() = false;

    Tickbase::resetTickshift();
}

static void __cdecl clSendMoveHook() noexcept
{
    int nextCommandNumber = memory->clientState->lastOutgoingCommand + memory->clientState->chokedCommands + 1;
    int chokedCommands = memory->clientState->chokedCommands;

    byte data[4000 /* MAX_CMD_BUFFER */];
    CLC_Move moveMsg;

    moveMsg.dataOut.startWriting(data, sizeof(data));

    const int newCommands = std::clamp(max(chokedCommands + 1, 0), 0, MAX_NEW_COMMANDS);
    const int extraCommands = (chokedCommands + 1) - newCommands;
    const int backupCommands = std::clamp(max(2, extraCommands), 0, MAX_BACKUP_COMMANDS);

    moveMsg.backupCommands = backupCommands;
    moveMsg.newCommands = newCommands;

    const int numCmds = newCommands + backupCommands;
    int from = -1;
    bool ok = true;

    for (int to = nextCommandNumber - numCmds + 1; to <= nextCommandNumber; ++to) {

        bool isnewcmd = to >= (nextCommandNumber - newCommands + 1);

        ok = ok && interfaces->client->writeUsercmdDeltaToBuffer(&moveMsg.dataOut, from, to, isnewcmd);
        from = to;
    }

    if (ok) 
    {
        if (extraCommands)
            memory->clientState->networkChannel->chokedPackets -= extraCommands;

        memory->clientState->networkChannel->sendNetMsg(&moveMsg, false, false);
    }
}

static void __fastcall customTextureOnItemProxyOnBindInternalHook(void* thisPointer, void*, void* scriptItem) noexcept
{
    static auto original = hooks->customTextureOnItemProxyOnBindInternal.getOriginal<void>(scriptItem);
    if (config->visuals.disableCustomDecals)
        return;

    original(thisPointer, scriptItem);
}

static void __cdecl doEnginePostProcessingHook(int x, int y, int w, int h, bool flashlightIsOn, bool postVGui) noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)(int, int, int, int, bool, bool)>(hooks->doEnginePostProcessing.getDetour());
    if (config->visuals.disablePostProcessing)
        return;

    original(x, y, w, h, flashlightIsOn, postVGui);
}

static void __cdecl enableWorldFogHook() noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)()>(hooks->enableWorldFog.getDetour());
    if (config->visuals.noFog)
        return;
    return original();
}

static void __fastcall estimateAbsVelocityHook(void* thisPointer, void*, Vector& vel) noexcept
{
    static auto original = hooks->estimateAbsVelocity.getOriginal<void>(&vel);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (localPlayer 
        && entity != localPlayer.get() 
        && entity->isPlayer())
    {
        entity->getAbsVelocity(vel);
        return;
    }

    original(thisPointer, &vel);
}

struct FireBulletsInfo
{
    int shots;
    Vector source;
    Vector directionShooting;
    Vector spread;
    float distance;
    int ammoType;
    int tracerFrequency;
    float damage;
    int playerDamage;
    int flags;
    float damageForceScale;
    Entity* attacker;
    Entity* additionalIgnoreEntity;
    bool primaryAttack;
    bool useServerRandomSeed;
};

static void __fastcall fireBulletHook(void* thisPointer, void*, Entity* weapon, const FireBulletsInfo& info, bool doEffects, int damageType, int customDamageType) noexcept
{
    static auto original = hooks->fireBullet.getOriginal<void>(weapon, info, doEffects, damageType, customDamageType);
    /*
    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (config->visuals.bulletTracers.enabled 
        && localPlayer && entity == localPlayer.get())
    {
        //Remove crit type so tracers some tracers don't break.
        if (damageType & (1 << 20))
            damageType &= ~(1 << 20);

        //Draws tracer on each shot
        //info.tracerFrequency = 1;
    }*/

    return original(thisPointer, weapon, info, doEffects, damageType, customDamageType);
}

std::vector<std::pair<int, float>> simTimes;

static float __fastcall frameAdvanceHook(void* thisPointer, void*, float interval) noexcept
{
    static auto original = hooks->frameAdvance.getOriginal<float>(interval);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (!entity || !localPlayer || !entity->getModelPtr() || !entity->isPlayer())
        return original(thisPointer, interval);

    const auto it = std::find_if(simTimes.begin(), simTimes.end(),
        [&](const auto& pair) { return pair.first == entity->handle(); });

    if (it == simTimes.end())
    {
        std::pair<int, float> newHandle;
        newHandle.first = entity->handle();
        newHandle.second = entity->simulationTime();
        simTimes.push_back(newHandle);
        std::sort(simTimes.begin(), simTimes.end());
        return original(thisPointer, interval);
    }
    else if (entity->simulationTime() != it->second)
    {
        const float newInterval = entity->simulationTime() - it->second;
        it->second = entity->simulationTime();
        if (newInterval > 0.0f)
            return original(thisPointer, newInterval);
    }
    return 0.0f;
}

static const char* __fastcall getTraceTypeHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->getTraceType.getOriginal<const char*>();
    if (!thisPointer || !localPlayer)
        return original(thisPointer);

    switch (config->visuals.bulletTracers.type)
    {
        case 1:
            return (localPlayer->teamNumber() == Team::RED ? "dxhr_sniper_rail_red" : "dxhr_sniper_rail_blue");
        case 2:
            return (localPlayer->isCritBoosted() ?
                (localPlayer->teamNumber() == Team::RED ? "bullet_tracer_raygun_red_crit" : "bullet_tracer_raygun_blue_crit") :
                (localPlayer->teamNumber() == Team::RED ? "bullet_tracer_raygun_red" : "bullet_tracer_raygun_blue"));
        case 3:
            return localPlayer->teamNumber() == Team::RED ? "dxhr_lightningball_hit_zap_red" : "dxhr_lightningball_hit_zap_blue";
        case 4:
            return "merasmus_zap";
        case 5:
            return "merasmus_zap_beam02";
        case 6:
            return localPlayer->isCritBoosted() ?
                (localPlayer->teamNumber() == Team::RED ? "bullet_bignasty_tracer01_red_crit" : "bullet_bignasty_tracer01_blue_crit") :
                (localPlayer->teamNumber() == Team::RED ? "bullet_bignasty_tracer01_blue" : "bullet_bignasty_tracer01_red");
        default:
            break;
    }

    return original(thisPointer);
}

static bool __fastcall interpolateHook(void* thisPointer, void*, float currentTime) noexcept
{
    static auto original = hooks->interpolate.getOriginal<bool>(currentTime);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (!entity || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, currentTime);

    if (Tickbase::pausedTicks())
        return true;

    return original(thisPointer, currentTime);
}

static void __cdecl interpolateServerEntitiesHook() noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)()>(hooks->interpolateServerEntities.getDetour());

    static auto extrapolate = interfaces->cvar->findVar("cl_extrapolate");
    if(extrapolate->getInt() != 0)
        extrapolate->setValue(0);
    original();
}

static bool __fastcall interpolateViewModelHook(void* thisPointer, void*, float currentTime) noexcept
{
    static auto original = hooks->interpolateViewModel.getOriginal<bool>(currentTime);
    
    const auto viewModel = reinterpret_cast<Entity*>(thisPointer);
    if(!viewModel)
        return original(thisPointer, currentTime);

    const auto entity = interfaces->entityList->getEntityFromHandle(viewModel->owner());
    if (!entity || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, currentTime);

    const int finalPredictedTick = entity->finalPredictedTick();
    const float interpolationAmount = memory->globalVars->interpolationAmount;

    entity->finalPredictedTick() = timeToTicks(memory->globalVars->currentTime);
    memory->globalVars->interpolationAmount = 0.f;

    const bool result = original(thisPointer, currentTime);

    entity->finalPredictedTick() = finalPredictedTick;
    memory->globalVars->interpolationAmount = interpolationAmount;

    return result;
}

static bool __fastcall isAllowedToWithdrawFromCritBucketHook(void* thisPointer, void*, float damage) noexcept
{
    static auto original = hooks->isAllowedToWithdrawFromCritBucket.getOriginal<bool>(damage);

    Crithack::setCorrectDamage(damage);

    return original(thisPointer, damage);
}

static void __fastcall newMatchFoundDashboardStateOnUpdateHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->newMatchFoundDashboardStateOnUpdate.getOriginal<void>();

    auto autoJoinTime = reinterpret_cast<double*>(reinterpret_cast<uintptr_t>(thisPointer) + 424);

    if (config->misc.autoAccept)
        *autoJoinTime = -10.0;

    original(thisPointer);
}

static void __fastcall runSimulationHook(void* thisPointer, void*, int currentCommand, float currentTime, UserCmd* cmd, Entity* entity) noexcept
{
    static auto original = hooks->runSimulation.getOriginal<void>(currentCommand, currentTime, cmd, entity);

    if(!entity || !localPlayer || entity != localPlayer.get())
        return original(thisPointer, currentCommand, currentTime, cmd, entity);

    if (Tickbase::pausedTicks())
    {
        //entity->tickBase() = Tickbase::getCorrectTickbase(-Tickbase::pausedTicks());

        if (currentCommand != Tickbase::getShiftCommandNumber())
        {
            Tickbase::pausedTicks() = 0;
            return;
        }
    }

    //for this case we have to wait 2 ticks
    // runSimulation (normal -> createmove we send shift
    // runSimulation (normal, because we havent shifted yet) -> createmove
    // runSimulation (SHIFT,  tickbase - shiftedamount) -> createmove

    static int tickWait = 0;
    static int savedCommandNumber = 0;

    if (savedCommandNumber == currentCommand)
        entity->tickBase() = Tickbase::getCorrectTickbase(Tickbase::getShiftedTickbase());

    if (tickWait)
    {
        savedCommandNumber = currentCommand;
        tickWait++;
        if (tickWait >= 3)
        {
            tickWait = 0;
            entity->tickBase() = Tickbase::getCorrectTickbase(Tickbase::getShiftedTickbase());
        }
    }

    if (currentCommand == Tickbase::getShiftCommandNumber())
        tickWait++;

    if (Tickbase::pausedTicks())
    {
        Tickbase::pausedTicks() = 0;
        return;
    }

    original(thisPointer, currentCommand, entity->tickBase() * memory->globalVars->intervalPerTick, cmd, entity);
}

static void __cdecl randomSeedHook(int seed) noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)(int)>(hooks->randomSeed.getDetour());

    if (Crithack::protectData())
        return original(seed);

    const auto returnAddress = reinterpret_cast<unsigned long>(_ReturnAddress());
    if (returnAddress == memory->randomSeedReturnAddress1 ||
        returnAddress == memory->randomSeedReturnAddress2 ||
        returnAddress == memory->randomSeedReturnAddress3)
    {
        if (localPlayer)
        {
            const auto activeWeapon = localPlayer->getActiveWeapon();
            if (activeWeapon)
            {
                activeWeapon->currentSeed() = seed;
            }
        }
    }

    original(seed);
}

static int __fastcall sendDatagramHook(NetworkChannel* network, void* edx, bufferWrite* datagram) noexcept
{
    static auto original = hooks->sendDatagram.getOriginal<int>(datagram);
    if (!localPlayer || !config->backtrack.fakeLatency || !interfaces->engine->isInGame() || !interfaces->engine->isConnected()
        || !network || datagram)
        return original(network, datagram);

    const int inState = network->inReliableState;
    const int inSequenceNr = network->inSequenceNr;

    Backtrack::updateLatency(network);

    int result = original(network, datagram);

    network->inReliableState = inState;
    network->inSequenceNr = inSequenceNr;

    return result;
}

static int __fastcall tfPlayerInventoryGetMaxItemCountHook(void* thisPointer) noexcept
{
    static auto original = hooks->tfPlayerInventoryGetMaxItemCount.getOriginal<int>();
    if (config->misc.backpackExpander)
        return 3000;
    return original(thisPointer);
}

static void __fastcall updateTFAnimStateHook(void* thisPointer, void*, float eyeYaw, float eyePitch) noexcept
{
    static auto original = hooks->updateTFAnimState.getOriginal<void>(eyeYaw, eyePitch);

    const auto animState = reinterpret_cast<TFPlayerAnimState*>(thisPointer);
    if (!animState)
        return;

    const auto entity = reinterpret_cast<Entity*>(animState->player);
    if (!entity || !localPlayer || !entity->getModelPtr())
        return;

    if (entity == localPlayer.get())
        return original(thisPointer, Animations::getLocalViewangles().y, Animations::getLocalViewangles().x);

    return original(thisPointer, eyeYaw, eyePitch);
}

void resetAll(int resetType) noexcept
{
    Aimbot::reset();
    Animations::reset();
    Backtrack::reset();
    Crithack::reset();
    Misc::reset(resetType);
    EnginePrediction::reset();
    TargetSystem::reset();
    Tickbase::reset();
    Visuals::reset(resetType);

    resetUtil();

    simTimes.clear();
}

static void __fastcall levelShutDown(void* thisPointer) noexcept
{
    static auto original = hooks->client.getOriginal<void, 7>();

    original(thisPointer);
    resetAll(0);
}

Hooks::Hooks(HMODULE moduleHandle) noexcept
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    this->moduleHandle = moduleHandle;

    // interfaces and memory shouldn't be initialized in wndProc because they show MessageBox on error which would cause deadlock
    interfaces = std::make_unique<const Interfaces>();
    steamInterfaces = std::make_unique<const SteamInterfaces>();
    memory = std::make_unique<const Memory>();

    window = FindWindowW(L"Valve001", nullptr);
    originalWndProc = WNDPROC(SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(wndProc)));
}

void Hooks::install() noexcept
{
    originalPresent = **reinterpret_cast<decltype(originalPresent)**>(memory->present);
    **reinterpret_cast<decltype(present)***>(memory->present) = present;
    originalReset = **reinterpret_cast<decltype(originalReset)**>(memory->reset);
    **reinterpret_cast<decltype(reset)***>(memory->reset) = reset;

    MH_Initialize();

    addToCritBucket.detour(memory->addToCritBucket, addToCritBucketHook);
    calcIsAttackCritical.detour(memory->calcIsAttackCritical, calcIsAttackCriticalHook);
    calculateChargeCap.detour(memory->calculateChargeCap, calculateChargeCapHook);
    calcViewModelView.detour(memory->calcViewModelView, calcViewModelViewHook);
    canFireRandomCriticalShot.detour(memory->canFireRandomCriticalShot, canFireRandomCriticalShotHook);
    checkForSequenceChange.detour(memory->checkForSequenceChange, checkForSequenceChangeHook);
    clLoadWhitelist.detour(memory->clLoadWhitelist, clLoadWhitelistHook);
    clMove.detour(memory->clMove, clMoveHook);
    clSendMove.detour(memory->clSendMove, clSendMoveHook);
    doEnginePostProcessing.detour(memory->doEnginePostProcessing, doEnginePostProcessingHook);
    enableWorldFog.detour(memory->enableWorldFog, enableWorldFogHook);
    estimateAbsVelocity.detour(memory->estimateAbsVelocity, estimateAbsVelocityHook);
    //fireBullet.detour(memory->fireBullet, fireBulletHook);
    frameAdvance.detour(memory->frameAdvance, frameAdvanceHook);
    //getTraceType.detour(memory->getTraceType, getTraceTypeHook);
    interpolate.detour(memory->interpolate, interpolateHook);
    interpolateServerEntities.detour(memory->interpolateServerEntities, interpolateServerEntitiesHook);
    interpolateViewModel.detour(memory->interpolateViewModel, interpolateViewModelHook);
    isAllowedToWithdrawFromCritBucket.detour(memory->isAllowedToWithdrawFromCritBucket, isAllowedToWithdrawFromCritBucketHook);
    newMatchFoundDashboardStateOnUpdate.detour(memory->newMatchFoundDashboardStateOnUpdate, newMatchFoundDashboardStateOnUpdateHook);
    randomSeed.detour(reinterpret_cast<uintptr_t>(memory->randomSeed), randomSeedHook);
    runSimulation.detour(memory->runSimulation, runSimulationHook);
    sendDatagram.detour(memory->sendDatagram, sendDatagramHook);
    tfPlayerInventoryGetMaxItemCount.detour(memory->tfPlayerInventoryGetMaxItemCount, tfPlayerInventoryGetMaxItemCountHook);
    updateTFAnimState.detour(memory->updateTFAnimState, updateTFAnimStateHook);

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(35, frameStageNotify);
    client.hookAt(36, dispatchUserMessage);

    clientMode.init(memory->clientMode);
    clientMode.hookAt(16, overrideView);
    clientMode.hookAt(21, createMove);
    clientMode.hookAt(39, doPostScreenEffects);

    eventManager.init(interfaces->gameEventManager);
    eventManager.hookAt(8, fireEventClientSide);

    input.init(memory->input);
    input.hookAt(8, getUserCmd);

    modelRender.init(interfaces->modelRender);
    modelRender.hookAt(19, drawModelExecute);

    panel.init(interfaces->panel);
    panel.hookAt(41, paintTraverse);

    prediction.init(interfaces->prediction);

    surface.init(interfaces->surface);
    surface.hookAt(62, lockCursor);

    MH_EnableHook(MH_ALL_HOOKS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
    Sleep(100);

    if (vguiFocusOverlayPanel != NULL)
        interfaces->panel->setMouseInputEnabled(vguiFocusOverlayPanel, false);
    interfaces->surface->unlockCursor();
    interfaces->inputSystem->enableInput(true);

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);
    
    FreeLibraryAndExitThread(moduleHandle, 0);
}

void Hooks::uninstall() noexcept
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_RemoveHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    resetAll(1);

    Netvars::restore();

    SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(memory->present) = originalPresent;
    **reinterpret_cast<void***>(memory->reset) = originalReset;

    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), moduleHandle, 0, nullptr))
        CloseHandle(thread);
}