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

    const bool mustBackstab = activeWeapon->isKnife() && cfg.autoBackstab;

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
            auto bestBacktrackFov{ 255.f };

            for (int i = 0; i < static_cast<int>(records.size()); i++)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                Vector worldSpaceCenter = targetTick.origin;
                worldSpaceCenter.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

                if (mustBackstab 
                    && !Math::canBackstab(cmd->viewangles, targetTick.eyeAngle, worldSpaceCenter))
                    continue;

                const auto angle = Math::calculateRelativeAngle(localPlayerEyePosition, worldSpaceCenter, cmd->viewangles);
                const auto fov = std::hypotf(angle.x, angle.y);
                if (fov < bestBacktrackFov) {
                    bestBacktrackFov = fov;
                    bestTick = i;
                }
            }
        }
        else
        {
            for (int i = static_cast<int>(records.size() - 1U); i >= 0; i--)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                bestTick = i;
            }
        }

        if (bestTick <= -1)
            continue;

        const auto& targetTick = target.playerData[bestTick];

        entity->replaceMatrix(targetTick.matrix.data());
        memory->setAbsOrigin(entity, targetTick.origin);
        memory->setAbsAngle(entity, targetTick.absAngle);
        memory->setCollisionBounds(entity->getCollideable(), targetTick.minsPrescaled, targetTick.maxsPrescaled);

        bestSimulationTime = targetTick.simulationTime;

        Vector worldSpaceCenter = targetTick.origin;
        worldSpaceCenter.z += (targetTick.mins.z + targetTick.maxs.z) * 0.5f;

        gotTarget = getTriggerbotMeleeTarget(cmd, activeWeapon, entity, mustBackstab, worldSpaceCenter);
        
        entity->replaceMatrix(backupBoneCache);
        memory->setAbsOrigin(entity, backupOrigin);
        memory->setAbsAngle(entity, backupAbsAngle);
        memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);
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