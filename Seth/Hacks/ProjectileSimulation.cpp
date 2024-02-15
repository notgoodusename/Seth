#include "ProjectileSimulation.h"

#include "../SDK/AttributeManager.h"
#include "../SDK/Entity.h"
#include "../SDK/Physics.h"
#include "../SDK/PhysicsCollision.h"

PhysicsEnviroment* physicsEnviroment;
PhysicsObject* physicsObject;

Vector getWeaponOffsetPosition(Entity* activeWeapon) noexcept
{
    Vector offset{ 0.0f, 0.0f, 0.0f };
    switch (activeWeapon->itemDefinitionIndex())
    {
    case Soldier_m_TheDirectHit:
    case Soldier_m_RocketLauncher:
    case Soldier_m_RocketLauncherR:
    case Soldier_m_TheBlackBox:
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
    case Pyro_s_TheDetonator:
    case Pyro_s_TheFlareGun:
    case Pyro_s_FestiveFlareGun:
    case Pyro_s_TheScorchShot:
    case Pyro_s_TheManmelter:
        offset = { 23.5f, 12.0f, -3.0f };
        if (localPlayer->flags() & (1 << 1))
            offset.z = 8.0f;
        break;
    case Soldier_m_TheOriginal:
        offset = { 23.5f, 0.0f, -3.0f };
        if (localPlayer->flags() & (1 << 1))
            offset.z = 8.0f;
        break;
    case Soldier_s_TheRighteousBison:
    case Soldier_m_TheCowMangler5000:
        offset = { 23.5f, 8.0f, -3.0f };
        if (localPlayer->flags() & (1 << 1))
            offset.z = 8.0f;
        break;
    case Engi_m_ThePomson6000:
    case Sniper_m_TheHuntsman:
    case Sniper_m_FestiveHuntsman:
    case Sniper_m_TheFortifiedCompound:
    case Engi_m_TheRescueRanger:
    case Medic_m_FestiveCrusadersCrossbow:
    case Medic_m_CrusadersCrossbow:
        offset = { 23.5f, 8.0f, -3.0f };
        break;
    case Demoman_s_StickyJumper:
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
        offset = { 16.0f, 8.0f, -6.0f };
        break;
    case Medic_m_SyringeGun:
    case Medic_m_SyringeGunR:
    case Medic_m_TheBlutsauger:
    case Medic_m_TheOverdose:
        offset = { 16.0f, 6.0f, -8.0f };
        break;
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
        offset = { 0.0f, 12.0f, 0.0f };
        break;
    case Scout_t_TheSandman:
    case Scout_t_TheWrapAssassin:
    case Scout_s_TheFlyingGuillotine:
    case Scout_s_TheFlyingGuillotineG:
        offset = { 32.0f, 0.0f, -15.0f };
        break;
    default:
        break;
    }
    return offset;
}

ProjectileSimulation::ProjectileWeaponInfo ProjectileSimulation::getProjectileWeaponInfo(Entity* player, Entity* activeWeapon, const Vector angles) noexcept
{
    float beginCharge = 0.0f;
    float charge = 0.0f;
    float chargeRate = 0.0f;
    bool usesPipes = false;
    float speed = 0.0f;
    float gravity = 0.0f;
    float maxTime = 0.0f;

    Vector offset = getWeaponOffsetPosition(activeWeapon);

    const auto itemDefinitionIndex = activeWeapon->itemDefinitionIndex();
    switch (itemDefinitionIndex)
    {
    case Soldier_m_RocketLauncher:
    case Soldier_m_TheDirectHit:
    case Soldier_m_RocketLauncherR:
    case Soldier_m_TheBlackBox:
    case Soldier_m_TheLibertyLauncher:
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
        speed = AttributeManager::attributeHookFloat(1100.0f, "mult_projectile_speed", activeWeapon, 0);
        break;
    case Soldier_s_TheRighteousBison:
    case Engi_m_ThePomson6000:
        speed = 1200.0f;
        break;
    case Pyro_m_DragonsFury:
        speed = 3000.0f;
        maxTime = 0.1753f;
        break;
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
        speed = 1000.0f;
        maxTime = 0.33f;
        break;
    case Pyro_s_TheDetonator:
    case Pyro_s_TheFlareGun:
    case Pyro_s_FestiveFlareGun:
    case Pyro_s_TheScorchShot:
    case Pyro_s_TheManmelter:
        speed = AttributeManager::attributeHookFloat(2000.0f, "mult_projectile_speed", activeWeapon);
        gravity = AttributeManager::attributeHookFloat(0.3f, "mult_projectile_speed", activeWeapon);
        break;
    case Medic_m_SyringeGun:
    case Medic_m_SyringeGunR:
    case Medic_m_TheBlutsauger:
    case Medic_m_TheOverdose:
        speed = 1000.0f;
        gravity = 0.2f;
        break;
    case Engi_m_TheRescueRanger:
    case Medic_m_FestiveCrusadersCrossbow:
    case Medic_m_CrusadersCrossbow:
        speed = 2400.0f;
        gravity = 0.2f;
        break;
    case Sniper_m_TheHuntsman:
    case Sniper_m_FestiveHuntsman:
    case Sniper_m_TheFortifiedCompound:
        beginCharge = activeWeapon->chargeTime();

        charge = (beginCharge == 0.0f) ? 0.0f : memory->globalVars->serverTime() - beginCharge;

        speed = Helpers::remapValClamped(charge, 0.0f, 1.f, 1800.f, 2600.f);
        gravity = Helpers::remapValClamped(charge, 0.0f, 1.f, 0.5f, 0.1f);
        break;
    case Demoman_m_TheIronBomber:
    case Demoman_m_TheLooseCannon:
    case Demoman_m_GrenadeLauncher:
    case Demoman_m_GrenadeLauncherR:
    case Demoman_m_TheLochnLoad:
    case Demoman_m_FestiveGrenadeLauncher:
    case Demoman_m_Autumn:
    case Demoman_m_MacabreWeb:
    case Demoman_m_Rainbow:
    case Demoman_m_SweetDreams:
    case Demoman_m_CoffinNail:
    case Demoman_m_TopShelf:
    case Demoman_m_Warhawk:
    case Demoman_m_ButcherBird:
        speed = AttributeManager::attributeHookFloat(1200.0f, "mult_projectile_speed", activeWeapon);
        //https://github.com/lua9520/source-engine-2018-hl2_src/blob/3bf9df6b2785fa6d951086978a3e66f49427166a/game/shared/tf/tf_weapon_grenadelauncher.cpp#L469-L470
        if (localPlayer->hasPrecisionRune())
            speed = 3000.0f;
        gravity = 0.5f;

        switch (itemDefinitionIndex)
        {
        case Demoman_m_TheIronBomber:
            maxTime = 1.4f;
            break;
        case Demoman_m_TheLooseCannon:
            maxTime = 0.95f;
            break;
        default:
            maxTime = 2.0f;
            break;
        }
        usesPipes = true;
        break;
    case Demoman_s_StickyJumper:
    case Demoman_s_StickybombLauncher:
    case Demoman_s_StickybombLauncherR:
    case Demoman_s_FestiveStickybombLauncher:
    case Demoman_s_TheScottishResistance:
    case Demoman_s_TheQuickiebombLauncher:
        beginCharge = activeWeapon->chargeTime();
        charge = (beginCharge <= 0.0f) ? 0.f : memory->globalVars->serverTime() - beginCharge;

        chargeRate = AttributeManager::attributeHookFloat(4.0f, "stickybomb_charge_rate", activeWeapon);
        speed = Helpers::remapValClamped(charge, 0.0f, chargeRate, 900.f, 2400.f);
        gravity = Helpers::remapValClamped(charge, 0.0f, chargeRate, 0.5f, 0.0f);
        usesPipes = true;
        break;
    case Scout_t_TheSandman:
    case Scout_t_TheWrapAssassin:
        speed = 3000.0f;
        gravity = 0.5f;
        break;
    case Scout_s_TheFlyingGuillotine:
    case Scout_s_TheFlyingGuillotineG:
        speed = 3000.0f;
        gravity = 0.2f;
        break;
    default:
        break;
    }

    static auto flipViewModels{ interfaces->cvar->findVar("cl_flipviewmodels") };
    if (flipViewModels->getInt())
        offset.y *= -1.0f;

    Vector forward{ }, right{ }, up{ };
    Vector::fromAngleAll(angles, &forward, &right, &up);

    Vector eyePosition = player->getEyePosition();
    Vector spawnPosition  = player->getEyePosition() + (forward * offset.x) + (right * offset.y) + (up * offset.z);

    Vector spawnAngle = angles;
    //Projectiles have aim assist
    if(!usesPipes)
    {
        Vector endPosition{ eyePosition + (forward * 2000.0f) };

        spawnAngle = (endPosition - spawnPosition).toAngle();
    }

    return ProjectileSimulation::ProjectileWeaponInfo{ speed, gravity, maxTime, offset, spawnPosition, spawnAngle, usesPipes, activeWeapon->weaponId(), itemDefinitionIndex };
}

bool ProjectileSimulation::init(const ProjectileWeaponInfo& projectileInfo) noexcept
{
    if (!physicsEnviroment)
    {
        physicsEnviroment = interfaces->physics->createEnviroment();
    }

    if (!physicsObject)
    {
        const auto collide{ interfaces->physicsCollision->bboxToCollide({ -2.0f, -2.0f, -2.0f }, { 2.0f, 2.0f, 2.0f }) };

        ObjectParameters parameters;
        parameters.massCenterOverride = nullptr;
        parameters.mass = 1.0f;
        parameters.inertia = 0.0f;
        parameters.damping = 0.0f;
        parameters.rotDamping = 0.0f;
        parameters.rotInertiaLimit = 0.0f;
        parameters.name = "DEFAULT";
        parameters.gameData = nullptr;
        parameters.volume = 0.0f;
        parameters.dragCoefficient = 1.0f;
        parameters.enableCollisions = false;

        physicsObject = physicsEnviroment->createPolyObject(collide, 0, projectileInfo.spawnPosition, projectileInfo.spawnAngle, &parameters);

        physicsObject->wake();
    }

    if (!physicsEnviroment || !physicsObject)
        return false;

    {
        Vector forward{ }, up{ };

        Vector::fromAngleAll(projectileInfo.spawnAngle, &forward, nullptr, &up);

        Vector velocity{ forward * projectileInfo.speed };
        Vector angularVelocity{ };

        if (projectileInfo.usesPipes)
        {
            velocity += up * 200.0f;

            angularVelocity = { 600.0f, -1200.0f, 0.0f };
        }

        //https://github.com/lua9520/source-engine-2018-hl2_src/blob/3bf9df6b2785fa6d951086978a3e66f49427166a/game/shared/tf/tf_weaponbase_gun.cpp#L710C2-L710C28
        //theres only 1 item that uses the no spin attribute
        if(projectileInfo.itemDefinitionIndex == Demoman_m_TheLochnLoad)
            angularVelocity = Vector{ };

        physicsObject->setPosition(projectileInfo.spawnPosition, projectileInfo.spawnAngle, true);
        physicsObject->setVelocity(&velocity, &angularVelocity);
    }

    {
        float drag{ };
        Vector dragBasis{ };
        Vector angularDragBasis{ };

        switch (projectileInfo.weaponId)
        {
            case WeaponId::GRENADELAUNCHER:
            {
                drag = 1.0f;
                dragBasis = { 0.003902f, 0.009962f, 0.009962f };
                angularDragBasis = { 0.003618f, 0.001514f, 0.001514f };
                break;
            }
            case WeaponId::PIPEBOMBLAUNCHER:
            {
                drag = 1.0f;
                dragBasis = { 0.007491f, 0.007491f, 0.007306f };
                angularDragBasis = { 0.002777f, 0.002842f, 0.002812f };
                break;
            }

            case WeaponId::CANNON:
            {
                drag = 1.0f;
                dragBasis = { 0.020971f, 0.019420f, 0.020971f };
                angularDragBasis = { 0.012997f, 0.013496f, 0.013714f };
                break;
            }
            default:
                break;
        }

        physicsObject->setDragCoefficient(&drag, &drag);

        physicsObject->dragBasis = dragBasis;
        physicsObject->angularDragBasis = angularDragBasis;
    }

    {
        float maxVelocity = 1000000.0f;
        float maxAngularVelocity = 1000000.0f;

        if (projectileInfo.usesPipes)
        {
            maxVelocity = MAX_VELOCITY;
            maxAngularVelocity = MAX_ANGULAR_VELOCITY;
        }
        
        PhysicsPerformanceParameters parameters{ };
        parameters.setDefaults();

        parameters.maxVelocity = maxVelocity;
        parameters.maxAngularVelocity = maxAngularVelocity;

        physicsEnviroment->setPerformanceSettings(&parameters);
        physicsEnviroment->setAirDensity(2.0f);

        static auto gravityConvar = interfaces->cvar->findVar("sv_gravity");
        const float gravity = projectileInfo.usesPipes ? 800.0f : projectileInfo.gravity * gravityConvar->getFloat();
        physicsEnviroment->setGravity(Vector{ 0.0f, 0.0f, -gravity });

        physicsEnviroment->resetSimulationClock();
    }

    return true;
}

void ProjectileSimulation::runTick() noexcept
{
    if (!physicsEnviroment)
        return;

    physicsEnviroment->simulate(memory->globalVars->intervalPerTick);
}

Vector ProjectileSimulation::getOrigin() noexcept
{
    if (!physicsObject)
        return Vector{ };

    Vector out{ };

    physicsObject->getPosition(&out, nullptr);

    return out;
}