#include "Aimbot.h"
#include "AimbotHitscan.h"
#include "../Animations.h"
#include "../Backtrack.h"

#include "../../SDK/UserCmd.h"
#include "../../SDK/Vector.h"
#include "../../SDK/ModelInfo.h"

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
            const Vector angle{ calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles) };
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

    if (!(cmd->buttons & UserCmd::IN_ATTACK || cfg.autoShoot || cfg.aimlock))
        return;
    //TODO: Fix decloaking on both functions
    if (!activeWeapon->clip() || activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime() || activeWeapon->isInReload())
        return;

    switch (activeWeapon->itemDefinitionIndex())
    {
        case Sniper_m_TheMachina:
        case Sniper_m_ShootingStar:
        {
            if (!localPlayer->isScoped())
                return;
            break;
        }
        default:
            break;
    }

    std::array<bool, Hitboxes::LeftUpperArm> hitbox{ false };

    hitbox[Hitboxes::Head] = (cfg.hitboxes & 1 << 0) == 1 << 0; // Head

    hitbox[Hitboxes::Spine0] = (cfg.hitboxes & 1 << 1) == 1 << 1; //Body
    hitbox[Hitboxes::Spine1] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine2] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine3] = (cfg.hitboxes & 1 << 1) == 1 << 1;

    hitbox[Hitboxes::Pelvis] = (cfg.hitboxes & 1 << 2) == 1 << 2; //Pelvis

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

    auto bestFov = cfg.fov;
    auto bestSimulationTime = -1.0f;
    Vector bestTarget{ };
    const auto localPlayerEyePosition = localPlayer->getEyePosition();
    for (const auto& target : enemies)
    {
        const auto entity{ interfaces->entityList->getEntity(target.id) };
        if ((entity->isCloaked() && cfg.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !cfg.friendlyFire))
            continue;

        const auto& player = Animations::getPlayer(target.id);

        const auto& backupBoneCache = entity->getBoneCache().memory;
        const auto& backupMins = entity->getCollideable()->obbMins();
        const auto& backupMaxs = entity->getCollideable()->obbMaxs();
        const auto& backupOrigin = entity->getAbsOrigin();
        const auto& backupAbsAngle = entity->getAbsAngle();
        const auto& backupEyeAngle = entity->eyeAngles();

        for (int cycle = 0; cycle < 2; cycle++)
        {
            float currentSimulationTime = -1.0f;

            if (config->backtrack.enabled || config->backtrack.fakeLatency)
            {
                if (!cfg.targetBacktrack && cycle == 1)
                    continue;

                const auto records = Animations::getBacktrackRecords(entity->index());
                if (!records || records->empty())
                    continue;

                int bestTick = -1;
                if (cycle == 0)
                {
                    for (size_t i = 0; i < records->size(); i++)
                    {
                        if (Backtrack::valid(records->at(i).simulationTime))
                        {
                            bestTick = static_cast<int>(i);
                            break;
                        }
                    }
                }
                else
                {
                    auto bestBacktrackFov = 255.0f;
                    for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
                    {
                        if (Backtrack::valid(records->at(i).simulationTime))
                        {
                            const Vector angle{ calculateRelativeAngle(localPlayerEyePosition, records->at(i).matrix[0].origin() , cmd->viewangles) };
                            const float fov{ angle.length2D() };

                            if (fov < bestBacktrackFov)
                            {
                                bestTick = i;
                                bestBacktrackFov = fov;
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

                bestSimulationTime = records->at(bestTick).simulationTime;
            }
            else
            {
                if (cycle == 1)
                    continue;

                memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, player.origin);
                entity->eyeAngles() = player.eyeAngle;
                memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

                bestSimulationTime = player.simulationTime;
            }

            bestTarget = getHitscanTarget(cmd, entity, entity->getBoneCache().memory, hitbox, bestFov, localPlayerEyePosition);
            applyMatrix(entity, backupBoneCache, backupOrigin, backupEyeAngle, backupMins, backupMaxs);
            if (bestTarget.notNull())
                break;
        }
        if (bestTarget.notNull())
            break;
    }

    if (bestTarget.notNull())
    {
        auto angle = calculateRelativeAngle(localPlayerEyePosition, bestTarget, cmd->viewangles);

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