#include "Aimbot/Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"
#include "Triggerbot.h"

#include "../Config.h"

#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Math.h"

bool getTriggerbotTarget(UserCmd* cmd, Entity* activeWeapon, Entity* entity, matrix3x4 matrix[MAXSTUDIOBONES], std::array<bool, Hitboxes::LeftUpperArm> hitbox, Vector startPos, Vector endPos, bool mustBackstab) noexcept
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

    if (mustBackstab && Math::canBackstab(entity, cmd->viewangles, entity->eyeAngles()))
        return Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles);

    for (size_t j = 0; j < hitbox.size(); j++)
    {
        if (!hitbox[j])
            continue;

        if (Math::hitboxIntersection(matrix, j, set, startPos, endPos))
        {
            Trace trace;
            interfaces->engineTrace->traceRay({ startPos, endPos }, MASK_SHOT | CONTENTS_HITBOX, TraceFilterSkipOne{ localPlayer.get() }, trace);
            if (!trace.entity)
                continue;

            return trace.entity == entity || trace.fraction > 0.97f;
        }
    }

    return false;
}

void Triggerbot::run(UserCmd* cmd) noexcept
{
    const auto& cfg = config->triggerbot;
    if (!config->triggerbotKey.isActive() || !cfg.enabled)
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponType = activeWeapon->getWeaponType();
    if (weaponType != WeaponType::HITSCAN && weaponType != WeaponType::MELEE)
        return;

    if (activeWeapon->nextPrimaryAttack() > memory->globalVars->serverTime())
        return;

    static auto lastTime = 0.0f;
    static auto lastContact = 0.0f;

    const auto now = memory->globalVars->realtime;

    if (now - lastTime < cfg.shotDelay / 1000.0f)
        return;

    std::array<bool, Hitboxes::LeftUpperArm> hitbox{ false };

    hitbox[Hitboxes::Head] = (cfg.hitboxes & 1 << 0) == 1 << 0; // Head

    hitbox[Hitboxes::Spine0] = (cfg.hitboxes & 1 << 1) == 1 << 1; //Body
    hitbox[Hitboxes::Spine1] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine2] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine3] = (cfg.hitboxes & 1 << 1) == 1 << 1;

    hitbox[Hitboxes::Pelvis] = (cfg.hitboxes & 1 << 2) == 1 << 2; //Pelvis


    //Yeah, this shit is hacky but it works well so who cares
    auto enemies = Aimbot::getEnemies();

    std::sort(enemies.begin(), enemies.end(), [&](const Aimbot::Enemy& a, const Aimbot::Enemy& b) { return a.fov < b.fov; });

    bool gotTarget = false;
    float bestSimulationTime = -1.0f;

    float range = 8192.0f;
    if (weaponType == WeaponType::MELEE)
    {
        range = activeWeapon->getSwingRange();

        if (localPlayer->modelScale() > 1.0f)
            range *= localPlayer->modelScale();
    }

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

        gotTarget = getTriggerbotTarget(cmd, activeWeapon, entity, entity->getBoneCache().memory, hitbox, startPos, endPos, activeWeapon->isKnife() && weaponType == WeaponType::MELEE);
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

void Triggerbot::updateInput() noexcept
{
	config->triggerbotKey.handleToggle();
}

void Triggerbot::reset() noexcept
{

}