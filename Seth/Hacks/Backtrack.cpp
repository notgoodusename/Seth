#include "../Config.h"

#include "Backtrack.h"
#include "TargetSystem.h"

#include "../SDK/ConVar.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Math.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Utils.h"

static std::deque<Backtrack::IncomingSequence> sequences;
float latencyRampup = 0.0f;

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
    return latencyRampup * std::clamp(static_cast<float>(config->backtrack.fakeLatencyAmount), 0.f, cvars.maxUnlag->getFloat() * 1000.0f);
}

void Backtrack::run(UserCmd* cmd) noexcept
{
    if (!config->backtrack.enabled)
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
    for (auto& sequence : sequences)
    {
        if (memory->globalVars->realTime - sequence.currentTime >= (getLatency() / 1000.0f))
        {
            network->inReliableState = sequence.inReliableState;
            network->inSequenceNr = sequence.inSequenceNr;
            break;
        }
    }
}

static int lastIncomingSequenceNumber = 0;

void Backtrack::update() noexcept
{
    if (!localPlayer)
    {
        latencyRampup = 0.0f;
        lastIncomingSequenceNumber = 0;
        sequences.clear();
        return;
    }

    const auto network = interfaces->engine->getNetworkChannel();
    if (!network)
    {
        latencyRampup = 0.0f;
        lastIncomingSequenceNumber = 0;
        sequences.clear();
        return;
    }

    latencyRampup = config->backtrack.fakeLatency ? min(1.0f, latencyRampup += memory->globalVars->intervalPerTick) : 0.0f;

    if (network->inSequenceNr > lastIncomingSequenceNumber)
    {
        lastIncomingSequenceNumber = network->inSequenceNr;

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
    if (!network)
        return false;

    const auto deadTime = static_cast<int>(memory->globalVars->serverTime() - cvars.maxUnlag->getFloat());
    if (simtime < deadTime)
        return false;

    const auto delta = std::clamp(network->getLatency(0) + network->getLatency(1) + getLerp(), 0.f, cvars.maxUnlag->getFloat())
        - (memory->globalVars->serverTime() - simtime);
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
    latencyRampup = 0.0f;
    lastIncomingSequenceNumber = 0;
    sequences.clear();
}

float Backtrack::getMaxUnlag() noexcept
{
    return cvars.maxUnlag->getFloat();
}
