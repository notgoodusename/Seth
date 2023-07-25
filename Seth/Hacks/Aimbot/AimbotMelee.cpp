#include "Aimbot.h"
#include "AimbotMelee.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"

#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

Vector getMeleeTarget(UserCmd* cmd, Entity* entity, matrix3x4 matrix[MAXSTUDIOBONES], Entity* activeWeapon, float& bestFov, Vector localPlayerEyePosition, bool mustBackstab = false, Vector eyeAngle = {}) noexcept
{
    const Model* model = entity->getModel();
    if (!model)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
    if (!hdr)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioHitboxSet* set = hdr->getHitboxSet(0);
    if (!set)
        return Vector{ 0.0f, 0.0f, 0.0f };

    StudioBbox* hitbox = set->getHitbox(mustBackstab ? Hitboxes::Spine1 : Hitboxes::Pelvis);
    if (!hitbox)
        return Vector{ 0.0f, 0.0f, 0.0f };

    for (auto& bonePosition : Aimbot::multiPoint(entity, matrix, hitbox, localPlayerEyePosition, 0, true))
    {
        const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles) };
        const float fov{ angle.length2D() };
        if (fov > bestFov)
            continue;

        if (mustBackstab && !Math::canBackstab(entity, cmd->viewangles + angle, eyeAngle))
            continue;

        if (!Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles + angle))
            continue;

        if (fov < bestFov) {
            bestFov = fov;
            return bonePosition;
        }
    }
    return Vector{ 0.0f, 0.0f, 0.0f };
}

void runKnife(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.melee;

    const auto enemies = TargetSystem::playerTargets(cfg.sortMethod);

    float bestSimulationTime{ -1.0f };
    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    for (const auto& target : enemies)
    {
        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        Vector backupEyeAngle = entity->eyeAngles();

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            auto records = &target.backtrackRecords;
            if (!records || records->empty() || records->size() <= 3U)
                continue;

            for (int i = static_cast<int>(records->size() - 1U); i >= 3; i--)
            {
                if (Backtrack::valid(records->at(i).simulationTime))
                {
                    memcpy(entity->getBoneCache().memory, records->at(i).matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                    memory->setAbsOrigin(entity, records->at(i).origin);
                    entity->eyeAngles() = records->at(i).eyeAngle;
                    memory->setCollisionBounds(entity->getCollideable(), records->at(i).mins, records->at(i).maxs);

                    bestTarget = getMeleeTarget(cmd, entity, entity->getBoneCache().memory, activeWeapon, bestFov, localPlayerEyePosition, cfg.autoBackstab, records->at(i).eyeAngle);
                    applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupPrescaledMins, backupPrescaledMaxs);

                    if (bestTarget.notNull())
                    {
                        bestSimulationTime = records->at(i).simulationTime;
                        break;
                    }
                }
            }
        }
        else
        {
            memcpy(entity->getBoneCache().memory, target.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, target.origin);
            entity->eyeAngles() = target.eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), target.mins, target.maxs);

            bestTarget = getMeleeTarget(cmd, entity, entity->getBoneCache().memory, activeWeapon, bestFov, localPlayerEyePosition, cfg.autoBackstab, target.eyeAngle);

            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupPrescaledMins, backupPrescaledMaxs);

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
    Vector eyeAngle;
    Vector target;
    Vector mins;
    Vector maxs;
    float simulationTime;
    matrix3x4* matrix;
} meleeRecord;

//TODO: Need optimizing
//Remove all distant players if not in reach, using an if check

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
            Vector backupEyeAngle = entity->eyeAngles();

            applyMatrix(entity, meleeRecord.matrix, meleeRecord.origin, meleeRecord.eyeAngle, meleeRecord.mins, meleeRecord.maxs);
            if (Math::doesMeleeHit(activeWeapon, meleeRecord.index, cmd->viewangles + angle))
            {
                cmd->viewangles += angle;
                cmd->tickCount = timeToTicks(meleeRecord.simulationTime + Backtrack::getLerp());

                if (!cfg.silent)
                    interfaces->engine->setViewAngles(cmd->viewangles);
            }
            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupPrescaledMins, backupPrescaledMaxs);
        }
    }

    if (!cfg.autoHit && !cfg.aimlock && !isAttacking(cmd, activeWeapon))
        return;

    if (!canAttack(cmd, activeWeapon))
        return;

    //The knife has no delay to register a hit
    if (activeWeapon->isKnife())
        return runKnife(activeWeapon, cmd);

    const auto enemies = TargetSystem::playerTargets(cfg.sortMethod);

    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    for (const auto& target : enemies)
    {
        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        Vector backupEyeAngle = entity->eyeAngles();

        float currentSimulationTime = -1.0f;

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            const auto records = &target.backtrackRecords;
            if (!records || records->empty() || records->size() <= 3U)
                continue;

            auto bestBacktrackDistance = FLT_MAX;
            int bestTick = -1;
            for (size_t i = 0; i < records->size(); i++)
            {
                //We gotta make sure if we fire now it will be correct when registering 0.2 seconds later and can register right now
                if (Backtrack::valid(records->at(i).simulationTime - 0.2121f) && Backtrack::valid(records->at(i).simulationTime))
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

            memcpy(entity->getBoneCache().memory, records->at(bestTick).matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, records->at(bestTick).origin);
            entity->eyeAngles() = records->at(bestTick).eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), records->at(bestTick).mins, records->at(bestTick).maxs);

            currentSimulationTime = records->at(bestTick).simulationTime;
        }
        else
        {
            memcpy(entity->getBoneCache().memory, target.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, target.origin);
            entity->eyeAngles() = target.eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), target.mins, target.maxs);

            currentSimulationTime = target.simulationTime;
        }

        meleeRecord.matrix = entity->getBoneCache().memory;
        meleeRecord.mins = entity->getCollideable()->obbMinsPreScaled();
        meleeRecord.maxs = entity->getCollideable()->obbMaxsPreScaled();
        meleeRecord.origin = entity->getAbsOrigin();
        meleeRecord.eyeAngle = entity->eyeAngles();

        bestTarget = getMeleeTarget(cmd, entity, entity->getBoneCache().memory, activeWeapon, bestFov, localPlayerEyePosition);
        applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupPrescaledMins, backupPrescaledMaxs);
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