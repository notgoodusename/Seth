#include "../Memory.h"
#include "../Interfaces.h"

#include "Animations.h"
#include "Backtrack.h"

#include "../SDK/LocalPlayer.h"
#include "../SDK/Cvar.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/MemAlloc.h"
#include "../SDK/Vector.h"

static std::array<Animations::Players, 65> players{};

void Animations::init() noexcept
{

}

void Animations::reset() noexcept
{
    for (auto& record : players)
        record.reset();
}

float Animations::getExtraTicks() noexcept
{
    if (!config->backtrack.fakeLatency || config->backtrack.fakeLatencyAmount <= 0)
        return 0.f;
    return std::clamp(static_cast<float>(config->backtrack.fakeLatencyAmount), 0.f, 800.f) / 1000.f;
}

void Animations::handlePlayers(FrameStage stage) noexcept
{
    static auto gravity = interfaces->cvar->findVar("sv_gravity");
    const float timeLimit = static_cast<float>(config->backtrack.timeLimit) / 1000.f + getExtraTicks();
    if (stage != FrameStage::NET_UPDATE_END)
        return;

    if (!localPlayer)
    {
        for (auto& record : players)
            record.clear();
        return;
    }

    const auto localPlayerOrigin{ localPlayer->getAbsOrigin() };

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        const auto entity = interfaces->entityList->getEntity(i);
        auto& player = players.at(i);
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
        {
            player.clear();
            continue;
        }

        if (entity->handle() != player.handle)
        {
            player.handle = entity->handle();
            player.reset();
        }

        bool runPostUpdate = false;

        if (player.simulationTime != entity->simulationTime() && player.simulationTime < entity->simulationTime())
        {
            runPostUpdate = true;
            if (player.simulationTime == -1.0f)
                player.simulationTime = entity->simulationTime();

            //Get chokedPackets
            const auto simDifference = fabsf(entity->simulationTime() - player.simulationTime);

            player.simulationTime != entity->simulationTime() ?
                player.chokedPackets = static_cast<int>(simDifference / memory->globalVars->intervalPerTick) - 1 : player.chokedPackets = 0;
            player.chokedPackets = std::clamp(player.chokedPackets, 0, maxUserCmdProcessTicks + 1);

            player.origin = entity->origin();
            player.eyeAngle = entity->eyeAngles();
            player.absAngle = entity->getAbsAngle();
        }

        //Setupbones
        if (runPostUpdate)
        {
            player.simulationTime = entity->simulationTime();
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
            player.gotMatrix = entity->setupBones(player.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
        }

        //Backtrack records
        if (!config->backtrack.enabled)
        {
            player.backtrackRecords.clear();
            continue;
        }

        if (runPostUpdate)
        {
            if (!player.backtrackRecords.empty() && (player.backtrackRecords.front().simulationTime == entity->simulationTime()))
                continue;

            if (!player.gotMatrix)
                continue;

            Players::Record record{ };
            record.origin = player.origin;
            record.absAngle = player.absAngle;
            record.eyeAngle = player.eyeAngle;
            record.simulationTime = player.simulationTime;
            record.mins = player.mins;
            record.maxs = player.maxs;
            std::copy(player.matrix.begin(), player.matrix.end(), record.matrix);
            for (auto bone : { 6, 0 }) {
                record.positions.push_back(record.matrix[bone].origin());
            }

            player.backtrackRecords.push_front(record);

            while (player.backtrackRecords.size() > 3U && player.backtrackRecords.size() > static_cast<size_t>(timeToTicks(timeLimit)))
                player.backtrackRecords.pop_back();
        }
    }
}

Animations::Players Animations::getPlayer(int index) noexcept
{
    return players.at(index);
}

Animations::Players* Animations::setPlayer(int index) noexcept
{
    return &players.at(index);
}

std::array<Animations::Players, 65> Animations::getPlayers() noexcept
{
    return players;
}

std::array<Animations::Players, 65>* Animations::setPlayers() noexcept
{
    return &players;
}

const std::deque<Animations::Players::Record>* Animations::getBacktrackRecords(int index) noexcept
{
    return &players.at(index).backtrackRecords;
}