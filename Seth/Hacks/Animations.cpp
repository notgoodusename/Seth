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
        record.clear();
}

void Animations::handlePlayers(FrameStage stage) noexcept
{
    static auto gravity = interfaces->cvar->findVar("sv_gravity");
    if (stage != FrameStage::NET_UPDATE_END)
        return;

    if (!localPlayer)
    {
        for (auto& record : players)
            record.clear();
        return;
    }

    const auto& localPlayerOrigin{ localPlayer->getAbsOrigin() };

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
            player.clear();
        }

        if (player.simulationTime != entity->simulationTime())
        {
            player.origin = entity->origin();
            player.eyeAngle = entity->eyeAngles();
            player.absAngle = entity->getAbsAngle();

            player.simulationTime = entity->simulationTime();
            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
            player.gotMatrix = entity->setupBones(player.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);

            //Handle backtrack
            if (!player.gotMatrix)
                continue;

            if (!player.backtrackRecords.empty() && (player.backtrackRecords.front().simulationTime == entity->simulationTime()))
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

            while (player.backtrackRecords.size() > 3U && static_cast<int>(player.backtrackRecords.size()) > static_cast<int>(round(1.0f / memory->globalVars->intervalPerTick)))
                player.backtrackRecords.pop_back();
        }
    }
}

Animations::Players Animations::getPlayer(int index) noexcept
{
    if (index >= static_cast<int>(players.size()))
        return {};
    return players.at(index);
}

Animations::Players* Animations::setPlayer(int index) noexcept
{
    if (index >= static_cast<int>(players.size()))
        return {};
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