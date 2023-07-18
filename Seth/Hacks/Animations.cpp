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

bool updatingLocal{ true };
bool updatingPlayer{ true };
bool sendPacket{ true };

bool skipAnimStateUpdate{ false };

float simulationTime{ 0.0f };
Vector viewangles{};

std::array<Animations::Players, 65> players{};

void Animations::init() noexcept
{

}

void Animations::reset() noexcept
{
    updatingLocal = true;
    updatingPlayer = true;
    sendPacket = true;
    skipAnimStateUpdate = false;
    simulationTime = 0.0f;
    for (auto& record : players)
        record.clear();
}

void Animations::update(UserCmd* cmd, bool& _sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (interfaces->engine->isHLTV())
        return;

    if (!localPlayer->getAnimState())
        return;

    viewangles = cmd->viewangles;
    sendPacket = _sendPacket;

    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;

    if(!simulationTime)
        simulationTime = localPlayer->simulationTime();

    static auto savedAbsAngle = localPlayer.get()->getAbsAngle();
    static auto savedPoses = localPlayer->poseParameters();

    if ((sendPacket && netChannel->chokedPackets <= 0) 
        || netChannel->chokedPackets == 1)
    {
        //We update hitboxes
        updatingLocal = true;
        
        localPlayer->updateTFAnimState(localPlayer->getAnimState(), viewangles);
        skipAnimStateUpdate = true;
        localPlayer->updateClientSideAnimation();
        skipAnimStateUpdate = false;

        simulationTime = localPlayer->simulationTime();
        updatingLocal = false;

        savedPoses = localPlayer->poseParameters();
        savedAbsAngle = localPlayer.get()->getAbsAngle();
    }

    localPlayer->poseParameters() = savedPoses;
    memory->setAbsAngle(localPlayer.get(), Vector{ 0.0f, savedAbsAngle.y, 0.0f });
}

void Animations::handlePlayers(FrameStage stage) noexcept
{
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

        if (player.simulationTime < entity->simulationTime())
        {
            player.origin = entity->origin();
            player.eyeAngle = entity->eyeAngles();
            player.absAngle = entity->getAbsAngle();

            player.mins = entity->getCollideable()->obbMins();
            player.maxs = entity->getCollideable()->obbMaxs();
            player.gotMatrix = entity->setupBones(player.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);

            updatingPlayer = true;
            entity->updateClientSideAnimation();
            updatingPlayer = false;

            player.simulationTime = entity->simulationTime();

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

bool Animations::isSkippingAnimStateUpdate() noexcept
{
    return skipAnimStateUpdate;
}

bool Animations::isLocalUpdating() noexcept
{
    return updatingLocal;
}

bool Animations::isPlayerUpdating() noexcept
{
    return updatingPlayer;
}

const float Animations::getLocalSimulationTime() noexcept
{
    return simulationTime;
}

const float Animations::getPlayerSimulationTime(int index) noexcept
{
    return players[index].simulationTime;
}

const std::array<Animations::Players, 65>& Animations::getPlayers() noexcept
{
    return players;
}

const std::deque<Animations::Players::Record>* Animations::getBacktrackRecords(int index) noexcept
{
    return &players.at(index).backtrackRecords;
}