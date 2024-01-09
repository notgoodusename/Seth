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
#include "StrayElements.h"
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
#include "Hacks/SkinChanger.h"
#include "Hacks/StreamProofESP.h"
#include "Hacks/TargetSystem.h"
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

        Misc::drawPlayerList();
        Crithack::draw();

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

static bool __fastcall createMove(void* thisPointer, void*, float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 21>(inputSampleTime, cmd);

    Backtrack::update();

    if (!cmd || !cmd->commandNumber)
        return result;

    uintptr_t framePointer;
    __asm mov framePointer, ebp;
    bool& sendPacket = *reinterpret_cast<bool*>(***reinterpret_cast<uintptr_t***>(framePointer) - 0x1);

    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };

    Misc::runFreeCam(cmd, currentViewAngles);

    memory->globalVars->serverTime(cmd);
    Misc::antiAfkKick(cmd);
    Misc::fastStop(cmd);
    Misc::bunnyHop(cmd);
    Misc::autoStrafe(cmd, currentViewAngles);

    EnginePrediction::update();
    EnginePrediction::run(cmd);

    TargetSystem::updateTick(cmd);

    Backtrack::run(cmd);
    Aimbot::run(cmd);
    Triggerbot::run(cmd);

    Misc::edgejump(cmd);

    Fakelag::run(sendPacket);

    //Crithack::run(cmd);

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
        TargetSystem::updateFrame();

    if (stage == FrameStage::RENDER_START) {
        Misc::unlockHiddenCvars();
    }
    
    original(thisPointer, stage); //render start crash
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

static void __fastcall calcViewModelViewHook(void* thisPointer, void*, Entity* owner, Vector* eyePosition, Vector* eyeAngles) noexcept
{
    static auto original = hooks->calcViewModelView.getOriginal<void>(owner, eyePosition, eyeAngles);

    auto eyePositionCopy = eyePosition;
    auto eyeAnglesCopy = eyeAngles;

    Misc::viewModelChanger(*eyePositionCopy, *eyeAnglesCopy);

    original(thisPointer, owner, eyePositionCopy, eyeAnglesCopy);
}

static unsigned int vguiFocusOverlayPanel;

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

static void* __cdecl clLoadWhitelistHook(void* whitelist, const char* name) noexcept
{
    static auto original = reinterpret_cast<void*(__cdecl*)(void*, const char*)>(hooks->enableWorldFog.getDetour());
    if(config->misc.svPureBypass)
        return NULL;
    return original(whitelist, name);
}

static void __fastcall estimateAbsVelocityHook(void* thisPointer, void*, Vector& vel) noexcept
{
    static auto original = hooks->estimateAbsVelocity.getOriginal<void>(&vel);

    const auto entity = reinterpret_cast<Entity*>(thisPointer);
    if (localPlayer && entity->isPlayer() && entity != localPlayer.get())
    {
        entity->getAbsVelocity(vel);
        return;
    }

    original(thisPointer, &vel);
}

static void __cdecl enableWorldFogHook() noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)()>(hooks->enableWorldFog.getDetour());
    if (config->visuals.noFog)
        return;
    return original();
}

static int __fastcall tfPlayerInventoryGetMaxItemCountHook(void* thisPointer) noexcept
{
    static auto original = hooks->tfPlayerInventoryGetMaxItemCount.getOriginal<int>();
    if (config->misc.backpackExpander)
        return 3000;
    return original(thisPointer);
}

static void __fastcall addToCritBucketHook(void* thisPointer, void*, const float amount) noexcept
{
    static auto original = hooks->addToCritBucket.getOriginal<void>(amount);
    if (Crithack::protectData())
        return;

    original(thisPointer, amount);
}

static bool __fastcall isAllowedToWithdrawFromCritBucketHook(void* thisPointer, void*, float damage) noexcept
{
    static auto original = hooks->isAllowedToWithdrawFromCritBucket.getOriginal<bool>(damage);
    if (Crithack::protectData())
        return true;

    return original(thisPointer, damage);
}

static float __fastcall calculateChargeCapHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->calculateChargeCap.getOriginal<float>();

    const float backupFrametime = memory->globalVars->frametime;
    memory->globalVars->frametime = FLT_MAX;
    float finalValue = original(thisPointer);
    memory->globalVars->frametime = backupFrametime;
    return finalValue;
}

static void __cdecl interpolateServerEntitiesHook() noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)()>(hooks->interpolateServerEntities.getDetour());
    
    static auto extrapolate = interfaces->cvar->findVar("cl_extrapolate");
    extrapolate->setValue(0);
    original();
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

static void __fastcall customTextureOnItemProxyOnBindInternalHook(void* thisPointer, void*, void* scriptItem) noexcept
{
    static auto original = hooks->customTextureOnItemProxyOnBindInternal.getOriginal<void>(scriptItem);
    if (config->visuals.disableCustomDecals)
        return;

    original(thisPointer, scriptItem);
}

static void __fastcall newMatchFoundDashboardStateOnUpdateHook(void* thisPointer, void*) noexcept
{
    static auto original = hooks->newMatchFoundDashboardStateOnUpdate.getOriginal<void>();

    auto autoJoinTime = reinterpret_cast<double*>(reinterpret_cast<uintptr_t>(thisPointer) + 424);

    if (config->misc.autoAccept)
        *autoJoinTime = -10.0;

    original(thisPointer);
}

static void __cdecl doEnginePostProcessingHook(int x, int y, int w, int h, bool flashlightIsOn, bool postVGui) noexcept
{
    static auto original = reinterpret_cast<void(__cdecl*)(int, int, int, int, bool, bool)>(hooks->doEnginePostProcessing.getDetour());
    if (config->visuals.disablePostProcessing)
        return;

    original(x, y, w, h, flashlightIsOn, postVGui);
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
    StrayElements::clear();
    Visuals::reset(resetType);

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
    calculateChargeCap.detour(memory->calculateChargeCap, calculateChargeCapHook);
    calcViewModelView.detour(memory->calcViewModelView, calcViewModelViewHook);
    clLoadWhitelist.detour(memory->clLoadWhitelist, clLoadWhitelistHook);
    customTextureOnItemProxyOnBindInternal.detour(memory->customTextureOnItemProxyOnBindInternal, customTextureOnItemProxyOnBindInternalHook);
    doEnginePostProcessing.detour(memory->doEnginePostProcessing, doEnginePostProcessingHook);
    estimateAbsVelocity.detour(memory->estimateAbsVelocity, estimateAbsVelocityHook);
    enableWorldFog.detour(memory->enableWorldFog, enableWorldFogHook);
    frameAdvance.detour(memory->frameAdvance, frameAdvanceHook);
    interpolateServerEntities.detour(memory->interpolateServerEntities, interpolateServerEntitiesHook);
    isAllowedToWithdrawFromCritBucket.detour(memory->isAllowedToWithdrawFromCritBucket, isAllowedToWithdrawFromCritBucketHook);
    newMatchFoundDashboardStateOnUpdate.detour(memory->newMatchFoundDashboardStateOnUpdate, newMatchFoundDashboardStateOnUpdateHook);
    tfPlayerInventoryGetMaxItemCount.detour(memory->tfPlayerInventoryGetMaxItemCount, tfPlayerInventoryGetMaxItemCountHook);
    updateTFAnimState.detour(memory->updateTFAnimState, updateTFAnimStateHook);
    sendDatagram.detour(memory->sendDatagram, sendDatagramHook);

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(35, frameStageNotify);
    client.hookAt(36, dispatchUserMessage);

    clientMode.init(memory->clientMode);
    clientMode.hookAt(16, overrideView);
    clientMode.hookAt(21, createMove);
    clientMode.hookAt(39, doPostScreenEffects);

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

    //IDK why, dont ask me why, but release crashes because of engineprediction on unhook
    //Debug doesnt tho, so this check is needed to unhook on release, also unhooking on release will lag your game
    //prob some exception being skipped lol
#ifdef _DEBUG
    _CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);
#endif
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