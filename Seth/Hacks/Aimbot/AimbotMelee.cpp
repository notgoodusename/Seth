#include "Aimbot.h"
#include "AimbotMelee.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"

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

    Vector possibleAngle = cmd->viewangles + angle;
    possibleAngle.normalize();

    if (!Math::canBackstab(possibleAngle, eyeAngle, worldSpaceCenter) 
        || !Math::doesMeleeHit(activeWeapon, entity->index(), possibleAngle))
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

    Vector possibleAngle = cmd->viewangles + angle;
    possibleAngle.normalize();

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

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    float bestSimulationTime{ -1.0f };
    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    for (const auto& target : enemies)
    {
        if (!target.isValid || target.priority == 0)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            const auto& records = target.backtrackRecords;
            if (records.empty() || records.size() <= 3U)
                continue;

            for (int i = static_cast<int>(records.size() - 1U); i >= 3; i--)
            {
                if (!Backtrack::valid(records[i].simulationTime))
                    continue;

                memcpy(entity->getBoneCache().memory, records[i].matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, records[i].origin);
                memory->setAbsAngle(entity, records[i].absAngle);
                memory->setCollisionBounds(entity->getCollideable(), records[i].mins, records[i].maxs);

                bestTarget = getKnifeTarget(cmd, entity, activeWeapon, bestFov, localPlayerEyePosition, records[i].eyeAngle, records[i].worldSpaceCenter);
                applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);

                if (bestTarget.notNull())
                {
                    bestSimulationTime = records[i].simulationTime;
                    break;
                }
            }
        }
        else
        {
            memcpy(entity->getBoneCache().memory, target.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, target.origin);
            memory->setAbsAngle(entity, target.absAngle);
            memory->setCollisionBounds(entity->getCollideable(), target.mins, target.maxs);

            bestTarget = getKnifeTarget(cmd, entity, activeWeapon, bestFov, localPlayerEyePosition, target.eyeAngle, target.worldSpaceCenter);

            applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);

            if (bestTarget.notNull())
            {
                bestSimulationTime = target.simulationTime;
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
    //If you miss its because of movement
    if (meleeRecord.commandNumber == cmd->commandNumber)
    {
        const auto entity{ interfaces->entityList->getEntityFromHandle(meleeRecord.index) };
        if (entity)
        {
            //We gotta recalculate to aim correctly
            const auto angle = Math::calculateRelativeAngle(localPlayer->getEyePosition(), meleeRecord.target, cmd->viewangles);

            matrix3x4* backupBoneCache = entity->getBoneCache().memory;
            Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
            Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
            Vector backupOrigin = entity->getAbsOrigin();
            Vector backupAbsAngle = entity->getAbsAngle();

            applyMatrix(entity, meleeRecord.matrix, meleeRecord.origin, meleeRecord.absAngle, meleeRecord.mins, meleeRecord.maxs);
            if (Math::doesMeleeHit(activeWeapon, meleeRecord.index, cmd->viewangles + angle))
            {
                cmd->viewangles += angle;
                cmd->tickCount = timeToTicks(meleeRecord.simulationTime + Backtrack::getLerp());

                if (!cfg.silent)
                    interfaces->engine->setViewAngles(cmd->viewangles);
            }
            applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);
        }
    }

    if (!cfg.autoHit && !cfg.aimlock && !isAttacking(cmd, activeWeapon))
        return;

    if (!canAttack(cmd, activeWeapon))
        return;

    //The knife has no delay to register a hit
    if (activeWeapon->isKnife())
        return runKnife(activeWeapon, cmd);

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    Vector worldSpaceCenter{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    for (const auto& target : enemies)
    {
        if (!target.isValid || target.priority == 0)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity || (entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        float currentSimulationTime = -1.0f;

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            const auto& records = target.backtrackRecords;
            if (records.empty() || records.size() <= 3U)
                continue;

            auto bestBacktrackDistance = FLT_MAX;
            int bestTick = -1;
            for (size_t i = 0; i < records.size(); i++)
            {
                //We gotta make sure if we fire now it will be correct when registering 0.2 seconds later and can register right now
                if (Backtrack::valid(records[i].simulationTime - 0.2121f) && Backtrack::valid(records[i].simulationTime))
                {
                    const auto distance{ localPlayerOrigin.distTo(entity->getAbsOrigin()) };

                    if (distance < bestBacktrackDistance)
                    {
                        bestTick = i;
                        bestBacktrackDistance = distance;
                    }
                }
            }

            if (bestTick <= -1)
                continue;

            memcpy(entity->getBoneCache().memory, records[bestTick].matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, records[bestTick].origin);
            memory->setAbsAngle(entity, records[bestTick].absAngle);
            memory->setCollisionBounds(entity->getCollideable(), records[bestTick].mins, records[bestTick].maxs);

            worldSpaceCenter = records[bestTick].worldSpaceCenter;
            currentSimulationTime = records[bestTick].simulationTime;
        }
        else
        {
            memcpy(entity->getBoneCache().memory, target.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, target.origin);
            memory->setAbsAngle(entity, target.absAngle);
            memory->setCollisionBounds(entity->getCollideable(), target.mins, target.maxs);

            worldSpaceCenter = target.worldSpaceCenter;
            currentSimulationTime = target.simulationTime;
        }

        meleeRecord.matrix = entity->getBoneCache().memory;
        meleeRecord.mins = entity->getCollideable()->obbMinsPreScaled();
        meleeRecord.maxs = entity->getCollideable()->obbMaxsPreScaled();
        meleeRecord.origin = entity->getAbsOrigin();
        meleeRecord.absAngle = entity->getAbsAngle();

        bestTarget = getMeleeTarget(cmd, entity, worldSpaceCenter, activeWeapon, bestFov, localPlayerEyePosition);
        applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);
        if (bestTarget.notNull())
        {
            meleeRecord.target = bestTarget;
            meleeRecord.index = target.handle;
            meleeRecord.simulationTime = currentSimulationTime;
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