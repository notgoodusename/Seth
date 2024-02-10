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
class TFPlayerResource;
class ViewRenderBeams;
class WeaponSystem;
template <typename Key, typename Value>
struct UtlMap;
template <typename T>
class UtlVector;
class matrix3x4;

struct ActiveChannels;
struct Channel;
struct CStudioHdr;
struct Frustum;
struct GlobalVars;
struct GlowObjectManager;
struct PanoramaEventRegistration;
struct Quaternion;
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

    void(__thiscall* boneSetupInitPose)(void*, Vector*, Quaternion*);
    void(__thiscall* boneSetupAccumulatePose)(void*, Vector*, Quaternion*, int, float, float, float, void*);
    void(__thiscall* boneSetupCalcAutoplaySequences)(void*, Vector*, Quaternion*, float, void*);
    void(__thiscall* boneSetupCalcBoneAdj)(void*, Vector*, Quaternion*, const float*);
    
    void(__thiscall* calcAbsoluteVelocity)(void*);
    bool(_cdecl* cullBox)(const Vector&, const Vector&, const Frustum&);
    int(__thiscall* getAmmoCount)(void*, int);
    EconItemDefinition*( __thiscall* getItemDefinition)(void*, int);
    int(__thiscall* getNextThinkTick)(void*, const char*);
    void*(_cdecl* getWeaponData)(int);
    void(_cdecl* generatePerspectiveFrustum)(const Vector&, const Vector&, const Vector&, const Vector&, float zNear, float zFar, float fovX, float fovY, Frustum&);

    void(__thiscall* IKContextConstruct)(void*);
    void(__thiscall* IKContextDeconstruct)(void*);
    void(__thiscall* IKContextInit)(void*, const CStudioHdr*, const Vector&, const Vector&, float, int, int);
    void(__thiscall* IKContextUpdateTargets)(void*, Vector*, Quaternion*, matrix3x4*, void*);
    void(__thiscall* IKContextSolveDependencies)(void*, Vector*, Quaternion*, matrix3x4*, void*);

    bool(_cdecl* passServerEntityFilter)(void* touch, void* pass);
    bool(__thiscall* physicsRunThink)(void*, int);
    std::add_pointer_t<int __cdecl(const int, ...)> randomSeed;
    std::add_pointer_t<int __cdecl(const int, const int, ...)> randomInt;
    std::add_pointer_t<float __cdecl(const float, const float, ...)> randomFloat;
    void(__thiscall* setAbsOrigin)(Entity*, const Vector&);
    void(__thiscall* setAbsAngle)(Entity*, const Vector&);
    void(__thiscall* setCollisionBounds)(void*, const Vector& preScaledMins, const Vector& preScaledMaxs);
    void(__thiscall* setNextThink)(void*, float, const char*);
    bool(__stdcall* shouldCollide)(int, int);
    void(__thiscall* simulatePlayerSimulatedEntities)(void*);
    void*(__thiscall* seqdesc)(void*, int);
    bool(_cdecl* standardFilterRules)(void*, int);

    int* predictionRandomSeed;

    std::uintptr_t addToCritBucket;
    std::uintptr_t calcIsAttackCritical;
    std::uintptr_t calculateChargeCap;
    std::uintptr_t calcViewModelView;
    std::uintptr_t canFireRandomCriticalShot;
    std::uintptr_t checkForSequenceChange;
    std::uintptr_t clLoadWhitelist;
    std::uintptr_t clMove;
    std::uintptr_t clSendMove;
    std::uintptr_t customTextureOnItemProxyOnBindInternal;
    std::uintptr_t doEnginePostProcessing;
    std::uintptr_t estimateAbsVelocity;
    std::uintptr_t enableWorldFog;
    std::uintptr_t fireBullet;
    std::uintptr_t frameAdvance;
    std::uintptr_t getTraceType;
    std::uintptr_t interpolateServerEntities;
    std::uintptr_t isAllowedToWithdrawFromCritBucket;
    std::uintptr_t newMatchFoundDashboardStateOnUpdate;
    std::uintptr_t physicsSimulate;
    std::uintptr_t tfPlayerInventoryGetMaxItemCount;
    std::uintptr_t randomSeedReturnAddress1;
    std::uintptr_t randomSeedReturnAddress2;
    std::uintptr_t randomSeedReturnAddress3;
    std::uintptr_t sendDatagram;
    std::uintptr_t updateClientSideAnimation;
    std::uintptr_t updateTFAnimState;

    std::add_pointer_t<void _cdecl(const char* msg, ...)> logDirect;
private:
};

inline std::unique_ptr<const Memory> memory;
