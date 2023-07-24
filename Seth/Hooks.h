#pragma once

#include <memory>
#include <type_traits>
#include <d3d9.h>
#include <Windows.h>

#include "Hooks/MinHook.h"

#include "SDK/Platform.h"

struct SoundInfo;

class Hooks {
public:
    Hooks(HMODULE moduleHandle) noexcept;

    WNDPROC originalWndProc;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;

    void install() noexcept;
    void uninstall() noexcept;

    MinHook addToCritBucket;
    MinHook calculateChargeCap;
    MinHook calcViewModelView;
    MinHook clLoadWhitelist;
    MinHook customTextureOnItemProxyOnBindInternal;
    MinHook estimateAbsVelocity;
    MinHook enableWorldFog;
    MinHook frameAdvance;
    MinHook isAllowedToWithdrawFromCritBucket;
    MinHook interpolateServerEntities;
    MinHook newMatchFoundDashboardStateOnUpdate;
    MinHook tfPlayerInventoryGetMaxItemCount;
    MinHook sendDatagram;
    MinHook updateClientSideAnimation;
    MinHook updateTFAnimState;

    MinHook client;
    MinHook clientMode;
    MinHook engine;
    MinHook modelRender;
    MinHook panel;
    MinHook prediction;
    MinHook surface;
private:
    HMODULE moduleHandle;
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
