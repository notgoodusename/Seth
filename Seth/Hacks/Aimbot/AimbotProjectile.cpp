#include "Aimbot.h"
#include "AimbotProjectile.h"
#include "../Backtrack.h"
#include "../MovementRebuild.h"
#include "../ProjectileSimulation.h"
#include "../TargetSystem.h"

#include "../../SDK/AttributeManager.h"
#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Physics.h"
#include "../../SDK/Vector.h"

Vector getProjectileWeaponAimOffset(Entity* weapon, Entity* entity) noexcept
{
    Vector offset{ 0.0f, 0.0f, 0.0f };
    switch (weapon->itemDefinitionIndex())
    {
    case Soldier_m_TheDirectHit:
    case Soldier_m_RocketLauncher:
    case Soldier_m_RocketLauncherR:
    case Soldier_m_TheBlackBox:
    case Soldier_m_TheCowMangler5000:
    case Soldier_m_TheOriginal:
    case Soldier_m_FestiveRocketLauncher:
    case Soldier_m_TheBeggarsBazooka:
    case Soldier_m_SilverBotkillerRocketLauncherMkI:
    case Soldier_m_GoldBotkillerRocketLauncherMkI:
    case Soldier_m_RustBotkillerRocketLauncherMkI:
    case Soldier_m_BloodBotkillerRocketLauncherMkI:
    case Soldier_m_CarbonadoBotkillerRocketLauncherMkI:
    case Soldier_m_DiamondBotkillerRocketLauncherMkI:
    case Soldier_m_SilverBotkillerRocketLauncherMkII:
    case Soldier_m_GoldBotkillerRocketLauncherMkII:
    case Soldier_m_FestiveBlackBox:
    case Soldier_m_TheAirStrike:
    case Soldier_m_WoodlandWarrior:
    case Soldier_m_SandCannon:
    case Soldier_m_AmericanPastoral:
    case Soldier_m_SmalltownBringdown:
    case Soldier_m_ShellShocker:
    case Soldier_m_AquaMarine:
    case Soldier_m_Autumn:
    case Soldier_m_BlueMew:
    case Soldier_m_BrainCandy:
    case Soldier_m_CoffinNail:
    case Soldier_m_HighRollers:
    case Soldier_m_Warhawk:
    case Soldier_m_TheLibertyLauncher:
        break; //We want to hit feet with soldier so we can airshot
    case Soldier_s_TheRighteousBison:
    case Engi_m_ThePomson6000:
    case Pyro_m_DragonsFury:
    case Pyro_m_FlameThrower:
    case Pyro_m_FlameThrowerR:
    case Pyro_m_TheBackburner:
    case Pyro_m_TheDegreaser:
    case Pyro_m_ThePhlogistinator:
    case Pyro_m_FestiveFlameThrower:
    case Pyro_m_TheRainblower:
    case Pyro_m_SilverBotkillerFlameThrowerMkI:
    case Pyro_m_GoldBotkillerFlameThrowerMkI:
    case Pyro_m_RustBotkillerFlameThrowerMkI:
    case Pyro_m_BloodBotkillerFlameThrowerMkI:
    case Pyro_m_CarbonadoBotkillerFlameThrowerMkI:
    case Pyro_m_DiamondBotkillerFlameThrowerMkI:
    case Pyro_m_SilverBotkillerFlameThrowerMkII:
    case Pyro_m_GoldBotkillerFlameThrowerMkII:
    case Pyro_m_FestiveBackburner:
    case Pyro_m_ForestFire:
    case Pyro_m_BarnBurner:
    case Pyro_m_BovineBlazemaker:
    case Pyro_m_EarthSkyandFire:
    case Pyro_m_FlashFryer:
    case Pyro_m_TurbineTorcher:
    case Pyro_m_Autumn:
    case Pyro_m_PumpkinPatch:
    case Pyro_m_Nutcracker:
    case Pyro_m_Balloonicorn:
    case Pyro_m_Rainbow:
    case Pyro_m_CoffinNail:
    case Pyro_m_Warhawk:
    case Pyro_m_NostromoNapalmer:
    case Pyro_s_TheDetonator:
    case Pyro_s_TheFlareGun:
    case Pyro_s_FestiveFlareGun:
    case Pyro_s_TheScorchShot:
    case Pyro_s_TheManmelter:
    case Medic_m_SyringeGun:
    case Medic_m_SyringeGunR:
    case Medic_m_TheBlutsauger:
    case Medic_m_TheOverdose:
    case Engi_m_TheRescueRanger:
    case Medic_m_FestiveCrusadersCrossbow:
    case Medic_m_CrusadersCrossbow:

    case Scout_t_TheSandman:
    case Scout_t_TheWrapAssassin:
    case Scout_s_TheFlyingGuillotine:
    case Scout_s_TheFlyingGuillotineG:
        //Center of the hitbox
        offset.z = (entity->obbMaxs().z - entity->obbMins().z) * 0.5f;
        break;

    case Demoman_m_GrenadeLauncher:
    case Demoman_m_GrenadeLauncherR:
    case Demoman_m_FestiveGrenadeLauncher:
    case Demoman_m_TheIronBomber:
    case Demoman_m_Autumn:
    case Demoman_m_MacabreWeb:
    case Demoman_m_Rainbow:
    case Demoman_m_SweetDreams:
    case Demoman_m_CoffinNail:
    case Demoman_m_TopShelf:
    case Demoman_m_Warhawk:
    case Demoman_m_ButcherBird:
    case Demoman_m_TheLochnLoad:
    case Demoman_m_TheLooseCannon:
    case Demoman_s_StickybombLauncher:
    case Demoman_s_StickybombLauncherR:
    case Demoman_s_FestiveStickybombLauncher:
    case Demoman_s_TheScottishResistance:
    case Demoman_s_TheQuickiebombLauncher:
        //Center of the hitbox
        offset.z = (entity->obbMaxs().z - entity->obbMins().z) * 0.2f;
        break;

    case Sniper_m_TheHuntsman:
    case Sniper_m_FestiveHuntsman:
    case Sniper_m_TheFortifiedCompound:
        //Close to Head
        offset.z = (entity->obbMaxs().z - entity->obbMins().z) * 0.92f;
        break;
    default:
        break;
    }
    return offset;
}

Vector getProjectileTarget(UserCmd* cmd, Entity* entity, Vector offset, float& bestFov, Vector localPlayerEyePosition, bool friendlyFire) noexcept
{
    const auto origin = entity->origin() + offset;

    const Vector angle{ Math::calculateRelativeAngle(localPlayerEyePosition, origin, cmd->viewangles) };
    const float fov{ angle.length2D() };
    if (fov > bestFov)
        return Vector{ 0.0f, 0.0f, 0.0f };

    if (!entity->isVisible(origin, friendlyFire))
        return Vector{ 0.0f, 0.0f, 0.0f };

    if (fov < bestFov) {
        bestFov = fov;
        return origin;
    }
    return Vector{ 0.0f, 0.0f, 0.0f };
}

bool calculateProjectileInfo(Vector source, Vector destination, ProjectileSimulation::ProjectileWeaponInfo projectileInfo, float& time, Vector& angle) noexcept
{
    static auto gravityConvar = interfaces->cvar->findVar("sv_gravity");
    //if they use vphysics gravity doesnt affect the projectile
    const float gravity = projectileInfo.usesPipes ? projectileInfo.gravity * 800.0f : projectileInfo.gravity * gravityConvar->getFloat();
    if (gravity == 0.0f)
    {
        angle = Math::calculateRelativeAngle(source, destination, Vector{ 0.0f, 0.0f, 0.0f });
        time = source.distTo(destination) / projectileInfo.speed;
        return true;
    }

    const Vector delta = destination - source;
    const float distance = delta.length2D();
    float projectileSpeed = projectileInfo.speed;

    if (projectileInfo.usesPipes)
    {
        if (projectileSpeed > MAX_VELOCITY)
            projectileSpeed = MAX_VELOCITY;
    }

    float root = std::powf(projectileSpeed, 4) - gravity * (gravity * std::powf(distance, 2) + 2.f * delta.z * std::powf(projectileSpeed, 2));
    if (root < 0.0f)
        return false;

    const float pitch = std::atanf((std::powf(projectileSpeed, 2) - std::sqrtf(root)) / (gravity * distance));
    const float yaw = std::atan2f(delta.y, delta.x);

    angle = Vector{ -Helpers::rad2deg(pitch), Helpers::rad2deg(yaw), 0.0f };
    time = distance / (cos(pitch) * projectileSpeed);

    //thank spook for this code
    if (projectileInfo.usesPipes)
    {
        auto magic{ 0.0f };

        switch (projectileInfo.weaponId)
        {
            case WeaponId::GRENADELAUNCHER:
                magic = projectileInfo.itemDefinitionIndex == Demoman_m_TheLochnLoad ? 0.07f : 0.11f;
                break;
            case WeaponId::PIPEBOMBLAUNCHER:
                magic = 0.16f;
                break;
            case WeaponId::CANNON:
                magic = 0.35f;
                break;
            default:
                break;
        }

        projectileSpeed -= (projectileSpeed * time) * magic;

        root = std::powf(projectileSpeed, 4) - gravity * (gravity * std::powf(distance, 2) + 2.f * delta.z * std::powf(projectileSpeed, 2));
        if (root < 0.0f)
            return false;

        angle = Vector{ -Helpers::rad2deg(pitch), Helpers::rad2deg(yaw), 0.0f };
        time = distance / (cos(pitch) * projectileSpeed);
    }

    return true;
}

bool doesProjectileHit(ProjectileSimulation::ProjectileWeaponInfo projectileInfo, Vector source, Vector destination) noexcept
{
    static auto gravityConvar = interfaces->cvar->findVar("sv_gravity");
    const float gravity = projectileInfo.gravity * gravityConvar->getFloat();
    if (gravity == 0.0f)
    {
        Trace trace;
        //This trace should use hullSize, but whatever
        interfaces->engineTrace->traceRay({ source, destination }, MASK_SHOT | CONTENTS_HITBOX, TraceFilterHitscan{ localPlayer.get() }, trace);

        float time = source.distTo(trace.endpos) / projectileInfo.speed;

        if (projectileInfo.maxTime != 0.0f && time > projectileInfo.maxTime)
            return false;

        return trace.fraction > 0.97f;
    }
    //TODO: Complete this func
    //Get hullSize of every projectile pls
    return true;
}

void AimbotProjectile::run(Entity* activeWeapon, UserCmd* cmd) noexcept
{
    const auto& cfg = config->aimbot.projectile;
    if (!cfg.enabled)
        return;

    const auto network = interfaces->engine->getNetworkChannel();
    if (!network)
        return;

    if (!cfg.autoShoot && !cfg.aimlock && !isAttacking(cmd, activeWeapon))
        return;

    if (!canAttack(cmd, activeWeapon))
        return;

    const auto& enemies = TargetSystem::playerTargets(cfg.sortMethod);

    auto bestFov = cfg.fov;

    //Still wrong but atleast its better?
    const float latencyTime = ticksToTime(timeToTicks(max(0, network->getLatency(0))));

    const auto projectileWeaponInfo = ProjectileSimulation::getProjectileWeaponInfo(localPlayer.get(), activeWeapon);
    if (projectileWeaponInfo.itemDefinitionIndex == Demoman_s_StickyJumper)
        return;

    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    const int maxTicks = timeToTicks((projectileWeaponInfo.maxTime == 0.f ? cfg.maxTime : projectileWeaponInfo.maxTime) + latencyTime);
    
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

        //We cant predict entities that fly and shit
        if (entity->moveType() != MoveType::WALK)
            continue;

        const auto aimOffset = getProjectileWeaponAimOffset(activeWeapon, entity);
        if (getProjectileTarget(cmd, entity, aimOffset, bestFov, localPlayerEyePosition, cfg.friendlyFire).null())
            continue;

        MovementRebuild::setEntity(entity);
        
        bool foundTarget = false;
        for (int step = 1; step <= maxTicks; step++)
        {
            Vector position = MovementRebuild::runPlayerMove() + aimOffset;
            float currentTime = static_cast<float>(step) * memory->globalVars->intervalPerTick;

            Vector angle{ 0.0f, 0.0f, 0.0f };
            float time = 0.0f;
            if (!calculateProjectileInfo(localPlayerEyePosition, position, projectileWeaponInfo, time, angle))
                continue;

            time += latencyTime;

            if (time > currentTime)
                continue;

            //We calculate the trajectory of the projectile and if it doesnt hit we just continue
            if (!doesProjectileHit(projectileWeaponInfo, localPlayerEyePosition - projectileWeaponInfo.offset, position))
                continue;

            //We already know that it can hit, so we set angle and fire
            //TODO: Compensate for weapon offset? idk how
            cmd->viewangles = angle;

            if (cfg.autoShoot)
            {
                cmd->buttons |= UserCmd::IN_ATTACK;
                if (activeWeapon->weaponId() == WeaponId::COMPOUND_BOW 
                    || activeWeapon->weaponId() == WeaponId::PIPEBOMBLAUNCHER
                    || activeWeapon->weaponId() == WeaponId::CANNON)
                {
                    if (activeWeapon->chargeTime() > 0.0f)
                        cmd->buttons &= ~UserCmd::IN_ATTACK;
                }
            }

            if (!cfg.silent)
                interfaces->engine->setViewAngles(cmd->viewangles);
            foundTarget = true;
            break;
        }
        if (foundTarget)
            break;
    }
}

void AimbotProjectile::reset() noexcept
{

}