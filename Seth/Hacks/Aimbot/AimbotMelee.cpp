#include "Aimbot.h"
#include "AimbotMelee.h"
#include "../Animations.h"
#include "../Backtrack.h"

#include "../../SDK/UserCmd.h"
#include "../../SDK/Vector.h"
#include "../../SDK/ModelInfo.h"

bool doesMeleeHit(Entity* activeWeapon, int index, const Vector angles) noexcept
{
    static Vector vecSwingMins(-18, -18, -18);
    static Vector vecSwingMaxs(18, 18, 18);

    if (!localPlayer)
        return false;

    float swingRange = activeWeapon->getSwingRange();
    if (swingRange <= 0.0f)
        return false;

    if (localPlayer->modelScale() > 1.0f)
        swingRange *= localPlayer->modelScale();

    Vector vecForward = Vector::fromAngle(angles);
    Vector vecSwingStart = localPlayer->getEyePosition();
    Vector vecSwingEnd = vecSwingStart + vecForward * swingRange;

    Trace trace;
    interfaces->engineTrace->traceRay({ vecSwingStart, vecSwingEnd, vecSwingMins, vecSwingMaxs }, MASK_SHOT, { localPlayer.get() }, trace);
    return trace.entity && trace.entity->index() == index;
}

bool canBackstab(Entity* entity, Vector angles, Vector entityAngles) noexcept
{
    Vector vecToTarget;
    vecToTarget = entity->getWorldSpaceCenter() - localPlayer->getWorldSpaceCenter();
    vecToTarget.z = 0.0f;
    vecToTarget = vecToTarget.normalized();

    // Get owner forward view vector
    Vector vecOwnerForward;
    Vector::fromAngleAll(angles, &vecOwnerForward, NULL, NULL);
    vecOwnerForward.z = 0.0f;
    vecOwnerForward = vecOwnerForward.normalized();

    // Get target forward view vector
    Vector vecTargetForward;
    Vector::fromAngleAll(entityAngles, &vecTargetForward, NULL, NULL);
    vecTargetForward.z = 0.0f;
    vecTargetForward = vecTargetForward.normalized();

    // Make sure owner is behind, facing and aiming at target's back
    float posVsTargetViewDot = vecToTarget.dotProduct(vecTargetForward);
    float posVsOwnerViewDot = vecToTarget.dotProduct(vecOwnerForward);
    float viewAnglesDot = vecTargetForward.dotProduct(vecOwnerForward);

    return (posVsTargetViewDot > 0.f && posVsOwnerViewDot > 0.5 && viewAnglesDot > -0.3f);
}

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
        const Vector angle{ calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles) };
        const float fov{ angle.length2D() };
        if (fov > bestFov)
            continue;

        if (mustBackstab && !canBackstab(entity, cmd->viewangles + angle, eyeAngle))
            continue;

        if (!doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles + angle))
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

    auto enemies = Aimbot::getEnemies();
    float bestSimulationTime{ -1.0f };
    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    for (const auto& target : enemies)
    {
        auto entity{ interfaces->entityList->getEntity(target.id) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        auto player = Animations::getPlayer(target.id);

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupMins = entity->getCollideable()->obbMins();
        Vector backupMaxs = entity->getCollideable()->obbMaxs();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        Vector backupEyeAngle = entity->eyeAngles();

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            auto records = Animations::getBacktrackRecords(entity->index());
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
                    applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);

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
            memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, player.origin);
            entity->eyeAngles() = player.eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

            bestTarget = getMeleeTarget(cmd, entity, entity->getBoneCache().memory, activeWeapon, bestFov, localPlayerEyePosition, cfg.autoBackstab, player.eyeAngle);

            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);

            if (bestTarget.notNull())
            {
                bestSimulationTime = player.simulationTime;
                break;
            }
        }

        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull())
    {
        const auto angle = calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles);

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
        const auto entity{ interfaces->entityList->getEntity(meleeRecord.index) };
        if (entity)
        {
            //We gotta recalculate to aim correctly
            const auto angle = calculateRelativeAngle(localPlayer->getEyePosition(), meleeRecord.target, cmd->viewangles);

            matrix3x4* backupBoneCache = entity->getBoneCache().memory;
            Vector backupMins = entity->getCollideable()->obbMins();
            Vector backupMaxs = entity->getCollideable()->obbMaxs();
            Vector backupOrigin = entity->getAbsOrigin();
            Vector backupAbsAngle = entity->getAbsAngle();
            Vector backupEyeAngle = entity->eyeAngles();

            applyMatrix(entity, meleeRecord.matrix, meleeRecord.origin, meleeRecord.eyeAngle, meleeRecord.mins, meleeRecord.maxs);
            if (doesMeleeHit(activeWeapon, meleeRecord.index, cmd->viewangles + angle))
            {
                cmd->viewangles += angle;
                cmd->tickCount = timeToTicks(meleeRecord.simulationTime + Backtrack::getLerp());

                if (!cfg.silent)
                    interfaces->engine->setViewAngles(cmd->viewangles);
            }
            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);
        }
    }

    if (!(cmd->buttons & UserCmd::IN_ATTACK || cfg.autoHit || cfg.aimlock))
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() && activeWeapon->nextSecondaryAttack() > memory->globalVars->serverTime())
        return;

    auto enemies = Aimbot::getEnemies();

    switch (cfg.sortMethod)
    {
    case 0:
        std::sort(enemies.begin(), enemies.end(), [&](const Aimbot::Enemy& a, const Aimbot::Enemy& b) { return a.distance < b.distance; });
        break;
    case 1:
        std::sort(enemies.begin(), enemies.end(), [&](const Aimbot::Enemy& a, const Aimbot::Enemy& b) { return a.fov < b.fov; });
        break;
    default:
        break;
    }

    //The knife has no delay to register a hit
    if (activeWeapon->isKnife())
        return runKnife(activeWeapon, cmd);

    auto bestFov = cfg.fov;
    Vector bestTarget{ };
    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    for (const auto& target : enemies)
    {
        auto entity{ interfaces->entityList->getEntity(target.id) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        auto player = Animations::getPlayer(target.id);

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupMins = entity->getCollideable()->obbMins();
        Vector backupMaxs = entity->getCollideable()->obbMaxs();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        Vector backupEyeAngle = entity->eyeAngles();

        for (int cycle = 0; cycle < 2; cycle++)
        {
            float currentSimulationTime = -1.0f;

            if (config->backtrack.enabled || config->backtrack.fakeLatency)
            {
                if (!cfg.targetBacktrack && cycle == 1)
                    continue;

                const auto records = Animations::getBacktrackRecords(entity->index());
                if (!records || records->empty() || records->size() <= 3U)
                    continue;

                int bestTick = -1;
                if (cycle == 0)
                {
                    for (size_t i = 3; i < records->size(); i++)
                    {
                        //We gotta make sure if we fire now it will be correct when registering 0.2 seconds later and can register right now
                        if (Backtrack::valid(records->at(i).simulationTime - 0.2121f) && Backtrack::valid(records->at(i).simulationTime))
                        {
                            bestTick = static_cast<int>(i);
                            break;
                        }
                    }
                }
                else
                {
                    auto bestBacktrackDistance = FLT_MAX;
                    for (int i = static_cast<int>(records->size() - 1U); i >= 3; i--)
                    {
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
                if (cycle == 1)
                    continue;

                memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, player.origin);
                entity->eyeAngles() = player.eyeAngle;
                memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

                currentSimulationTime = player.simulationTime;
            }

            meleeRecord.matrix = entity->getBoneCache().memory;
            meleeRecord.mins = entity->getCollideable()->obbMins();
            meleeRecord.maxs = entity->getCollideable()->obbMaxs();
            meleeRecord.origin = entity->getAbsOrigin();
            meleeRecord.eyeAngle = entity->eyeAngles();

            bestTarget = getMeleeTarget(cmd, entity, entity->getBoneCache().memory, activeWeapon, bestFov, localPlayerEyePosition);
            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);
            if (bestTarget.notNull())
            {
                meleeRecord.target = bestTarget;
                meleeRecord.index = target.id;
                meleeRecord.simulationTime = currentSimulationTime;
                meleeRecord.commandNumber = cmd->commandNumber + static_cast<int>(round(0.2121f / memory->globalVars->intervalPerTick));
                cmd->buttons |= UserCmd::IN_ATTACK;
                break;
            }
        }
        if (bestTarget.notNull())
            break;
    }
}

void AimbotMelee::reset() noexcept
{
    meleeRecord.commandNumber = -1;
}