#pragma once

#include <memory>
#include <type_traits>
#include <d3d9.h>
#include <Windows.h>

#include "Hooks/MinHook.h"
#include "Hooks/VmtHook.h"
#include "Hooks/VmtSwap.h"

#include "SDK/Platform.h"

struct SoundInfo;

using HookType = MinHook;

class Hooks {
public:
    Hooks(HMODULE moduleHandle) noexcept;

    WNDPROC originalWndProc;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;

    void install() noexcept;
    void uninstall() noexcept;

    HookType client;
    HookType engine;
    HookType surface;
private:
    HMODULE moduleHandle;
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
