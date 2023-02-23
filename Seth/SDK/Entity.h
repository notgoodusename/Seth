#pragma once

#include <functional>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"

#include "ClientClass.h"
#include "CommandContext.h"
#include "Conditions.h"
#include "Cvar.h"
#include "Datamap.h"
#include "Engine.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "matrix3x4.h"
#include "MDLCache.h"
#include "ModelInfo.h"
#include "ModelRender.h"
#include "Utils.h"
#include "UtlVector.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponId.h"

struct AnimState;

struct AnimationLayer
{
public:
    float animationTime; //0
    float fadeOut; //4
    CStudioHdr* dispatchedStudioHdr; //8
    int dispatchedSrc; //12
    int dispatchedDst; //16
    unsigned int order; //20, networked
    unsigned int sequence; //24, networked
    float prevCycle; //28, networked
    float weight; //32, networked
    float weightDeltaRate; //36, networked
    float playbackRate; //40, networked
    float cycle; //44, networked
    void* owner; //48
    int invalidatePhysicsBits; //52

    void reset()
    {
        sequence = 0;
        weight = 0;
        weightDeltaRate = 0;
        playbackRate = 0;
        prevCycle = 0;
        cycle = 0;
    }
};

enum class MoveType {
    NONE = 0,
    ISOMETRIC,
    WALK,
    STEP,
    FLY,
    FLYGRAVITY,
    vPHYSICS,
    PUSH,
    NOCLIP,
    LADDER,
    OBSERVER,
    CUSTOM,

    LAST = CUSTOM,

    MAX_BITS = 4
};

enum class ObsMode {
    None = 0,
    Deathcam,
    Freezecam,
    Fixed,
    InEye,
    Chase,
    Roaming
};

enum class Team {
    None = 0,
    Spectators,
    BLU,
    RED
};

enum class TFClass {
    NONE = 0,
    SCOUT,
    SNIPER,
    SOLDIER,
    DEMOMAN,
    MEDIC,
    HEAVY,
    PYRO,
    SPY,
    ENGINEER
};

class Collideable {
public:
    VIRTUAL_METHOD(const Vector&, obbMins, 3, (), (this))
    VIRTUAL_METHOD(const Vector&, obbMaxs, 4, (), (this))
};

class Entity {
public:
    VIRTUAL_METHOD(void, release, 1, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(ClientClass*, getClientClass, 2, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(bool, isDormant, 8, (), (this + sizeof(uintptr_t) * 2))
    VIRTUAL_METHOD(int, index, 9, (), (this + sizeof(uintptr_t) * 2))

    VIRTUAL_METHOD(Vector&, getRenderOrigin, 1, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(Vector&, getRenderAngles, 2, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(bool, shouldDraw, 3, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(const Model*, getModel, 9, (), (this + sizeof(uintptr_t)))
    VIRTUAL_METHOD(const matrix3x4&, toWorldTransform, 34, (), (this + sizeof(uintptr_t)))

    VIRTUAL_METHOD(int&, handle, 2, (), (this))
    VIRTUAL_METHOD(Collideable*, getCollideable, 3, (), (this))

    VIRTUAL_METHOD(const Vector&, getAbsOrigin, 9, (), (this))
    VIRTUAL_METHOD(Vector&, getAbsAngle, 10, (), (this))
    VIRTUAL_METHOD(int, getMaxHealth, 107, (), (this))


    VIRTUAL_METHOD(Vector&, bulletSpread, 286, (), (this))
    VIRTUAL_METHOD(int, slot, 330, (), (this))
    VIRTUAL_METHOD(const char*, getPrintName, 333, (), (this))
    VIRTUAL_METHOD(int, damageType, 340, (), (this))
    VIRTUAL_METHOD(WeaponId, weaponId, 381, (), (this))

    bool isPlayer() noexcept
    {
        return getClassId() == ClassId::TFPlayer;
    }

    bool isAlive() noexcept
    {
        return lifeState() == 0;
    }

    bool isEnemy(Entity* entity) noexcept
    {
        return entity->teamNumber() != teamNumber();
    }

    bool isOnGround() noexcept
    {
        return groundEntity() >= 0 || flags() & 1;
    }

    bool isSwimming() noexcept
    {
        return waterLevel() > 1;
    }
    
    const char* getModelName() noexcept
    {
        return interfaces->modelInfo->getModelName(getModel());
    }

    Entity* getActiveWeapon() noexcept
    {
        return reinterpret_cast<Entity*>(interfaces->entityList->getEntityFromHandle(activeWeapon()));
    }

    std::string getPlayerName() noexcept;

    Vector getEyePosition() noexcept
    {
        return viewOffset() + getAbsOrigin();
    }

    ClassId getClassId() noexcept
    {
        const auto clientClass = getClientClass();
        return clientClass ? clientClass->classId : ClassId(0);
    }

    UtlVector<matrix3x4>& getBoneCache() noexcept
    {
        return *reinterpret_cast<UtlVector<matrix3x4>*>(reinterpret_cast<uintptr_t>(this) + 0x848);
    }

    Entity* getObjectOwner() noexcept
    {
        return reinterpret_cast<Entity*>(interfaces->entityList->getEntityFromHandle(objectBuilder()));
    }

    CONDITION(isCharging, condition(), TFCond_Charging)
    CONDITION(isScoped, condition(), TFCond_Zoomed)
    CONDITION(isUbered, condition(), TFCond_Ubercharged)
    CONDITION(isBonked, condition(), TFCond_Bonked)
    CONDITION(inMilk, condition(), TFCond_Milked)
    CONDITION(inJarate, condition(), TFCond_Jarated)
    CONDITION(isBleeding, condition(), TFCond_Bleeding)
    CONDITION(isDisguised, condition(), TFCond_Disguised)
    CONDITION(isCloaked, condition(), TFCond_Cloaked)
    CONDITION(isTaunting, condition(), TFCond_Taunting)
    CONDITION(isOnFire, condition(), TFCond_OnFire)
    CONDITION(isStunned, condition(), TFCond_Stunned)
    CONDITION(isSlowed, condition(), TFCond_Slowed)
    CONDITION(isMegaHealed, condition(), TFCond_MegaHeal)
    CONDITION(isAGhost, conditionEx2(), TFCondEx2_HalloweenGhostMode)
    CONDITION(isInBumperKart, conditionEx2(), TFCondEx_InKart)
    CONDITION(isPhlogUbered, conditionEx(), TFCondEx_PhlogUber)
    CONDITION(isBlastImmune, conditionEx2(), TFCondEx2_BlastImmune)
    CONDITION(isBulletImmune, conditionEx2(), TFCondEx2_BulletImmune)
    CONDITION(isFireImmune, conditionEx2(), TFCondEx2_FireImmune)
    CONDITION(hasStrengthRune, conditionEx2(), TFCondEx2_StrengthRune)
    CONDITION(hasHasteRune, conditionEx2(), TFCondEx2_HasteRune)
    CONDITION(hasRegenRune, conditionEx2(), TFCondEx2_RegenRune)
    CONDITION(hasResistRune, conditionEx2(), TFCondEx2_ResistRune)
    CONDITION(hasVampireRune, conditionEx2(), TFCondEx2_VampireRune)
    CONDITION(hasReflectRune, conditionEx2(), TFCondEx2_ReflectRune)
    CONDITION(hasPrecisionRune, conditionEx3(), TFCondEx3_PrecisionRune)
    CONDITION(hasAgilityRune, conditionEx3(), TFCondEx3_AgilityRune)
    CONDITION(hasKnockoutRune, conditionEx3(), TFCondEx3_KnockoutRune)
    CONDITION(hasImbalanceRune, conditionEx3(), TFCondEx3_ImbalanceRune)
    CONDITION(hasCritTempRune, conditionEx3(), TFCondEx3_CritboostedTempRune)
    CONDITION(hasKingRune, conditionEx3(), TFCondEx3_KingRune)
    CONDITION(hasPlagueRune, conditionEx3(), TFCondEx3_PlagueRune)
    CONDITION(hasSupernovaRune, conditionEx3(), TFCondEx3_SupernovaRune)
    CONDITION(hasBuffedByKing, conditionEx3(), TFCondEx3_KingBuff)
    CONDITION(hasBlastResist, conditionEx(), TFCondEx_ExplosiveCharge)
    CONDITION(hasBulletResist, conditionEx(), TFCondEx_BulletCharge)
    CONDITION(hasFireResist, conditionEx(), TFCondEx_FireCharge)

    NETVAR(clientSideAnimation, "CBaseAnimating", "m_bClientSideAnimation", bool)
    NETVAR(hitboxSet, "CBaseAnimating", "m_nHitboxSet", int)
    NETVAR(sequence, "CBaseAnimating", "m_nSequence", int)

    NETVAR(modelIndex, "CBaseEntity", "m_nModelIndex", unsigned)
    NETVAR(origin, "CBaseEntity", "m_vecOrigin", Vector)
    NETVAR_OFFSET(moveType, "CBaseEntity", "m_nRenderMode", 1, MoveType)
    NETVAR(simulationTime, "CBaseEntity", "m_flSimulationTime", float)
    NETVAR(ownerEntity, "CBaseEntity", "m_hOwnerEntity", int)
    NETVAR(spotted, "CBaseEntity", "m_bSpotted", bool)
    NETVAR(lifeState, "CBasePlayer", "m_lifeState", unsigned char)
    NETVAR(teamNumber, "CBaseEntity", "m_iTeamNum", Team)
    NETVAR(obbMins, "CBaseEntity", "m_vecMins", Vector)
    NETVAR(obbMaxs, "CBaseEntity", "m_vecMaxs", Vector)

    NETVAR(condition, "CTFPlayer", "m_nPlayerCond", int)
    NETVAR(conditionEx, "CTFPlayer", "m_nPlayerCondEx", int)
    NETVAR(conditionEx2, "CTFPlayer", "m_nPlayerCondEx2", int)
    NETVAR(conditionEx3, "CTFPlayer", "m_nPlayerCondEx3", int)
    NETVAR(getPlayerClass, "CTFPlayer", "m_iClass", TFClass)
    NETVAR(waterLevel, "CTFPlayer", "m_nWaterLevel", unsigned char)

    NETVAR(objectType, "CBaseObject", "m_iObjectType", int)
    NETVAR(objectMode, "CBaseObject", "m_iObjectMode", int)
    NETVAR(mapPlaced, "CBaseObject", "m_bWasMapPlaced", bool)
    NETVAR(isMini, "CBaseObject", "m_bMiniBuilding", bool)
    NETVAR(objectBuilder, "CBaseObject", "m_hBuilder", int)
    NETVAR(objectHealth, "CBaseObject", "m_iHealth", int)
    NETVAR(objectMaxHealth, "CBaseObject", "m_iMaxHealth", int)

    NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int[64])

    NETVAR(viewModel, "CBasePlayer", "m_hViewModel[0]", int)
    NETVAR(fov, "CBasePlayer", "m_iFOV", int)
    NETVAR(fovStart, "CBasePlayer", "m_iFOVStart", int)
    NETVAR(defaultFov, "CBasePlayer", "m_iDefaultFOV", int)
    NETVAR(flags, "CBasePlayer", "m_fFlags", int)
    NETVAR(tickBase, "CBasePlayer", "m_nTickBase", int)
    NETVAR(baseVelocity, "CBasePlayer", "m_vecBaseVelocity", Vector)
    NETVAR(viewPunchAngle, "CBasePlayer", "m_viewPunchAngle", Vector)
    NETVAR(viewOffset, "CBasePlayer", "m_vecViewOffset[0]", Vector)
    NETVAR(velocity, "CBasePlayer", "m_vecVelocity[0]", Vector)
    NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", const char*)
    NETVAR(fallVelocity, "CBasePlayer", "m_flFallVelocity", float)
    NETVAR(groundEntity, "CBasePlayer", "m_hGroundEntity", int)
    NETVAR(health, "CBasePlayer", "m_iHealth", int)

    NETVAR(activeWeapon, "CBaseCombatCharacter", "m_hActiveWeapon", int)
    NETVAR(nextAttack, "CBaseCombatCharacter", "m_flNextAttack", float)
};