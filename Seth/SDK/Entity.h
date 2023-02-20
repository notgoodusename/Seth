#pragma once

#include <functional>

#include "../Config.h"
#include "../Interfaces.h"
#include "../Memory.h"
#include "../Netvars.h"

#include "ClientClass.h"
#include "CommandContext.h"
#include "Cvar.h"
#include "Datamap.h"
#include "Engine.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "matrix3x4.h"
#include "MDLCache.h"
#include "ModelRender.h"
#include "Utils.h"
#include "UtlVector.h"
#include "Vector.h"
#include "VirtualMethod.h"

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
    TT,
    CT
};

class Collideable {
public:
    PAD(8); //0
    Vector vecMins; //8(x), 12(y), 16(z) 
    Vector vecMaxs; //20(x), 24(y), 28(z)
    unsigned short solidFlags; //32
    unsigned char solidType; //34
    unsigned char triggerBloat; //35
    float radius; //36

    VIRTUAL_METHOD(Vector&, obbMins, 1, (), (this))
    VIRTUAL_METHOD(Vector&, obbMaxs, 2, (), (this))
};

class Entity {
public:

    bool isAlive() noexcept
    {
        return lifeState() == 0;
    }

    bool isOnGround() noexcept
    {
        return groundEntity() >= 0 || flags() & 1;
    }

    UtlVector<matrix3x4>& getBoneCache() noexcept
    {
        return *reinterpret_cast<UtlVector<matrix3x4>*>(reinterpret_cast<uintptr_t>(this) + 0x2138);
    }

    NETVAR(didSmokeEffect, "CSmokeGrenadeProjectile", "m_bDidSmokeEffect", bool)

    NETVAR(pinPulled, "CBaseCSGrenade", "m_bPinPulled", bool);
    NETVAR(throwTime, "CBaseCSGrenade", "m_fThrowTime", float_t);

    NETVAR(body, "CBaseAnimating", "m_nBody", int)
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

    NETVAR(weapons, "CBaseCombatCharacter", "m_hMyWeapons", int[64])
    PNETVAR(wearables, "CBaseCombatCharacter", "m_hMyWearables", int)

    NETVAR(viewModel, "CBasePlayer", "m_hViewModel[0]", int)
    NETVAR(fov, "CBasePlayer", "m_iFOV", int)
    NETVAR(fovStart, "CBasePlayer", "m_iFOVStart", int)
    NETVAR(defaultFov, "CBasePlayer", "m_iDefaultFOV", int)
    NETVAR(flags, "CBasePlayer", "m_fFlags", int)
    NETVAR(tickBase, "CBasePlayer", "m_nTickBase", int)
    NETVAR(aimPunchAngle, "CBasePlayer", "m_aimPunchAngle", Vector)
    NETVAR(aimPunchAngleVelocity, "CBasePlayer", "m_aimPunchAngleVel", Vector)
    NETVAR(baseVelocity, "CBasePlayer", "m_vecBaseVelocity", Vector)
    NETVAR(viewPunchAngle, "CBasePlayer", "m_viewPunchAngle", Vector)
    NETVAR(viewOffset, "CBasePlayer", "m_vecViewOffset[0]", Vector)
    NETVAR(velocity, "CBasePlayer", "m_vecVelocity[0]", Vector)
    NETVAR(lastPlaceName, "CBasePlayer", "m_szLastPlaceName", char[18])
    NETVAR(getLadderNormal, "CBasePlayer", "m_vecLadderNormal", Vector)
    NETVAR(duckAmount, "CBasePlayer", "m_flDuckAmount", float)
    NETVAR(duckSpeed, "CBasePlayer", "m_flDuckSpeed", float)
    NETVAR(fallVelocity, "CBasePlayer", "m_flFallVelocity", float)
    NETVAR(groundEntity, "CBasePlayer", "m_hGroundEntity", int)

    NETVAR(armor, "CCSPlayer", "m_ArmorValue", int)
    NETVAR(hasHeavyArmor, "CCSPlayer", "m_bHasHeavyArmor", bool)
    NETVAR(eyeAngles, "CCSPlayer", "m_angEyeAngles", Vector)
    NETVAR(isScoped, "CCSPlayer", "m_bIsScoped", bool)
    NETVAR(isDefusing, "CCSPlayer", "m_bIsDefusing", bool)
    NETVAR(flashMaxAlpha, "CCSPlayer", "m_flFlashMaxAlpha", float)
    NETVAR_OFFSET(flashDuration, "CCSPlayer", "m_flFlashMaxAlpha", 4, float)
    NETVAR_OFFSET(flashBuildUp, "CCSPlayer", "m_flFlashMaxAlpha", -4, bool)
    NETVAR_OFFSET(flashOverlayAlpha, "CCSPlayer", "m_flFlashMaxAlpha", -8, float)
    NETVAR_OFFSET(flashScreenshotAlpha, "CCSPlayer", "m_flFlashMaxAlpha", -12, float)
    NETVAR_OFFSET(flashBangTime, "CCSPlayer", "m_flFlashMaxAlpha", -16, float)
    NETVAR(gunGameImmunity, "CCSPlayer", "m_bGunGameImmunity", bool)
    NETVAR(account, "CCSPlayer", "m_iAccount", int)
    NETVAR(inBombZone, "CCSPlayer", "m_bInBombZone", bool)
    NETVAR(hasDefuser, "CCSPlayer", "m_bHasDefuser", bool)
    NETVAR(hasHelmet, "CCSPlayer", "m_bHasHelmet", bool)
    NETVAR(lby, "CCSPlayer", "m_flLowerBodyYawTarget", float)
    NETVAR(ragdoll, "CCSPlayer", "m_hRagdoll", int)
    NETVAR(shotsFired, "CCSPlayer", "m_iShotsFired", int)
    NETVAR(money, "CCSPlayer", "m_iAccount", int)
    NETVAR(waitForNoAttack, "CCSPlayer", "m_bWaitForNoAttack", bool)
    NETVAR(isStrafing, "CCSPlayer", "m_bStrafing", bool)
    NETVAR(moveState, "CCSPlayer", "m_iMoveState", int)
    NETVAR(duckOverride, "CCSPlayer", "m_bDuckOverride", bool)
    NETVAR(stamina, "CCSPlayer", "m_flStamina", float)
    NETVAR(thirdPersonRecoil, "CCSPlayer", "m_flThirdpersonRecoil", float)
    NETVAR(velocityModifier, "CCSPlayer", "m_flVelocityModifier", float)

    NETVAR(viewModelIndex, "CBaseCombatWeapon", "m_iViewModelIndex", int)
    NETVAR(worldModelIndex, "CBaseCombatWeapon", "m_iWorldModelIndex", int)
    NETVAR(worldDroppedModelIndex, "CBaseCombatWeapon", "m_iWorldDroppedModelIndex", int)
    NETVAR(weaponWorldModel, "CBaseCombatWeapon", "m_hWeaponWorldModel", int)
    NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
    NETVAR(reserveAmmoCount, "CBaseCombatWeapon", "m_iPrimaryReserveAmmoCount", int)
    NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)
    NETVAR(nextSecondaryAttack, "CBaseCombatWeapon", "m_flNextSecondaryAttack", float)
    NETVAR(recoilIndex, "CBaseCombatWeapon", "m_flRecoilIndex", float)

    NETVAR(nextAttack, "CBaseCombatCharacter", "m_flNextAttack", float)

    NETVAR(readyTime, "CWeaponCSBase", "m_flPostponeFireReadyTime", float)
    NETVAR(burstMode, "CWeaponCSBase", "m_bBurstMode", bool)

    NETVAR(burstShotRemaining, "CWeaponCSBaseGun", "m_iBurstShotsRemaining", int)
    NETVAR(zoomLevel, "CWeaponCSBaseGun", "m_zoomLevel", int)

    NETVAR(accountID, "CBaseAttributableItem", "m_iAccountID", int)
    NETVAR(itemDefinitionIndex, "CBaseAttributableItem", "m_iItemDefinitionIndex", short)
    NETVAR(itemDefinitionIndex2, "CBaseAttributableItem", "m_iItemDefinitionIndex", WeaponId)
    NETVAR(itemIDHigh, "CBaseAttributableItem", "m_iItemIDHigh", int)
    NETVAR(entityQuality, "CBaseAttributableItem", "m_iEntityQuality", int)
    NETVAR(customName, "CBaseAttributableItem", "m_szCustomName", char[32])
    NETVAR(fallbackPaintKit, "CBaseAttributableItem", "m_nFallbackPaintKit", unsigned)
    NETVAR(fallbackSeed, "CBaseAttributableItem", "m_nFallbackSeed", unsigned)
    NETVAR(fallbackWear, "CBaseAttributableItem", "m_flFallbackWear", float)
    NETVAR(fallbackStatTrak, "CBaseAttributableItem", "m_nFallbackStatTrak", unsigned)
    NETVAR(initialized, "CBaseAttributableItem", "m_bInitialized", bool)

    NETVAR(owner, "CBaseViewModel", "m_hOwner", int)
    NETVAR(weapon, "CBaseViewModel", "m_hWeapon", int)

    NETVAR(c4StartedArming, "CC4", "m_bStartedArming", bool)

    NETVAR(tabletReceptionIsBlocked, "CTablet", "m_bTabletReceptionIsBlocked", bool)
    
    NETVAR(droneTarget, "CDrone", "m_hMoveToThisEntity", int)

    NETVAR(thrower, "CBaseGrenade", "m_hThrower", int)
        
    NETVAR(mapHasBombTarget, "CCSGameRulesProxy", "m_bMapHasBombTarget", bool)
    NETVAR(freezePeriod, "CCSGameRulesProxy", "m_bFreezePeriod", bool)
    NETVAR(isValveDS, "CCSGameRulesProxy", "m_bIsValveDS", bool)

    NETVAR(fireXDelta, "CInferno", "m_fireXDelta", int[100])
    NETVAR(fireYDelta, "CInferno", "m_fireYDelta", int[100])
    NETVAR(fireZDelta, "CInferno", "m_fireZDelta", int[100])
    NETVAR(fireIsBurning, "CInferno", "m_bFireIsBurning", bool[100])
    NETVAR(fireCount, "CInferno", "m_fireCount", int)
       
    float getFlashStartTime() noexcept 
    {
        return (flashBangTime() - flashDuration()); 
    }
    float getFlashTimeElapsed() noexcept
    { 
        return max(memory->globalVars->currenttime - getFlashStartTime(), 0.0f); 
    }

    bool isFlashed() noexcept
    {
        return flashOverlayAlpha() > 75.0f;
    }
};

class PlantedC4 : public Entity {
public:
    NETVAR(c4BlowTime, "CPlantedC4", "m_flC4Blow", float)
    NETVAR(c4TimerLength, "CPlantedC4", "m_flTimerLength", float)
    NETVAR(c4BombSite, "CPlantedC4", "m_nBombSite", int)
    NETVAR(c4Ticking, "CPlantedC4", "m_bBombTicking", bool)
    NETVAR(c4DefuseCountDown, "CPlantedC4", "m_flDefuseCountDown", float)
    NETVAR(c4DefuseLength, "CPlantedC4", "m_flDefuseLength", float)
    NETVAR(c4Defuser, "CPlantedC4", "m_hBombDefuser", int)
};

class FogController : public Entity {
public:
    NETVAR(enable, "CFogController", "m_fog.enable", int)
    NETVAR(start, "CFogController", "m_fog.start", float)
    NETVAR(end, "CFogController", "m_fog.end", float)
    NETVAR(density, "CFogController", "m_fog.maxdensity", float)
    NETVAR(color, "CFogController", "m_fog.colorPrimary", int)
};
