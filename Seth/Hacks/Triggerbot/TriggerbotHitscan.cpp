#include "../Aimbot/Aimbot.h"
#include "../Backtrack.h"
#include "../TargetSystem.h"
#include "Triggerbot.h"
#include "TriggerbotHitscan.h"

#include "../../Config.h"

#include "../../SDK/Entity.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"

bool getTriggerbotHitscanTarget(UserCmd* cmd, Entity* activeWeapon, Entity* entity, const matrix3x4* matrix, 
    std::array<bool, Hitboxes::LeftUpperArm> hitbox, 
    Vector startPos, Vector endPos, StudioBbox* bestHitbox, bool friendlyFire) noexcept
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

    for (size_t j = 0; j < hitbox.size(); j++)
    {
        if (!hitbox[j])
            continue;

        if (Math::hitboxIntersection(matrix, j, set, startPos, endPos))
        {
            Trace trace;
            if(friendlyFire)
                interfaces->engineTrace->traceRay({ startPos, endPos }, MASK_SHOT | CONTENTS_HITBOX, TraceFilterHitscan{ localPlayer.get() }, trace);
            else
                interfaces->engineTrace->traceRay({ startPos, endPos }, MASK_SHOT | CONTENTS_HITBOX, TraceFilterHitscanIgnoreTeammates{ localPlayer.get() }, trace);

            if (!trace.entity)
                continue;

            bestHitbox = set->getHitbox(j);

            return trace.entity == entity || trace.fraction > 0.97f;
        }
    }

    return false;
}

void TriggerbotHitscan::run(Entity* activeWeapon, UserCmd* cmd, float& lastTime, float& lastContact) noexcept
{
    const auto& cfg = config->hitscanTriggerbot;
    if (!cfg.enabled)
        return;

    const auto now = memory->globalVars->realTime;

    if (now - lastTime < cfg.shotDelay / 1000.0f)
        return;

    if (cfg.scopedOnly && activeWeapon->isSniper() && !localPlayer->isScoped())
        return;

    std::array<bool, Hitboxes::LeftUpperArm> hitbox{ false };

    const auto canWeaponHeadshot = activeWeapon->canWeaponHeadshot();
    if ((cfg.hitboxes & 1 << 2) == 1 << 2)
    {
        if (canWeaponHeadshot)
        {
            hitbox[Hitboxes::Head] = true;
        }
        else
        {
            hitbox[Hitboxes::Spine0] = true;
            hitbox[Hitboxes::Spine1] = true;
            hitbox[Hitboxes::Spine2] = true;
            hitbox[Hitboxes::Spine3] = true;
        }
    }
    else
    {
        hitbox[Hitboxes::Head] = (cfg.hitboxes & 1 << 0) == 1 << 0; // Head

        hitbox[Hitboxes::Spine0] = (cfg.hitboxes & 1 << 1) == 1 << 1; //Body
        hitbox[Hitboxes::Spine1] = (cfg.hitboxes & 1 << 1) == 1 << 1;
        hitbox[Hitboxes::Spine2] = (cfg.hitboxes & 1 << 1) == 1 << 1;
        hitbox[Hitboxes::Spine3] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    }

    //Yeah, this shit is hacky but it works well so who cares
    const auto& enemies = TargetSystem::playerTargets(SortType::Fov);

    bool gotTarget = false;
    float bestSimulationTime = -1.0f;
    matrix3x4* matrix = nullptr;
    StudioBbox* bestHitbox = nullptr;

    float range = 8192.0f;

    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    const auto startPos = localPlayerEyePosition;
    const auto endPos = startPos + Vector::fromAngle(cmd->viewangles) * range;

    const bool ignoreCloaked = (cfg.ignore & 1 << 0) == 1 << 0;
    const bool ignoreInvulnerable = (cfg.ignore & 1 << 1) == 1 << 1;
    for (const auto& target : enemies)
    {
        if (target.playerData.empty() || !target.isAlive || target.priority == 0)
            continue;

        auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity ||
            (entity->isCloaked() && ignoreCloaked) ||
            (entity->isInvulnerable() && ignoreInvulnerable) ||
            (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        matrix3x4* backupBoneCache = new matrix3x4[MAXSTUDIOBONES];
        memcpy(backupBoneCache, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
        Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
        Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
        Vector backupOrigin = entity->getAbsOrigin();
        Vector backupAbsAngle = entity->getAbsAngle();
        Vector backupEyeAngle = entity->eyeAngles();

        const auto& records = target.playerData;

        int bestTick = -1;
        if ((config->backtrack.enabled || config->backtrack.fakeLatency) && cfg.targetBacktrack)
        {
            auto bestFov{ 255.f };
            for (int i = 0; i < static_cast<int>(records.size()); i++)
            {
                const auto& targetTick = records[i];
                if (!Backtrack::valid(targetTick.simulationTime))
                    continue;

                //if head is enabled do head
                if ((cfg.hitboxes & 1 << 0) == 1 << 0 || ((cfg.hitboxes & 1 << 2) == 1 << 2 && canWeaponHeadshot))
                {
                    for (const auto& position : targetTick.headPositions)
                    {
                        const auto angle = Math::calculateRelativeAngle(startPos, position, cmd->viewangles);
                        const auto fov = std::hypotf(angle.x, angle.y);
                        if (fov < bestFov) {
                            bestFov = fov;
                            bestTick = i;
                        }
                    }
                }
                
                //and if body is also enabled do it too
                if ((cfg.hitboxes & 1 << 1) == 1 << 1 || ((cfg.hitboxes & 1 << 2) == 1 << 2 && !canWeaponHeadshot))
                {
                    for (const auto& position : targetTick.headPositions)
                    {
                        const auto angle = Math::calculateRelativeAngle(startPos, position, cmd->viewangles);
                        const auto fov = std::hypotf(angle.x, angle.y);
                        if (fov < bestFov) {
                            bestFov = fov;
                            bestTick = i;
                        }
                    }
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

        gotTarget = getTriggerbotHitscanTarget(cmd, activeWeapon, entity, targetTick.matrix.data(), hitbox, startPos, endPos, bestHitbox, cfg.friendlyFire);
        if (gotTarget)
            matrix = entity->getBoneCache().memory;
 
        entity->replaceMatrix(backupBoneCache);
        memory->setAbsOrigin(entity, backupOrigin);
        memory->setAbsAngle(entity, backupAbsAngle);
        memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);

        delete[] backupBoneCache;

        if (gotTarget)
            break;
    }

    if (gotTarget)
    {
        cmd->buttons |= UserCmd::IN_ATTACK;
        cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());

        if (cfg.magnet && matrix && bestHitbox)
        {
            const auto centerHitbox = Math::getCenterOfHitbox(matrix, bestHitbox);
            if (centerHitbox.notNull())
            {
                const auto angle = Math::calculateRelativeAngle(startPos, centerHitbox, cmd->viewangles);

                cmd->viewangles += angle;
                interfaces->engine->setViewAngles(cmd->viewangles);
            }
        }

        lastTime = 0.0f;
        lastContact = now;
    }
}