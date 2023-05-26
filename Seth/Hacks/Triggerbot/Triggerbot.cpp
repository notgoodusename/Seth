#include "../Aimbot/Aimbot.h"
#include "../Animations.h"
#include "../Backtrack.h"
#include "Triggerbot.h"
#include "TriggerbotHitscan.h"
#include "TriggerbotMelee.h"

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
    if (!config->triggerbotKey.isActive())
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

    switch (weaponType)
    {
    case WeaponType::UNKNOWN:
        return;
    case WeaponType::HITSCAN:
        TriggerbotHitscan::run(activeWeapon, cmd, lastTime, lastContact);
        return;
    case WeaponType::MELEE:
        TriggerbotMelee::run(activeWeapon, cmd, lastTime, lastContact);
        return;
    default:
        break;
    }
}

void Triggerbot::updateInput() noexcept
{
	config->triggerbotKey.handleToggle();
}

void Triggerbot::reset() noexcept
{

}