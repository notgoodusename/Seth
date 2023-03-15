#include "EngineTrace.h"
#include "Entity.h"

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
        !(reinterpret_cast<bool* (__cdecl*)(Entity*, int)>(extraShouldHitCheckFunction)(handleEntity, contentsMask)))
        return false;

    return true;
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

bool TraceFilterIgnorePlayers::shouldHitEntity(Entity* handleEntity, int contentsMask)
{
    if (!handleEntity)
        return false;

    const auto baseEntity = handleEntity->getBaseEntity();
    if (!baseEntity)
        return false;

    if (baseEntity->isPlayer())
        return false;

    return TraceFilterSimple::shouldHitEntity(handleEntity, contentsMask);
}