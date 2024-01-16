#include "../Aimbot/Aimbot.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"
#include "Triggerbot.h"
#include "TriggerbotMelee.h"

#include "../../Config.h"

#include "../../SDK/Entity.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"

bool getTriggerbotMeleeTarget(UserCmd* cmd, Entity* activeWeapon, Entity* entity, bool mustBackstab, Vector entityWorldSpaceCenter) noexcept
{
    const Model* model = entity->getModel();
    if (!model)
        return false;

    StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
    if (!hdr)
        return false;

    StudioHitboxSet* set = hdr->getHitboxSet(0);
    if (!set)
        return false;
    
    if (mustBackstab)
        return Math::canBackstab(cmd->viewangles, entity->eyeAngles(), entityWorldSpaceCenter)
        && Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles);

    return Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles);
}

void TriggerbotMelee::run(Entity* activeWeapon, UserCmd* cmd, float& lastTime, float& lastContact) noexcept
{
    const auto& cfg = config->meleeTriggerbot;
    if (!cfg.enabled)
        return;

    const auto now = memory->globalVars->realTime;

    if (now - lastTime < cfg.shotDelay / 1000.0f)
        return;

    const auto& enemies = TargetSystem::playerTargets(SortType::Fov);

    bool gotTarget = false;
    float bestSimulationTime = -1.0f;

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

        Vector worldSpaceCenter = entity->getWorldSpaceCenter();

        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            const auto& records = target.backtrackRecords;
            if (records.empty() || records.size() <= 3U)
                continue;

            int bestTick = -1;
            auto bestFov{ 255.f };

            for (int i = static_cast<int>(records.size() - 1U); i >= 0; i--)
            {
                if (!Backtrack::valid(records[i].simulationTime))
                    continue;

                for (const auto& position : records[i].bodyPositions)
                {
                    const auto angle = Math::calculateRelativeAngle(localPlayerEyePosition, position, cmd->viewangles);
                    const auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTick = i;
                    }
                }
            }

            if (bestTick <= -1)
            {
                applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);
                continue;
            }

            memcpy(entity->getBoneCache().memory, records[bestTick].matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, records[bestTick].origin);
            memory->setAbsAngle(entity, records[bestTick].absAngle);
            memory->setCollisionBounds(entity->getCollideable(), records[bestTick].mins, records[bestTick].maxs);

            worldSpaceCenter = records[bestTick].worldSpaceCenter;
            bestSimulationTime = records[bestTick].simulationTime;
        }
        else
        {
            memcpy(entity->getBoneCache().memory, target.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, target.origin);
            memory->setAbsAngle(entity, target.absAngle);
            memory->setCollisionBounds(entity->getCollideable(), target.mins, target.maxs);

            worldSpaceCenter = target.worldSpaceCenter;
            bestSimulationTime = target.simulationTime;
        }

        gotTarget = getTriggerbotMeleeTarget(cmd, activeWeapon, entity, activeWeapon->isKnife() && cfg.autoBackstab, worldSpaceCenter);
        applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupPrescaledMins, backupPrescaledMaxs);
        if (gotTarget)
            break;
    }
    if (gotTarget)
    {
        cmd->buttons |= UserCmd::IN_ATTACK;
        cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());
        lastTime = 0.0f;
        lastContact = now;
    }
}