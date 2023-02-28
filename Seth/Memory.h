#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <string>

#include "SDK/Platform.h"

class ClientMode;
class ClientState;
class Entity;
class GameEventDescriptor;
class GameEventManager;
class Input;
class ItemSystem;
class KeyValues;
class MemAlloc;
class MoveHelper;
class PlantedC4;
class PlayerResource;
class ViewRender;
class ViewRenderBeams;
class WeaponSystem;
template <typename Key, typename Value>
struct UtlMap;
template <typename T>
class UtlVector;

struct ActiveChannels;
struct Channel;
struct CStudioHdr;
struct GlobalVars;
struct GlowObjectManager;
struct PanoramaEventRegistration;
struct Trace;
struct Vector;

class Memory {
public:
    Memory() noexcept;

    std::uintptr_t present;
    std::uintptr_t reset;

    ClientMode* clientMode;
    ClientState* clientState;
    GlobalVars* globalVars;
    Input* input;
    MoveHelper* moveHelper;

    KeyValues* (__thiscall* keyValuesInitialize)(KeyValues*, char*);
    KeyValues* (__thiscall* keyValuesFindKey)(KeyValues* keyValues, const char* keyName, bool create);

    void(__thiscall* calcAbsoluteVelocity)(void*);
    int(__thiscall* getNextThinkTick)(void*, const char*);
    void(__thiscall* setAbsOrigin)(Entity*, const Vector&);
    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    void(__thiscall* setCollisionBounds)(void*, const Vector& mins, const Vector& maxs);
    void(__thiscall* setNextThink)(void*, float, const char*);

    std::add_pointer_t<void _cdecl(const char* msg, ...)> logDirect;
    bool(__thiscall* physicsRunThink)(void*, int);

    int* predictionRandomSeed;

    std::uintptr_t calcViewModelView;
    std::uintptr_t estimateAbsVelocity;
    std::uintptr_t sendDatagram;
private:
};

inline std::unique_ptr<const Memory> memory;
