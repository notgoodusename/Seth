#pragma once

#include "../SDK/Vector.h"

namespace ProjectileSimulation
{
    struct ProjectileWeaponInfo
    {
    public:
        float speed;
        float gravity;
        float maxTime;
        Vector offset;

        //used for projectile simulation
        Vector spawnPosition;
        Vector spawnAngle;

        bool usesPipes;

        WeaponId weaponId;
        int itemDefinitionIndex;
    };
    
    ProjectileWeaponInfo getProjectileWeaponInfo(Entity* player, Entity* activeWeapon, const Vector angles = Vector{ }) noexcept;
    bool init(const ProjectileWeaponInfo& projectileInfo) noexcept;
    void runTick() noexcept;
    Vector getOrigin() noexcept;

}