#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <string>

#include "SDK/Platform.h"

class ClientMode;
class ClientState;
class EconItemDefinition;
class Entity;
class GameEventDescriptor;
class GameEventManager;
class GameRules;
class Input;
class ItemSchema;
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
    GameRules* gameRules;
    GlobalVars* globalVars;
    Input* input;
    std::add_pointer_t<ItemSchema* __cdecl()> itemSchema;
    MoveHelper* moveHelper;
    MemAlloc* memAlloc;

    KeyValues* (__thiscall* keyValuesInitialize)(KeyValues*, char*);
    KeyValues* (__thiscall* keyValuesFindKey)(KeyValues* keyValues, const char* keyName, bool create);

    
    float(__cdecl* attributeHookValue)(float, const char*, void*, void*, bool);
    void(__thiscall* calcAbsoluteVelocity)(void*);
    bool(__thiscall* calcIsAttackCriticalHelper)(void*);
    bool(__thiscall* calcIsAttackCriticalHelperMelee)(void*);
    EconItemDefinition*( __thiscall* getItemDefinition)(void*, int);
    int(__thiscall* getNextThinkTick)(void*, const char*);
    bool(_cdecl* passServerEntityFilter)(void* touch, void* pass);
    bool(__thiscall* physicsRunThink)(void*, int);
    std::add_pointer_t<int __cdecl(const int, ...)> randomSeed;
    std::add_pointer_t<int __cdecl(const int, const int, ...)> randomInt;
    std::add_pointer_t<float __cdecl(const float, const float, ...)> randomFloat;
    void(__thiscall* setAbsOrigin)(Entity*, const Vector&);
    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    void(__thiscall* setCollisionBounds)(void*, const Vector& mins, const Vector& maxs);
    void(__thiscall* setNextThink)(void*, float, const char*);
    bool(__stdcall* shouldCollide)(int, int);
    void(__thiscall* simulatePlayerSimulatedEntities)(void*);
    bool(_cdecl* standardFilterRules)(void*, int);

    int* predictionRandomSeed;

    std::uintptr_t addToCritBucket;
    std::uintptr_t calculateChargeCap;
    std::uintptr_t calcViewModelView;
    std::uintptr_t clLoadWhitelist;
    std::uintptr_t estimateAbsVelocity;
    std::uintptr_t enableWorldFog;
    std::uintptr_t frameAdvance;
    std::uintptr_t interpolateServerEntities;
    std::uintptr_t isAllowedToWithdrawFromCritBucket;
    std::uintptr_t tfPlayerInventoryGetMaxItemCount;
    std::uintptr_t sendDatagram;
    std::uintptr_t updateClientSideAnimation;
    std::uintptr_t updateTFAnimState;

    std::add_pointer_t<void _cdecl(const char* msg, ...)> logDirect;
private:
};

inline std::unique_ptr<const Memory> memory;
