#include "../Aimbot/Aimbot.h"
#include "../Animations.h"
#include "../Backtrack.h"
#include "Triggerbot.h"
#include "TriggerbotMelee.h"

#include "../../Config.h"

#include "../../SDK/Entity.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"

bool getTriggerbotMeleeTarget(UserCmd* cmd, Entity* activeWeapon, Entity* entity, bool mustBackstab) noexcept
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
        return Math::canBackstab(entity, cmd->viewangles, entity->eyeAngles()) && Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles);

    return Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles);
}

void TriggerbotMelee::run(Entity* activeWeapon, UserCmd* cmd, float& lastTime, float& lastContact) noexcept
{
    const auto& cfg = config->meleeTriggerbot;
    if (!cfg.enabled)
        return;

    const auto now = memory->globalVars->realtime;

    if (now - lastTime < cfg.shotDelay / 1000.0f)
        return;

    //Yeah, this shit is hacky but it works well so who cares
    auto enemies = Aimbot::getEnemies();

    std::sort(enemies.begin(), enemies.end(), [&](const Aimbot::Enemy& a, const Aimbot::Enemy& b) { return a.fov < b.fov; });

    bool gotTarget = false;
    float bestSimulationTime = -1.0f;

    float range = activeWeapon->getSwingRange();;
    if (localPlayer->modelScale() > 1.0f)
        range *= localPlayer->modelScale();

    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    const auto startPos = localPlayerEyePosition;
    const auto endPos = startPos + Vector::fromAngle(cmd->viewangles) * range;
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

            int bestTick = -1;
            auto bestFov{ 255.f };

            for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
            {
                if (!Backtrack::valid(records->at(i).simulationTime))
                    continue;

                for (auto& position : records->at(i).positions)
                {
                    const auto angle = Math::calculateRelativeAngle(startPos, position, cmd->viewangles);
                    const auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTick = i;
                    }
                }
            }

            if (bestTick <= -1)
                continue;

            memcpy(entity->getBoneCache().memory, records->at(bestTick).matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, records->at(bestTick).origin);
            entity->eyeAngles() = records->at(bestTick).eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), records->at(bestTick).mins, records->at(bestTick).maxs);

            bestSimulationTime = records->at(bestTick).simulationTime;
        }
        else
        {
            memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
            memory->setAbsOrigin(entity, player.origin);
            entity->eyeAngles() = player.eyeAngle;
            memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

            bestSimulationTime = player.simulationTime;
        }

        gotTarget = getTriggerbotMeleeTarget(cmd, activeWeapon, entity, activeWeapon->isKnife() && cfg.autoBackstab);
        applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);
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