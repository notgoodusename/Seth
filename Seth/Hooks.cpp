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
#include "EventListener.h"
#include "GameData.h"
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Logger.h"
#include "Memory.h"

#include "Hacks/Animations.h"
#include "Hacks/AntiAim.h"
#include "Hacks/Backtrack.h"
#include "Hacks/Chams.h"
#include "Hacks/EnginePrediction.h"
#include "Hacks/Fakelag.h"
#include "Hacks/StreamProofESP.h"
#include "Hacks/Glow.h"
#include "Hacks/GrenadePrediction.h"
#include "Hacks/Knifebot.h"
#include "Hacks/Legitbot.h"
#include "Hacks/Misc.h"
#include "Hacks/Ragebot.h"
#include "Hacks/Resolver.h"
#include "Hacks/Sound.h"
#include "Hacks/Tickbase.h"
#include "Hacks/Triggerbot.h"
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
#include "SDK/GameMovement.h"
#include "SDK/GameUI.h"
#include "SDK/GlobalVars.h"
#include "SDK/Input.h"
#include "SDK/InputSystem.h"
#include "SDK/MaterialSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/NetworkChannel.h"
#include "SDK/NetworkMessage.h"
#include "SDK/Panel.h"
#include "SDK/Platform.h"
#include "SDK/Prediction.h"
#include "SDK/PredictionCopy.h"
#include "SDK/RenderContext.h"
#include "SDK/SoundInfo.h"
#include "SDK/SoundEmitter.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"
#include "SDK/ViewSetup.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    [[maybe_unused]] static const auto once = [](HWND window) noexcept {
        Netvars::init();
        Misc::initHiddenCvars();
        eventListener = std::make_unique<EventListener>();

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
        GrenadePrediction::draw();
        Misc::purchaseList();
        Visuals::visualizeSpread(ImGui::GetBackgroundDrawList());
        Visuals::drawAimbotFov(ImGui::GetBackgroundDrawList());
        Misc::noscopeCrosshair(ImGui::GetBackgroundDrawList());
        Misc::recoilCrosshair(ImGui::GetBackgroundDrawList());
        Misc::drawOffscreenEnemies(ImGui::GetBackgroundDrawList());
        Misc::drawBombTimer();
        Misc::hurtIndicator();
        Misc::spectatorList();
        Misc::showKeybinds();
        Visuals::hitMarker(nullptr, ImGui::GetBackgroundDrawList());
        Visuals::drawMolotovHull(ImGui::GetBackgroundDrawList());
        Visuals::drawMolotovPolygon(ImGui::GetBackgroundDrawList());
        Visuals::drawSmokeHull(ImGui::GetBackgroundDrawList());
        Visuals::drawSmokeTimer(ImGui::GetBackgroundDrawList());
        Visuals::drawMolotovTimer(ImGui::GetBackgroundDrawList());
        Misc::watermark();
        Misc::drawAutoPeek(ImGui::GetBackgroundDrawList());
        Logger::process(ImGui::GetBackgroundDrawList());
        Misc::drawKeyDisplay(ImGui::GetBackgroundDrawList());
        Misc::drawVelocity(ImGui::GetBackgroundDrawList());
        Legitbot::updateInput();
        Visuals::updateInput();
        StreamProofESP::updateInput();
        Misc::updateInput();
        Triggerbot::updateInput();
        Chams::updateInput();
        Glow::updateInput();
        Ragebot::updateInput();
        AntiAim::updateInput();
        Tickbase::updateInput();
        Misc::drawPlayerList();
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

static bool __stdcall createMove(float inputSampleTime, UserCmd* cmd, bool& sendPacket) noexcept
{
    static auto previousViewAngles{ cmd->viewangles };
    const auto viewAngles{ cmd->viewangles };
    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };
    auto angOldViewPoint{ cmd->viewangles };
    const auto currentPredictedTick{ interfaces->prediction->split->commandsPredicted - 1 };

    memory->globalVars->serverTime(cmd);
    Misc::bunnyHop(cmd);

    EnginePrediction::update();
    EnginePrediction::run(cmd);

    auto viewAnglesDelta{ cmd->viewangles - previousViewAngles };
    viewAnglesDelta.normalize();
    viewAnglesDelta.x = std::clamp(viewAnglesDelta.x, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);
    viewAnglesDelta.y = std::clamp(viewAnglesDelta.y, -config->misc.maxAngleDelta, config->misc.maxAngleDelta);

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

static void __stdcall CHLCreateMove(int sequenceNumber, float inputSampleTime, bool active, bool& sendPacket) noexcept
{
    static auto original = hooks->client.getOriginal<void, 22>(sequenceNumber, inputSampleTime, active);

    original(interfaces->client, sequenceNumber, inputSampleTime, active);

    UserCmd* cmd = memory->input->getUserCmd(0, sequenceNumber);
    if (!cmd || !cmd->commandNumber)
        return;

    VerifiedUserCmd* verified = memory->input->getVerifiedUserCmd(sequenceNumber);
    if (!verified)
        return;

    bool cmoveactive = createMove(inputSampleTime, cmd, sendPacket);

    verified->cmd = *cmd;
    verified->crc = cmd->getChecksum();
}

#pragma warning(disable : 4409)
__declspec(naked) void __stdcall createMoveProxy(int sequenceNumber, float inputSampleTime, bool active)
{
    __asm
    {
        PUSH	EBP
        MOV		EBP, ESP
        PUSH	EBX
        LEA		ECX, [ESP]
        PUSH	ECX
        PUSH	active
        PUSH	inputSampleTime
        PUSH	sequenceNumber
        CALL	CHLCreateMove
        POP		EBX
        POP		EBP
        RETN	0xC
    }
}

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
    if (stage == FrameStage::START)
        GameData::update();

    hooks->client.callOriginal<void, 37>(stage);
}

static void __stdcall lockCursor() noexcept
{
    if (gui->isOpen())
        return interfaces->surface->unlockCursor();
    return hooks->surface.callOriginal<void, 67>();
}

void resetAll(int resetType) noexcept
{

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
    dynamicClassId = std::make_unique<const ClassIdManager>();

    window = FindWindowW(L"Valve001", nullptr);
    originalWndProc = WNDPROC(SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(wndProc)));
}

void Hooks::install() noexcept
{
    originalPresent = **reinterpret_cast<decltype(originalPresent)**>(memory->present);
    **reinterpret_cast<decltype(present)***>(memory->present) = present;
    originalReset = **reinterpret_cast<decltype(originalReset)**>(memory->reset);
    **reinterpret_cast<decltype(reset)***>(memory->reset) = reset;

    if constexpr (std::is_same_v<HookType, MinHook>)
        MH_Initialize();

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(22, createMoveProxy);
    client.hookAt(37, frameStageNotify);

    surface.init(interfaces->surface);
    surface.hookAt(67, lockCursor);
    if constexpr (std::is_same_v<HookType, MinHook>)
        MH_EnableHook(MH_ALL_HOOKS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
    Sleep(100);

    interfaces->inputSystem->enableInput(true);
    eventListener->remove();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    _CRT_INIT(moduleHandle, DLL_PROCESS_DETACH, nullptr);

    FreeLibraryAndExitThread(moduleHandle, 0);
}

void Hooks::uninstall() noexcept
{
    Misc::updateEventListeners(true);
    Visuals::updateEventListeners(true);
    Resolver::updateEventListeners(true);

    if constexpr (std::is_same_v<HookType, MinHook>) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    client.restore();
    engine.restore();
    surface.restore();

    Netvars::restore();

    Glow::clearCustomObjects();
    resetAll(1);

    SetWindowLongPtrW(window, GWLP_WNDPROC, LONG_PTR(originalWndProc));
    **reinterpret_cast<void***>(memory->present) = originalPresent;
    **reinterpret_cast<void***>(memory->reset) = originalReset;

    if (HANDLE thread = CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(unload), moduleHandle, 0, nullptr))
        CloseHandle(thread);
}
