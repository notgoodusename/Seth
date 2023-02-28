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

#include "Hacks/Aimbot/Aimbot.h"
#include "Hacks/Animations.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/Misc.h"
#include "Hacks/StreamProofESP.h"
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

        Visuals::updateInput();
        StreamProofESP::updateInput();
        Misc::updateInput();
        Chams::updateInput();

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

    return hooks->originalPresent(device, src, dest, windowOverride, dirtyRegion);
}

static HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    return hooks->originalReset(device, params);
}

static int __fastcall sendDatagramHook(NetworkChannel* network, void*, void* datagram) noexcept
{
    static auto original = hooks->sendDatagram.getOriginal<int>(datagram);
    if (!config->backtrack.fakeLatency || !interfaces->engine->isInGame() || datagram)
        return original(network, datagram);

    const int instate = network->inReliableState;
    const int insequencenr = network->inSequenceNr;

    const float delta = max(0.f, (config->backtrack.fakeLatencyAmount / 1000.f));

    Backtrack::addLatencyToNetwork(network, delta);

    int result = original(network, datagram);

    network->inReliableState = instate;
    network->inSequenceNr = insequencenr;

    return result;
}

static bool __fastcall createMove(void* thisPointer, void*, float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 21>(inputSampleTime, cmd);

    if (!cmd || !cmd->commandNumber)
        return result;

    static auto previousViewAngles{ cmd->viewangles };
    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };

    memory->globalVars->serverTime(cmd);
    Misc::bunnyHop(cmd);
    Misc::autoStrafe(cmd, currentViewAngles);

    Backtrack::updateIncomingSequences();

    EnginePrediction::update();
    EnginePrediction::run(cmd);

    Backtrack::run(cmd);
    Aimbot::run(cmd);

    auto viewAnglesDelta{ cmd->viewangles - previousViewAngles };
    viewAnglesDelta.normalize();
    viewAnglesDelta.x = std::clamp(viewAnglesDelta.x, -255.0f, 255.0f);
    viewAnglesDelta.y = std::clamp(viewAnglesDelta.y, -255.0f, 255.0f);

    cmd->viewangles = previousViewAngles + viewAnglesDelta;

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

    previousViewAngles = cmd->viewangles;
    return false;
}

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
    static auto backtrackInit = (Backtrack::init(), false);

    if (stage == FrameStage::START)
        GameData::update();

    if (stage == FrameStage::RENDER_START) {
        Misc::unlockHiddenCvars();
    }
    if (interfaces->engine->isInGame()) {
        Animations::handlePlayers(stage);
    }

    hooks->client.callOriginal<void, 35>(stage);
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
        const char* szName = interfaces->panel->getName(vguiPanel);
        if (szName[0] == 'F' && szName[5] == 'O' && szName[12] == 'P')
        {
            vguiFocusOverlayPanel = vguiPanel;
        }
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

void resetAll(int resetType) noexcept
{
    Aimbot::reset();
    Animations::reset();
    Misc::reset(resetType);
    EnginePrediction::reset();
    Visuals::reset(resetType);
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

    calcViewModelView.detour(memory->calcViewModelView, calcViewModelViewHook);
    estimateAbsVelocity.detour(memory->estimateAbsVelocity, estimateAbsVelocityHook);
    sendDatagram.detour(memory->sendDatagram, sendDatagramHook);

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(35, frameStageNotify);

    clientMode.init(memory->clientMode);
    clientMode.hookAt(16, overrideView);
    clientMode.hookAt(21, createMove);

    modelRender.init(interfaces->modelRender);
    modelRender.hookAt(19, drawModelExecute);

    panel.init(interfaces->panel);
    panel.hookAt(41, paintTraverse);

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
    MH_Uninitialize();

    Netvars::restore();

    resetAll(1);

    SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(memory->present) = originalPresent;
    **reinterpret_cast<void***>(memory->reset) = originalReset;

    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), moduleHandle, 0, nullptr))
        CloseHandle(thread);
}
