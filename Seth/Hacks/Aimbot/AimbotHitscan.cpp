#include "Aimbot.h"
#include "AimbotHitscan.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"

#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

Vector getHitscanTarget(UserCmd* cmd, Entity* entity, matrix3x4 matrix[MAXSTUDIOBONES], std::array<bool, Hitboxes::LeftUpperArm> hitbox, float& bestFov, Vector localPlayerEyePosition) noexcept
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

    for (size_t j = 0; j < hitbox.size(); j++)
    {
        if (!hitbox[j])
            continue;

        StudioBbox* hitbox = set->getHitbox(j);
        if (!hitbox)
            continue;

        for (auto& bonePosition : Aimbot::multiPoint(entity, matrix, hitbox, localPlayerEyePosition, j))
        {
            const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles) };
            const float fov{ angle.length2D() };
            if (fov > bestFov)
                continue;

            if (!entity->isVisible(bonePosition))
                continue;

            if (fov < bestFov) {
                bestFov = fov;
                return bonePosition;
            }
        }
    }
    return Vector{ 0.0f, 0.0f, 0.0f };
}

void AimbotHitscan::run(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.hitscan;
    if (!cfg.enabled)
        return;

    if (!cfg.autoShoot && !cfg.aimlock && !isAttacking(cmd, activeWeapon))
        return;

    if (!canAttack(cmd, activeWeapon))
        return;

    std::array<bool, Hitboxes::LeftUpperArm> hitbox{ false };

    hitbox[Hitboxes::Head] = (cfg.hitboxes & 1 << 0) == 1 << 0; // Head

    hitbox[Hitboxes::Spine0] = (cfg.hitboxes & 1 << 1) == 1 << 1; //Body
    hitbox[Hitboxes::Spine1] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine2] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine3] = (cfg.hitboxes & 1 << 1) == 1 << 1;

    hitbox[Hitboxes::Pelvis] = (cfg.hitboxes & 1 << 2) == 1 << 2; //Pelvis

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    auto bestFov = cfg.fov;
    auto bestSimulationTime = -1.0f;
    Vector bestTarget{ };

    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    for (const auto& target : enemies)
    {
        if (target.playerData.empty() || !target.isAlive || target.priority == 0)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity || (entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = entity->getBoneCache().memory;
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();

        const auto& records = target.playerData;

        int bestTick = -1;
        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            auto bestBacktrackFov = cfg.fov;
            for (int i = static_cast<int>(records.size() - 1U); i >= 0; i--)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, targetTick.matrix[0].origin() , cmd->viewangles) };
                const float fov{ angle.length2D() };

                if (fov < bestBacktrackFov)
                {
                    bestTick = i;
                    bestBacktrackFov = fov;
                }
            }
        }
        else
        {
            bestTick = records.size() - 1U;
        }

        if (bestTick <= -1)
            continue;

        const auto& targetTick = records[bestTick];
        if (!Backtrack::valid(targetTick.simulationTime))
            continue;

        entity->replaceMatrix(targetTick.matrix.data());
        memory->setAbsOrigin(entity, targetTick.origin);
        memory->setAbsAngle(entity, targetTick.absAngle);
        memory->setCollisionBounds(entity->getCollideable(), targetTick.mins, targetTick.maxs);

        bestSimulationTime = targetTick.simulationTime;

        bestTarget = getHitscanTarget(cmd, entity, entity->getBoneCache().memory, hitbox, bestFov, localPlayerEyePosition);
        applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);
        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull())
    {
        auto angle = Math::calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles);

        if (cfg.autoShoot)
            cmd->buttons |= UserCmd::IN_ATTACK;

        if (!cfg.autoShoot)
            angle /= cfg.smooth;

        if (cmd->buttons & UserCmd::IN_ATTACK)
            cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());

        cmd->viewangles += angle;

        if (!cfg.silent)
            interfaces->engine->setViewAngles(cmd->viewangles);
    }
}

void AimbotHitscan::reset() noexcept
{

}