#include "../Memory.h"
#include "../Interfaces.h"

#include "Animations.h"

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

float getExtraTicks() noexcept
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
        if (!config->backtrack.enabled || !entity->isEnemy(localPlayer.get()))
        {
            player.backtrackRecords.clear();
            continue;
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