#include "Aimbot.h"
#include "AimbotMelee.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"

#include "../../SDK/AttributeManager.h"
#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

Vector getKnifeTarget(UserCmd* cmd, Entity* entity, Entity* activeWeapon,
    float& bestFov, Vector localPlayerEyePosition, Vector eyeAngle, Vector worldSpaceCenter) noexcept
{
    const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, worldSpaceCenter, cmd->viewangles) };
    const float fov{ angle.length2D() };
    if (fov > bestFov)
        return Vector{ 0.0f, 0.0f, 0.0f };

    const auto targetAngle = (cmd->viewangles + angle).normalize();

    if (!Math::canBackstab(targetAngle, eyeAngle, worldSpaceCenter)
        || !Math::doesMeleeHit(activeWeapon, entity->index(), targetAngle))
        return Vector{ 0.0f, 0.0f, 0.0f };

    if (fov < bestFov) {
        bestFov = fov;
        return worldSpaceCenter;
    }

    return Vector{ 0.0f , 0.0f, 0.0f };
}

Vector getMeleeTarget(UserCmd* cmd, Entity* entity, Vector worldSpaceCenter, Entity* activeWeapon,
    float& bestFov, Vector localPlayerEyePosition) noexcept
{
    const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, worldSpaceCenter, cmd->viewangles) };
    const float fov{ angle.length2D() };
    if (fov > bestFov)
        return Vector{ 0.0f, 0.0f, 0.0f };

    const auto possibleAngle = (cmd->viewangles + angle).normalize();

    if (!Math::doesMeleeHit(activeWeapon, entity->index(), possibleAngle))
        return Vector{ 0.0f, 0.0f, 0.0f };

    if (fov < bestFov) {
        bestFov = fov;
        return worldSpaceCenter;
    }

    return Vector{ 0.0f, 0.0f, 0.0f };
}

void runKnife(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.melee;

    float swingRange = activeWeapon->getSwingRange();
    if (swingRange <= 0.0f)
        return;

    float modelScale = localPlayer->modelScale();
    if (modelScale > 1.0f)
        swingRange *= modelScale;

    swingRange = AttributeManager::attributeHookFloat(swingRange, "melee_range_multiplier", activeWeapon);

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    float bestSimulationTime{ -1.0f };
    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    const bool ignoreCloaked = (cfg.ignore & 1 << 0) == 1 << 0;
    const bool ignoreInvulnerable = (cfg.ignore & 1 << 1) == 1 << 1;
    for (const auto& target : enemies)
    {
        if (target.playerData.empty() || !target.isAlive || target.priority == 0)
            continue;

        if (target.distanceToLocal > swingRange * 4.0f)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity ||
            (entity->isCloaked() && ignoreCloaked) ||
            (entity->isInvulnerable() && ignoreInvulnerable) ||
            (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4 backupBoneCache[MAXSTUDIOBONES];
        memcpy(backupBoneCache, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        const auto& records = target.playerData;
        
        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            for (int i = 0; i < static_cast<int>(records.size()); i++)
            {
                if (!Backtrack::valid(records[i].simulationTime))
                    continue;
                
                const auto& targetTick = records[i];

                entity->replaceMatrix(targetTick.matrix.data());
                memory->setAbsOrigin(entity, targetTick.origin);
                memory->setAbsAngle(entity, targetTick.absAngle);
                memory->setCollisionBounds(entity->getCollideable(), targetTick.minsPrescaled, targetTick.maxsPrescaled);

                Vector worldSpaceCenter = targetTick.origin;
                worldSpaceCenter.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

                bestTarget = getKnifeTarget(cmd, entity, activeWeapon, bestFov, localPlayerEyePosition, targetTick.eyeAngle, worldSpaceCenter);
                
                entity->replaceMatrix(backupBoneCache);
                memory->setAbsOrigin(entity, backupOrigin);
                memory->setAbsAngle(entity, backupAbsAngle);
                memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);

                if (bestTarget.notNull())
                {
                    bestSimulationTime = targetTick.simulationTime;
                    break;
                }
            }
        }
        else
        {
            int bestTick = -1;
            for (int i = static_cast<int>(records.size() - 1U); i >= 0; i--)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                bestTick = i;
            }

            if (bestTick <= -1)
                continue;

            const auto& targetTick = records[bestTick];

            entity->replaceMatrix(targetTick.matrix.data());
            memory->setAbsOrigin(entity, targetTick.origin);
            memory->setAbsAngle(entity, targetTick.absAngle);
            memory->setCollisionBounds(entity->getCollideable(), targetTick.minsPrescaled, targetTick.maxsPrescaled);

            Vector worldSpaceCenter = targetTick.origin;
            worldSpaceCenter.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

            bestTarget = getKnifeTarget(cmd, entity, activeWeapon, bestFov, localPlayerEyePosition, targetTick.eyeAngle, worldSpaceCenter);
            
            entity->replaceMatrix(backupBoneCache);
            memory->setAbsOrigin(entity, backupOrigin);
            memory->setAbsAngle(entity, backupAbsAngle);
            memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);

            if (bestTarget.notNull())
            {
                bestSimulationTime = targetTick.simulationTime;
                break;
            }
        }

        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull())
    {
        const auto angle = Math::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles);

        if (cfg.autoHit)
            cmd->buttons |= UserCmd::IN_ATTACK;

        if (cmd->buttons & UserCmd::IN_ATTACK)
            cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());

        cmd->viewangles += angle;

        if (!cfg.silent)
            interfaces->engine->setViewAngles(cmd->viewangles);
    }
}

struct MeleeRecord
{
public:
    int commandNumber{ -1 };
    int index;
    Vector origin;
    Vector absAngle;
    Vector target;
    Vector mins;
    Vector maxs;
    float simulationTime;
    matrix3x4* matrix;
} meleeRecord;

//TODO: Need optimizing
//Remove all distant players if not in reach

void AimbotMelee::run(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.melee;
    if (!cfg.enabled)
        return;

    //Due to the delay of melee registering after shooting we gotta do this
    //And also recalculate melee
    if (meleeRecord.commandNumber == cmd->commandNumber)
    {
        const auto entity{ interfaces->entityList->getEntityFromHandle(meleeRecord.index) };
        if (entity)
        {
            //We gotta recalculate to aim correctly
            const auto angle = Math::calculateRelativeAngle(localPlayer->getEyePosition(), meleeRecord.target, cmd->viewangles);

            matrix3x4 backupBoneCache[MAXSTUDIOBONES];
            memcpy(backupBoneCache, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
            Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
            Vector backupOrigin = entity->getAbsOrigin();
            Vector backupAbsAngle = entity->getAbsAngle();

            entity->replaceMatrix(meleeRecord.matrix);
            memory->setAbsOrigin(entity, meleeRecord.origin);
            memory->setAbsAngle(entity, meleeRecord.absAngle);
            memory->setCollisionBounds(entity->getCollideable(), meleeRecord.mins, meleeRecord.maxs);
            if (Math::doesMeleeHit(activeWeapon, meleeRecord.index, cmd->viewangles + angle))
            {
                cmd->viewangles += angle;
                cmd->tickCount = timeToTicks(meleeRecord.simulationTime + Backtrack::getLerp());

                if (!cfg.silent)
                    interfaces->engine->setViewAngles(cmd->viewangles);
            }
            entity->replaceMatrix(backupBoneCache);
            memory->setAbsOrigin(entity, backupOrigin);
            memory->setAbsAngle(entity, backupAbsAngle);
            memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);
        }
    }

    if (!cfg.autoHit && !cfg.aimlock && !isAttacking(cmd, activeWeapon))
        return;

    if (!canAttack(cmd, activeWeapon))
        return;

    //The knife has no delay to register a hit
    if (activeWeapon->isKnife())
        return runKnife(activeWeapon, cmd);

    float swingRange = activeWeapon->getSwingRange();
    if (swingRange <= 0.0f)
        return;
    
    float modelScale = localPlayer->modelScale();
    if (modelScale > 1.0f)
        swingRange *= modelScale;

    swingRange = AttributeManager::attributeHookFloat(swingRange, "melee_range_multiplier", activeWeapon);

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    const bool ignoreCloaked = (cfg.ignore & 1 << 0) == 1 << 0;
    const bool ignoreInvulnerable = (cfg.ignore & 1 << 1) == 1 << 1;
    for (const auto& target : enemies)
    {
        if (target.playerData.empty() || !target.isAlive || target.priority == 0)
            continue;

        if (target.distanceToLocal > swingRange * 4.0f)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity ||
            (entity->isCloaked() && ignoreCloaked) ||
            (entity->isInvulnerable() && ignoreInvulnerable) ||
            (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4 backupBoneCache[MAXSTUDIOBONES];
        memcpy(backupBoneCache, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        
        const auto& records = target.playerData;

        int bestTick = -1;
        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            auto bestBacktrackDistance = FLT_MAX;
            for (int i = static_cast<int>(records.size() - 1U); i >= 0; i--)
            {
                //We gotta make sure if we fire now it will be correct when registering 0.2 seconds later and can register right now
                if (!Backtrack::valid(records[i].simulationTime - 0.2121f) 
                    || !Backtrack::valid(records[i].simulationTime))
                    continue;

                const auto distance{ localPlayerOrigin.distTo(entity->getAbsOrigin()) };

                if (distance < bestBacktrackDistance)
                {
                    bestTick = i;
                    bestBacktrackDistance = distance;
                }
            }
        }
        else
        {
            for (int i = 0; i < static_cast<int>(records.size()); i++)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                bestTick = i;
            }
        }

        if (bestTick <= -1)
            continue;

        const auto& targetTick = records[bestTick];

        entity->replaceMatrix(targetTick.matrix.data());
        memory->setAbsOrigin(entity, targetTick.origin);
        memory->setAbsAngle(entity, targetTick.absAngle);
        memory->setCollisionBounds(entity->getCollideable(), targetTick.minsPrescaled, targetTick.maxsPrescaled);

        meleeRecord.matrix = entity->getBoneCache().memory;
        meleeRecord.mins = entity->getCollideable()->obbMinsPreScaled();
        meleeRecord.maxs = entity->getCollideable()->obbMaxsPreScaled();
        meleeRecord.origin = entity->getAbsOrigin();
        meleeRecord.absAngle = entity->getAbsAngle();

        Vector worldSpaceCenter = targetTick.origin;
        worldSpaceCenter.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

        bestTarget = getMeleeTarget(cmd, entity, worldSpaceCenter, activeWeapon, bestFov, localPlayerEyePosition);
        
        entity->replaceMatrix(backupBoneCache);
        memory->setAbsOrigin(entity, backupOrigin);
        memory->setAbsAngle(entity, backupAbsAngle);
        memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);
        if (bestTarget.notNull())
        {
            meleeRecord.target = bestTarget;
            meleeRecord.index = target.handle;
            meleeRecord.simulationTime = targetTick.simulationTime;
            meleeRecord.commandNumber = cmd->commandNumber + static_cast<int>(round(0.2121f / memory->globalVars->intervalPerTick));
            cmd->buttons |= UserCmd::IN_ATTACK;
            break;
        }
    }
}

void AimbotMelee::reset() noexcept
{
    meleeRecord.commandNumber = -1;
}