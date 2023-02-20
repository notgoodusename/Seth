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
#include "GUI.h"
#include "Hooks.h"
#include "Interfaces.h"
#include "Memory.h"

#include "Hacks/Misc.h"

#include "SDK/ClassId.h"
#include "SDK/Client.h"
#include "SDK/ClientState.h"
#include "SDK/ConVar.h"
#include "SDK/Engine.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/FrameStage.h"
#include "SDK/GlobalVars.h"
#include "SDK/InputSystem.h"
#include "SDK/ModelRender.h"
#include "SDK/Platform.h"
#include "SDK/Prediction.h"
#include "SDK/StudioRender.h"
#include "SDK/Surface.h"
#include "SDK/UserCmd.h"

static LRESULT __stdcall wndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
    [[maybe_unused]] static const auto once = [](HWND window) noexcept {
        Netvars::init();

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

static bool __fastcall createMove(void* ecx, void* edx, float inputSampleTime, UserCmd* cmd) noexcept
{
    auto result = hooks->clientMode.callOriginal<bool, 21>(inputSampleTime, cmd);

    if (!cmd || !cmd->commandNumber)
        return result;

    static auto previousViewAngles{ cmd->viewangles };
    auto currentViewAngles{ cmd->viewangles };
    const auto currentCmd{ *cmd };

    memory->globalVars->serverTime(cmd);
    Misc::bunnyHop(cmd);

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

static void __stdcall frameStageNotify(FrameStage stage) noexcept
{
    hooks->client.callOriginal<void, 35>(stage);
}

static void __stdcall lockCursor() noexcept
{
    if (gui->isOpen())
        return interfaces->surface->unlockCursor();
    return hooks->surface.callOriginal<void, 62>();
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
    //dynamicClassId = std::make_unique<const ClassIdManager>();

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

    client.init(interfaces->client);
    client.hookAt(7, levelShutDown);
    client.hookAt(35, frameStageNotify);

    clientMode.init(memory->clientMode);
    clientMode.hookAt(21, createMove);

    surface.init(interfaces->surface);
    surface.hookAt(62, lockCursor);

    MH_EnableHook(MH_ALL_HOOKS);
}

extern "C" BOOL WINAPI _CRT_INIT(HMODULE moduleHandle, DWORD reason, LPVOID reserved);

static DWORD WINAPI unload(HMODULE moduleHandle) noexcept
{
    Sleep(100);

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
