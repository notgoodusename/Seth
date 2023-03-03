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
#include "Engine.h"
#include "EngineTrace.h"
#include "EntityList.h"
#include "GlobalVars.h"
#include "LocalPlayer.h"
#include "matrix3x4.h"
#include "MDLCache.h"
#include "ModelInfo.h"
#include "ModelRender.h"
#include "TFWeapons.h"
#include "Utils.h"
#include "UtlVector.h"
#include "Vector.h"
#include "VirtualMethod.h"
#include "WeaponId.h"

struct AnimState;

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

enum class WeaponType {
    UNKNOWN, 
    HITSCAN, 
    PROJECTILE, 
    MELEE,
    THROWABLE
};

enum WeaponSlots {
    SLOT_PRIMARY,
    SLOT_SECONDARY,
    SLOT_MELEE
};

class Collideable;

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
    VIRTUAL_METHOD(void, getRenderBounds, 20, (Vector& mins, Vector& maxs), (this + sizeof(uintptr_t), &mins, &maxs))
    VIRTUAL_METHOD(const matrix3x4&, toWorldTransform, 34, (), (this + sizeof(uintptr_t)))

    VIRTUAL_METHOD(Entity*, getBaseAnimating, 39, (), (this))

    VIRTUAL_METHOD(int&, handle, 2, (), (this))
    VIRTUAL_METHOD(Collideable*, getCollideable, 3, (), (this))

    VIRTUAL_METHOD(const Vector&, getAbsOrigin, 9, (), (this))
    VIRTUAL_METHOD(Vector&, getAbsAngle, 10, (), (this))
    VIRTUAL_METHOD(int, getMaxHealth, 107, (), (this))

    VIRTUAL_METHOD(void, think, 121, (), (this))
    VIRTUAL_METHOD(void, setSequence, 189, (int sequence), (this, sequence))
    VIRTUAL_METHOD(void, studioFrameAdvance, 190, (), (this))
    VIRTUAL_METHOD(void, updateClientSideAnimation, 193, (), (this))
    VIRTUAL_METHOD(void, preThink, 261, (), (this))
    VIRTUAL_METHOD(void, postThink, 262, (), (this))
    VIRTUAL_METHOD(Vector&, bulletSpread, 286, (), (this))
    VIRTUAL_METHOD(void, setLocalViewAngles, 302, (Vector& viewAngles), (this, &viewAngles))
    VIRTUAL_METHOD(int, slot, 330, (), (this))
    VIRTUAL_METHOD(const char*, getPrintName, 333, (), (this))
    VIRTUAL_METHOD(int, getDamageType, 340, (), (this))
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

    bool isKnife() noexcept
    {
        return weaponId() == WeaponId::KNIFE;
    }

    bool isVisible(const Vector& position = { }) noexcept
    {
        if (!localPlayer)
            return false;

        Trace trace;
        interfaces->engineTrace->traceRay({ localPlayer->getEyePosition(), position.notNull() ? position : getBonePosition(6) }, MASK_SHOT | CONTENTS_HITBOX, { localPlayer.get() }, trace);
        return trace.entity == this || trace.fraction > 0.97f;
    }

    void setCurrentCommand(UserCmd* cmd) noexcept
    {
        static int m_hConstraintEntity = Netvars::get(fnv::hash("CBasePlayer->m_hConstraintEntity"));
        *reinterpret_cast<UserCmd**>(reinterpret_cast<uintptr_t>(this) + m_hConstraintEntity - 4) = cmd;
    }

    void getAbsVelocity(Vector& vel) noexcept
    {
        EFlags() |= (1 << 12);
        memory->calcAbsoluteVelocity(this);
        vel.x = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x15C);
        vel.y = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x160);
        vel.z = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x164);
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

    WeaponType getWeaponType() noexcept
    {
        if (!this)
            return WeaponType::UNKNOWN;

        if (slot() == WeaponSlots::SLOT_MELEE)
            return WeaponType::MELEE;

        switch (weaponId())
        {
            case WeaponId::ROCKETLAUNCHER:
            case WeaponId::FLAME_BALL:
            case WeaponId::GRENADELAUNCHER:
            case WeaponId::FLAREGUN:
            case WeaponId::COMPOUND_BOW:
            case WeaponId::DIRECTHIT:
            case WeaponId::CROSSBOW:
            case WeaponId::PARTICLE_CANNON:
            case WeaponId::DRG_POMSON:
            case WeaponId::FLAREGUN_REVENGE:
            case WeaponId::RAYGUN:
            case WeaponId::CANNON:
            case WeaponId::SYRINGEGUN_MEDIC:
            case WeaponId::SHOTGUN_BUILDING_RESCUE:
            case WeaponId::FLAMETHROWER:
            case WeaponId::CLEAVER:
            case WeaponId::PIPEBOMBLAUNCHER:
                return WeaponType::PROJECTILE;
            default:
                const int damageType = getDamageType();
                if (damageType & (1 << 1) || damageType && (1 << 29))
                    return WeaponType::HITSCAN;
                break;
        }
        int currentItemDefinitonIndex = itemDefinitionIndex();
        if (currentItemDefinitonIndex == Heavy_s_RoboSandvich ||
            currentItemDefinitonIndex == Heavy_s_Sandvich ||
            currentItemDefinitonIndex == Heavy_s_FestiveSandvich ||
            currentItemDefinitonIndex == Heavy_s_Fishcake ||
            currentItemDefinitonIndex == Heavy_s_TheDalokohsBar ||
            currentItemDefinitonIndex == Heavy_s_SecondBanana) {
            return WeaponType::THROWABLE;
        }

        return WeaponType::UNKNOWN;
    }

    float getSwingRange() noexcept
    {
        if (weaponId() == WeaponId::SWORD)
            return 72; // swords are typically 72
        return 48;
    }

    Vector getWorldSpaceCenter() noexcept
    {
        Vector mins, maxs;
        getRenderBounds(mins, maxs);
        Vector worldSpaceCenter = getAbsOrigin();
        worldSpaceCenter.z += (mins.z + maxs.z) / 2.0f;
        return worldSpaceCenter;
    }

    Entity* getGroundEntity() noexcept
    {
        return interfaces->entityList->getEntityFromHandle(groundEntity());
    }

    Entity* getObserverTarget() noexcept
    {
        return interfaces->entityList->getEntityFromHandle(observerTarget());
    }

    Vector getBonePosition(int bone) noexcept
    {
        if (matrix3x4 boneMatrices[MAXSTUDIOBONES]; setupBones(boneMatrices, MAXSTUDIOBONES, 256, 0.0f))
            return boneMatrices[bone].origin();
        else
            return Vector{ };
    }

    bool setupBones(matrix3x4* out, int maxBones, int boneMask, float currentTime) noexcept
    {
        Vector absOrigin = getAbsOrigin();

        this->invalidateBoneCache();

        memory->setAbsOrigin(this, origin());

        auto result = VirtualMethod::call<bool, 16>(this + sizeof(uintptr_t), out, maxBones, boneMask, currentTime);

        memory->setAbsOrigin(this, absOrigin);
        return result;
    }

    void invalidateBoneCache() noexcept
    {
        *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x59C) = UINT_MAX; //m_iMostRecentModelBoneCounter = g_iModelBoneCounter - 1
        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x860) = -FLT_MAX; //m_flLastBoneSetupTime = -FLT_MAX
    }

    int& EFlags() noexcept
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x1A0);
    }

    int& getButtons() noexcept
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x1180);
    }

    int& getButtonLast() noexcept
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x1174);
    }

    int& getButtonPressed() noexcept
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x1178);
    }

    int& getButtonReleased() noexcept
    {
        return *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x117C);
    }

    void updateButtonState(int userCmdButtonMask) noexcept
    {
        getButtonLast() = getButtons();

        getButtons() = userCmdButtonMask;
        const int buttonsChanged = getButtonLast() ^ getButtons();

        getButtonPressed() = buttonsChanged & getButtons();
        getButtonReleased() = buttonsChanged & (~getButtons());
    }

    bool physicsRunThink(int thinkMethod = 0) noexcept
    {
        return memory->physicsRunThink(this, thinkMethod);
    }

    void runPreThink() noexcept
    {
        if (physicsRunThink())
            preThink();
    }

    void runThink() noexcept
    {
        const auto nextThinkTick = memory->getNextThinkTick(this, NULL);
        if (nextThinkTick > 0 &&
            nextThinkTick <= tickBase())
        {
            memory->setNextThink(this, -1, NULL);
            think();
        }
    }

    void runPostThink() noexcept
    {
        postThink();
    }

    bool isInReload() noexcept
    {
        static auto nextPrimaryAttack = Netvars::get(fnv::hash("CBaseCombatWeapon->m_flNextPrimaryAttack"));
        bool inReload = *reinterpret_cast<bool*>(this + (nextPrimaryAttack + 0xC));
        int reloadMode = *reinterpret_cast<int*>(this + 0xB28);
        return (inReload || reloadMode != 0);
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
    NETVAR(modelScale, "CBaseAnimating", "m_flModelScale", float)

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
    NETVAR(forceTauntCam, "CTFPlayer", "m_nForceTauntCam", int)
    NETVAR(isFeignDeathReady, "CTFPlayer", "m_bFeignDeathReady", bool)
    NETVAR(eyeAngles, "CTFPlayer", "m_angEyeAngles[0]", Vector)

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
    NETVAR(observerTarget, "CBasePlayer", "m_hObserverTarget", int)
    NETVAR(getObserverMode, "CBasePlayer", "m_iObserverMode", ObsMode)
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

    NETVAR(lastFireTime, "CTFWeaponBase", "m_flLastFireTime", float)

    NETVAR(clip, "CBaseCombatWeapon", "m_iClip1", int)
    NETVAR(nextSecondaryAttack, "CBaseCombatWeapon","m_flNextSecondaryAttack", float)
    NETVAR(nextPrimaryAttack, "CBaseCombatWeapon", "m_flNextPrimaryAttack", float)

    NETVAR(itemDefinitionIndex, "CEconEntity", "m_iItemDefinitionIndex", int)
};

class Collideable {
public:
    PAD(4)
    Entity* outer;
    Vector vecMinsPreScaled;
    Vector vecMaxsPreScaled;
    Vector vecMins;
    Vector vecMaxs;
    float radius;
    unsigned short solidFlags;
    unsigned short partition;
    unsigned char surroundType;
    unsigned char solidType;
    unsigned char triggerBloat;
    Vector vecSpecifiedSurroundingMinsPreScaled;
    Vector vecSpecifiedSurroundingMaxsPreScaled;
    Vector vecSpecifiedSurroundingMins;
    Vector vecSpecifiedSurroundingMaxs;
    Vector vecSurroundingMins;
    Vector vecSurroundingMaxs;

    VIRTUAL_METHOD(const Vector&, obbMins, 3, (), (this))
    VIRTUAL_METHOD(const Vector&, obbMaxs, 4, (), (this))

    void setCollisionBounds(const Vector& mins, const Vector& maxs) noexcept
    {
        //Imma leave this here
        if ((vecMinsPreScaled != mins) || (vecMaxsPreScaled != maxs))
        {
            vecMinsPreScaled = mins;
            vecMaxsPreScaled = maxs;
        }

        bool dirty = false;

        Entity* baseAnimating = outer->getBaseAnimating();
        if (baseAnimating && baseAnimating->modelScale() != 1.0f)
        {
            Vector vecNewMins = mins * baseAnimating->modelScale();
            Vector vecNewMaxs = maxs * baseAnimating->modelScale();

            if ((vecMins != vecNewMins) || (vecMaxs != vecNewMaxs))
            {
                vecMins = vecNewMins;
                vecMaxs = vecNewMaxs;
                dirty = true;
            }
        }
        else
        {
            if ((vecMins != mins) || (vecMaxs != maxs))
            {
                vecMins = mins;
                vecMaxs = maxs;
                dirty = true;
            }
        }

        if (dirty)
        {
            Vector vecSize;
            Vector::vectorSubtract(vecMaxs, vecMins, vecSize);
            radius = vecSize.length() * 0.5f;

            outer->EFlags() |= (1 << 14);

            if (outer->index() == 0)
                return;

            if (!(outer->EFlags() & (1 << 15)))
            {
                outer->EFlags() |= (1 << 15);
            }
        }
    }
};