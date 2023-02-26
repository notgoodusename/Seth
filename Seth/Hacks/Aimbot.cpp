#include "Aimbot.h"
#include "Animations.h"

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

void Aimbot::runHitscan(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.hitscan;
    if (!cfg.enabled)
        return;

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
    Vector bestTarget{ };
    const auto localPlayerEyePosition = localPlayer->getEyePosition();
    for (const auto& target : enemies)
    {
        const auto entity{ interfaces->entityList->getEntity(target.id) };
        if (entity->isCloaked() && config->aimbot.hitscan.ignoreCloaked)
            continue;
        const auto player = Animations::getPlayer(target.id);
        const Model* model = entity->getModel();
        if (!model)
            continue;

        StudioHdr* hdr = interfaces->modelInfo->getStudioModel(model);
        if (!hdr)
            continue;

        StudioHitboxSet* set = hdr->getHitboxSet(0);
        if (!set)
            continue;

        for (size_t j = 0; j < hitbox.size(); j++)
        {
            if (!hitbox[j])
                continue;

            StudioBbox* hitbox = set->getHitbox(j);
            if (!hitbox)
                continue;

            for (auto& bonePosition : multiPoint(entity, player.matrix.data(), hitbox, localPlayerEyePosition, j))
            {
                const Vector angle{ calculateRelativeAngle(localPlayerEyePosition, bonePosition, cmd->viewangles) };
                const float fov{ angle.length2D() };
                if (fov > bestFov)
                    continue;

                if (!entity->isVisible(bonePosition))
                    continue;

                if (fov < bestFov) {
                    bestFov = fov;
                    bestTarget = bonePosition;
                    break;
                }
            }
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
        cmd->viewangles += angle;

        if (!cfg.silent)
            interfaces->engine->setViewAngles(cmd->viewangles);
    }
}

void Aimbot::runProjectile(Entity* activeWeapon, UserCmd* cmd) noexcept
{

}

void Aimbot::runMelee(Entity* activeWeapon, UserCmd* cmd) noexcept
{

}