#include "Aimbot.h"
#include "Animations.h"
#include "Backtrack.h"

#include "../SDK/UserCmd.h"
#include "../SDK/Vector.h"
#include "../SDK/ModelInfo.h"

std::vector<Aimbot::Enemy> enemies;

std::vector<Vector> multiPoint(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox, Vector localEyePos, int _hitbox) noexcept
{
    auto VectorTransformWrapper = [](const Vector& in1, const matrix3x4 in2, Vector& out)
    {
        auto VectorTransform = [](const float* in1, const matrix3x4 in2, float* out)
        {
            auto dotProducts = [](const float* v1, const float* v2)
            {
                return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
            };
            out[0] = dotProducts(in1, in2[0]) + in2[0][3];
            out[1] = dotProducts(in1, in2[1]) + in2[1][3];
            out[2] = dotProducts(in1, in2[2]) + in2[2][3];
        };
        VectorTransform(&in1.x, in2, &out.x);
    };

    Vector min, max, center;
    VectorTransformWrapper(hitbox->bbMin, matrix[hitbox->bone], min);
    VectorTransformWrapper(hitbox->bbMax, matrix[hitbox->bone], max);
    center = (min + max) * 0.5f;

    std::vector<Vector> vecArray;

    vecArray.emplace_back(center);

    Vector currentAngles = calculateRelativeAngle(center, localEyePos, Vector{});

    Vector forward;
    Vector::fromAngle(currentAngles, &forward);

    Vector right = forward.cross(Vector{ 0, 0, 1 });
    Vector left = Vector{ -right.x, -right.y, right.z };

    Vector top = Vector{ 0, 0, 1 };
    Vector bottom = Vector{ 0, 0, -1 };

    float multiPoint = 50.0f * 0.01f;

    switch (_hitbox)
    {
    case Hitboxes::Head:
        for (auto i = 0; i < 4; ++i)
            vecArray.emplace_back(center);

        vecArray[1] += top * multiPoint;
        vecArray[2] += right * multiPoint;
        vecArray[3] += left * multiPoint;
        break;
    default://rest
        for (auto i = 0; i < 3; ++i)
            vecArray.emplace_back(center);

        vecArray[1] += right * multiPoint;
        vecArray[2] += left * multiPoint;
        break;
    }
    return vecArray;
}

bool canShoot() noexcept
{
    if (!localPlayer)
        return false;

    Entity* activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return false;

    static float lastFire = 0, nextAttack = 0, a = 0;
    static Entity* oldActiveWeapon;

    if (lastFire != activeWeapon->lastFireTime() || activeWeapon != oldActiveWeapon)
    {
        lastFire = activeWeapon->lastFireTime();
        nextAttack = activeWeapon->nextPrimaryAttack();
    }

    if (!activeWeapon->clip() && activeWeapon->isInReload())
        return activeWeapon->nextPrimaryAttack() <= memory->globalVars->serverTime();

    if (activeWeapon->isInReload())
        return true;

    if (!activeWeapon->clip())
        return false;

    oldActiveWeapon = activeWeapon;

    return nextAttack <= memory->globalVars->serverTime();
}

void Aimbot::updateInput() noexcept
{
    config->aimbotKey.handleToggle();
}

void Aimbot::run(UserCmd* cmd) noexcept
{
    canShoot();
    if (!config->aimbotKey.isActive() || 
        (!config->aimbot.hitscan.enabled && !config->aimbot.projectile.enabled && !config->aimbot.melee.enabled))
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    enemies.clear();

    const auto localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto localPlayerEyePosition = localPlayer->getEyePosition();
    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto player = Animations::getPlayer(i);
        if (!player.gotMatrix)
            continue;

        const auto entity{ interfaces->entityList->getEntity(i) };
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
            continue;

        const auto angle{ calculateRelativeAngle(localPlayerEyePosition, player.matrix[6].origin(), cmd->viewangles) };
        const auto fov{ angle.length2D() }; //fov
        const auto health{ entity->health() }; //health
        const auto distance{ localPlayerOrigin.distTo(entity->getAbsOrigin()) }; //distance
        enemies.emplace_back(i, health, distance, fov);
    }

    if (enemies.empty())
        return;

    const auto weaponType = activeWeapon->getWeaponType();
    switch (weaponType)
    {
        case WeaponType::UNKNOWN:
            return;
        case WeaponType::HITSCAN:
            Aimbot::runHitscan(activeWeapon, cmd);
            return;
        case WeaponType::PROJECTILE:
            Aimbot::runProjectile(activeWeapon, cmd);
            return;
        case WeaponType::MELEE:
            Aimbot::runMelee(activeWeapon, cmd);
            return;
        case WeaponType::THROWABLE:
            return;
        default:
            break;
    }
}

Vector getHitscanTarget( UserCmd* cmd, Entity* entity, matrix3x4 matrix[MAXSTUDIOBONES], std::array<bool, Hitboxes::Max> hitbox, float& bestFov, Vector localPlayerEyePosition) noexcept
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

        for (auto& bonePosition : multiPoint(entity, matrix, hitbox, localPlayerEyePosition, j))
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

void Aimbot::runHitscan(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.hitscan;
    if (!cfg.enabled)
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

    if (!canShoot())
        return;

    if (!(cmd->buttons & UserCmd::IN_ATTACK || cfg.autoShoot || cfg.aimlock))
        return;

    std::array<bool, Hitboxes::Max> hitbox{ false };

    hitbox[Hitboxes::Head] = (cfg.hitboxes & 1 << 0) == 1 << 0; // Head

    hitbox[Hitboxes::Spine0] = (cfg.hitboxes & 1 << 1) == 1 << 1; //Body
    hitbox[Hitboxes::Spine1] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine2] = (cfg.hitboxes & 1 << 1) == 1 << 1;
    hitbox[Hitboxes::Spine3] = (cfg.hitboxes & 1 << 1) == 1 << 1;

    hitbox[Hitboxes::Pelvis] = (cfg.hitboxes & 1 << 2) == 1 << 2; //Pelvis

    hitbox[Hitboxes::RightUpperArm] = (cfg.hitboxes & 1 << 3) == 1 << 3; //Arms
    hitbox[Hitboxes::RightForearm] = (cfg.hitboxes & 1 << 3) == 1 << 3;
    hitbox[Hitboxes::LeftUpperArm] = (cfg.hitboxes & 1 << 3) == 1 << 3;
    hitbox[Hitboxes::LeftForearm] = (cfg.hitboxes & 1 << 3) == 1 << 3;

    hitbox[Hitboxes::LeftHip] = (cfg.hitboxes & 1 << 4) == 1 << 4; //Legs
    hitbox[Hitboxes::RightHip] = (cfg.hitboxes & 1 << 4) == 1 << 4;
    hitbox[Hitboxes::LeftKnee] = (cfg.hitboxes & 1 << 4) == 1 << 4;
    hitbox[Hitboxes::RightKnee] = (cfg.hitboxes & 1 << 4) == 1 << 4;

    switch (cfg.sortMethod)
    {
    case 0:
        std::sort(enemies.begin(), enemies.end(), [&](const Enemy& a, const Enemy& b) { return a.distance < b.distance; });
        break;
    case 1:
        std::sort(enemies.begin(), enemies.end(), [&](const Enemy& a, const Enemy& b) { return a.fov < b.fov; });
        break;
    case 2:
        std::sort(enemies.begin(), enemies.end(), [&](const Enemy& a, const Enemy& b) { return a.health < b.health; });
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
        if ((entity->isCloaked() && config->aimbot.hitscan.ignoreCloaked) || (!entity->isEnemy(localPlayer.get()) && !config->aimbot.hitscan.friendlyFire))
            continue;

        const auto& player = Animations::getPlayer(target.id);
        const auto& backupBoneCache = entity->getBoneCache().memory;
        const auto& backupMins = entity->getCollideable()->obbMins();
        const auto& backupMaxs = entity->getCollideable()->obbMaxs();
        const auto& backupOrigin = entity->getAbsOrigin();
        const auto& backupAbsAngle = entity->getAbsAngle();

        for (int cycle = 0; cycle < 2; cycle++)
        {
            float currentSimulationTime = -1.0f;

            if (config->backtrack.enabled)
            {
                if (!config->aimbot.hitscan.targetBacktrack && cycle == 1)
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
                    for (int i = static_cast<int>(records->size() - 1U); i >= 0; i--)
                    {
                        if (Backtrack::valid(records->at(i).simulationTime))
                        {
                            bestTick = i;
                            break;
                        }
                    }
                }

                if (bestTick <= -1)
                    continue;

                memcpy(entity->getBoneCache().memory, records->at(bestTick).matrix, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, records->at(bestTick).origin);
                memory->setAbsAngle(entity, Vector{ 0.f, records->at(bestTick).absAngle.y, 0.f });
                memory->setCollisionBounds(entity->getCollideable(), records->at(bestTick).mins, records->at(bestTick).maxs);

                currentSimulationTime = records->at(bestTick).simulationTime;
            }
            else
            {
                if (cycle == 1)
                    continue;

                memcpy(entity->getBoneCache().memory, player.matrix.data(), std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                memory->setAbsOrigin(entity, player.origin);
                memory->setAbsAngle(entity, Vector{ 0.f, player.absAngle.y, 0.f });
                memory->setCollisionBounds(entity->getCollideable(), player.mins, player.maxs);

                currentSimulationTime = player.simulationTime;
            }

            bestTarget = getHitscanTarget(cmd, entity, entity->getBoneCache().memory, hitbox, bestFov, localPlayerEyePosition);
            applyMatrix(entity, backupBoneCache, backupOrigin, backupAbsAngle, backupMins, backupMaxs);
            if (bestTarget.notNull())
            {
                bestSimulationTime = currentSimulationTime;
                break;
            }
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
        cmd->viewangles += angle;

        if (!cfg.silent)
            interfaces->engine->setViewAngles(cmd->viewangles);

        if (cmd->buttons & UserCmd::IN_ATTACK)
            cmd->tickCount = timeToTicks(bestSimulationTime + Backtrack::getLerp());
    }
}

void Aimbot::runProjectile(Entity* activeWeapon, UserCmd* cmd) noexcept
{

}

void Aimbot::runMelee(Entity* activeWeapon, UserCmd* cmd) noexcept
{

}