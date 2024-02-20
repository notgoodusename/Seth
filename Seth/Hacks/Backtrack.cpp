#include "../Config.h"

#include "Backtrack.h"
#include "TargetSystem.h"

#include "../SDK/ConVar.h"
#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/Network.h"
#include "../SDK/Math.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Utils.h"

static std::deque<Backtrack::IncomingSequence> sequences;

struct Cvars {
    ConVar* updateRate;
    ConVar* maxUpdateRate;
    ConVar* interp;
    ConVar* interpRatio;
    ConVar* minInterpRatio;
    ConVar* maxInterpRatio;
    ConVar* maxUnlag;
};

static Cvars cvars;

float Backtrack::getLerp() noexcept
{
    auto ratio = std::clamp(cvars.interpRatio->getFloat(), cvars.minInterpRatio->getFloat(), cvars.maxInterpRatio->getFloat());
    return (std::max)(cvars.interp->getFloat(), (ratio / ((cvars.maxUpdateRate) ? cvars.maxUpdateRate->getFloat() : cvars.updateRate->getFloat())));
}

float Backtrack::getLatency() noexcept
{
    return std::clamp(static_cast<float>(config->backtrack.fakeLatencyAmount), 0.f, cvars.maxUnlag->getFloat() * 1000.0f);
}

void Backtrack::run(UserCmd* cmd) noexcept
{
    if (!config->backtrack.enabled && !config->backtrack.fakeLatency)
        return;

    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponType = activeWeapon->getWeaponType();
    if (weaponType != WeaponType::HITSCAN && weaponType != WeaponType::MELEE)
        return;

    if (!isAttacking(cmd, activeWeapon))
        return;

    const bool canHeadshot = activeWeapon->canWeaponHeadshot();

    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    auto bestFov{ 255.f };
    int bestTargetIndex = -1;
    int bestTick = -1;

    const auto& enemies = TargetSystem::playerTargets();

    for (const auto& target : enemies) 
    {
        if (target.playerData.empty() || !target.isAlive || target.priority == 0)
            continue;

        const auto entity{ interfaces->entityList->getEntityFromHandle(target.handle) };
        if (!entity || entity->isDormant()
            || !entity->isEnemy(localPlayer.get()))
            continue;

        const auto& records = target.playerData;
     
        for (int i = 0; i < static_cast<int>(records.size()); i++)
        {
            const auto& targetTick = records[i];
            if (!Backtrack::valid(targetTick.simulationTime))
                continue;

            if (weaponType != WeaponType::MELEE)
            {
                for (auto& position :
                    canHeadshot ? targetTick.headPositions : targetTick.bodyPositions) {
                    auto angle = Math::calculateRelativeAngle(localPlayerEyePosition, position, cmd->viewangles);
                    auto fov = std::hypotf(angle.x, angle.y);
                    if (fov < bestFov) {
                        bestFov = fov;
                        bestTick = i;
                        bestTargetIndex = target.handle;
                    }
                }
            }
            else
            {
                matrix3x4 backupBoneCache[MAXSTUDIOBONES];
                memcpy(backupBoneCache, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
                Vector backupPrescaledMins = entity->getCollideable()->obbMinsPreScaled();
                Vector backupPrescaledMaxs = entity->getCollideable()->obbMaxsPreScaled();
                Vector backupOrigin = entity->getAbsOrigin();
                Vector backupAbsAngle = entity->getAbsAngle();

                entity->replaceMatrix(targetTick.matrix.data());
                memory->setAbsOrigin(entity, targetTick.origin);
                memory->setAbsAngle(entity, targetTick.absAngle);
                memory->setCollisionBounds(entity->getCollideable(), targetTick.minsPrescaled, targetTick.maxsPrescaled);

                if (Math::doesMeleeHit(activeWeapon, entity->index(), cmd->viewangles))
                {
                    bestTick = i;
                    bestTargetIndex = target.handle;
                }

                entity->replaceMatrix(backupBoneCache);
                memory->setAbsOrigin(entity, backupOrigin);
                memory->setAbsAngle(entity, backupAbsAngle);
                memory->setCollisionBounds(entity->getCollideable(), backupPrescaledMins, backupPrescaledMaxs);

                if(bestTick != -1)
                    break;
            }
        }
    }

    if (bestTick <= -1 || bestTargetIndex <= -1)
        return;

    const auto& player = TargetSystem::playerByHandle(bestTargetIndex);
    if (!player || player->playerData.empty() || !player->isAlive || player->priority == 0)
        return;

    const auto& record = player->playerData[bestTick];
    cmd->tickCount = timeToTicks(record.simulationTime + getLerp());
}

void Backtrack::updateLatency(NetworkChannel* network) noexcept
{
    for (const auto& sequence : sequences)
    {
        if (memory->globalVars->realTime - sequence.currentTime >= (getLatency() / 1000.0f))
        {
            network->inReliableState = sequence.inReliableState;
            network->inSequenceNr = sequence.inSequenceNr;
            break;
        }
    }
}

void Backtrack::updateSequences() noexcept
{
    const auto network = interfaces->engine->getNetworkChannel();
    if (!network)
        return;

    if (sequences.empty() || network->inSequenceNr > sequences.front().inSequenceNr)
    {
        IncomingSequence sequence{ };
        sequence.inReliableState = network->inReliableState;
        sequence.inSequenceNr = network->inSequenceNr;
        sequence.currentTime = memory->globalVars->realTime;
        sequences.push_front(sequence);
    }

    while (sequences.size() > 2048)
        sequences.pop_back();
}

bool Backtrack::valid(float simtime) noexcept
{
    const auto network = interfaces->engine->getNetworkChannel();
    if (!network || !memory->clientState)
        return false;

    const int serverTick = memory->clientState->clockDrift.serverTick + 1;

    const auto deadTime = static_cast<int>(ticksToTime(serverTick) - cvars.maxUnlag->getFloat());
    if (simtime < deadTime) 
        return false;

    const auto delta = std::clamp(
        network->getLatency(0) + getLerp(),
        0.f,
        cvars.maxUnlag->getFloat())
        - ticksToTime(serverTick - timeToTicks(simtime));
    return std::fabs(delta) <= 0.2f;
}

void Backtrack::init() noexcept
{
    cvars.updateRate = interfaces->cvar->findVar("cl_updaterate");
    cvars.maxUpdateRate = interfaces->cvar->findVar("sv_maxupdaterate");
    cvars.interp = interfaces->cvar->findVar("cl_interp");
    cvars.interpRatio = interfaces->cvar->findVar("cl_interp_ratio");
    cvars.minInterpRatio = interfaces->cvar->findVar("sv_client_min_interp_ratio");
    cvars.maxInterpRatio = interfaces->cvar->findVar("sv_client_max_interp_ratio");
    cvars.maxUnlag = interfaces->cvar->findVar("sv_maxunlag");
}

void Backtrack::reset() noexcept
{
    sequences.clear();
}

float Backtrack::getMaxUnlag() noexcept
{
    return cvars.maxUnlag->getFloat();
}
