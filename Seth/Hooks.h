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
    MinHook calcIsAttackCritical;
    MinHook calculateChargeCap;
    MinHook calcViewModelView;
    MinHook canFireRandomCriticalShot;
    MinHook checkForSequenceChange;
    MinHook clLoadWhitelist;
    MinHook clMove;
    MinHook clSendMove;
    MinHook customTextureOnItemProxyOnBindInternal;
    MinHook doEnginePostProcessing;
    MinHook enableWorldFog;
    MinHook estimateAbsVelocity;
    MinHook fireBullet;
    MinHook frameAdvance;
    MinHook getTraceType;
    MinHook interpolateServerEntities;
    MinHook isAllowedToWithdrawFromCritBucket;
    MinHook newMatchFoundDashboardStateOnUpdate;
    MinHook randomSeed;
    MinHook runSimulation;
    MinHook sendDatagram;
    MinHook tfPlayerInventoryGetMaxItemCount;
    MinHook updateClientSideAnimation;
    MinHook updateTFAnimState;

    MinHook client;
    MinHook clientMode;
    MinHook engine;
    MinHook eventManager;
    MinHook input;
    MinHook modelRender;
    MinHook panel;
    MinHook prediction;
    MinHook surface;
private:
    HMODULE moduleHandle;
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
