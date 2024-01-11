#include "ClassId.h"
#include "EngineTrace.h"
#include "Entity.h"
#include "LocalPlayer.h"

bool TraceFilterHitscan::shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept
{
    switch (serverEntity->getClassId())
    {
    case ClassId::FuncAreaPortalWindow:
    case ClassId::FuncRespawnRoomVisualizer:
    case ClassId::SniperDot:
    case ClassId::TFKnife:
    case ClassId::TFGrenadePipebombProjectile:
    case ClassId::TFProjectile_Arrow:
    case ClassId::TFProjectile_SentryRocket:
    case ClassId::TFProjectile_ThrowableRepel:
    case ClassId::TFProjectile_Cleaver:
    case ClassId::TFProjectile_Flare:
    case ClassId::TFProjectile_Rocket:
    case ClassId::TFProjectile_EnergyBall:
    case ClassId::TFProjectile_EnergyRing:
        return false;
    case ClassId::TFPlayer:
    case ClassId::ObjectSentrygun:
    case ClassId::ObjectDispenser:
    case ClassId::ObjectTeleporter:
        if (localPlayer)
        {
            if (!serverEntity->isEnemy(localPlayer.get()))
            {
                return false;
            }
        }
        break;
    default:
        break;
    }

    return serverEntity != passEntity;
}

bool TraceFilterSimple::shouldHitEntity(Entity* handleEntity, int contentsMask)
{
    if (!handleEntity)
        return false;

    if (!memory->standardFilterRules(handleEntity, contentsMask))
        return false;

    if (passEntity) {
        if (!memory->passServerEntityFilter(handleEntity, passEntity)) {
            return false;
        }
    }

    const auto baseEntity = handleEntity->getBaseEntity();
    if (!baseEntity)
        return false;

    if (!baseEntity->shouldCollide(collisionGroup, contentsMask))
        return false;

    if (baseEntity && !memory->shouldCollide(collisionGroup, baseEntity->collisionGroup()))
        return false;

    if (extraShouldHitCheckFunction &&
        !extraShouldHitCheckFunction(handleEntity, contentsMask))
        return false;

    return true;
}

bool TraceFilterIgnorePlayers::shouldHitEntity(Entity* handleEntity, int contentsMask)
{
    if (!handleEntity)
        return false;

    const auto baseEntity = handleEntity->getBaseEntity();
    if (!baseEntity || baseEntity->isPlayer())
        return false;

    return TraceFilterSimple::shouldHitEntity(handleEntity, contentsMask);
}

bool TraceFilterIgnoreTeammates::shouldHitEntity(Entity* handleEntity, int contentsMask)
{
    if (!handleEntity)
        return false;

    const auto baseEntity = handleEntity->getBaseEntity();
    if (!baseEntity)
        return false;

    if (baseEntity->isPlayer() && static_cast<int>(baseEntity->teamNumber()) == ignoreTeam)
        return false;

    return TraceFilterSimple::shouldHitEntity(handleEntity, contentsMask);
}