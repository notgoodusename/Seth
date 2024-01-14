#include "../Interfaces.h"
#include "../Memory.h"

#include "EnginePrediction.h"

#include "../SDK/ClientState.h"
#include "../SDK/Engine.h"
#include "../SDK/Entity.h"
#include "../SDK/EntityList.h"
#include "../SDK/FrameStage.h"
#include "../SDK/GameMovement.h"
#include "../SDK/GlobalVars.h"
#include "../SDK/MD5.h"
#include "../SDK/MoveHelper.h"
#include "../SDK/Prediction.h"

static int localPlayerFlags;
static Vector localPlayerVelocity;
static bool localPlayerWasOnGround;
static bool inPrediction{ false };

void EnginePrediction::reset() noexcept
{
    localPlayerFlags = {};
    localPlayerVelocity = Vector{};
    localPlayerWasOnGround = false;
    inPrediction = false;
}

void EnginePrediction::update() noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    const auto deltaTick = memory->clientState->deltaTick;
    const auto start = memory->clientState->lastCommandAck;
    const auto stop = memory->clientState->lastOutgoingCommand + memory->clientState->chokedCommands;
    interfaces->prediction->update(deltaTick, deltaTick > 0, start, stop);
}

void EnginePrediction::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    inPrediction = true;
    
    localPlayerFlags = localPlayer->flags();
    localPlayerVelocity = localPlayer->velocity();
    localPlayerWasOnGround = localPlayer->isOnGround();

    static MoveData moveData;
    memset(&moveData, 0, sizeof(MoveData));

    localPlayer->setCurrentCommand(cmd);

    const auto oldCurrentTime = memory->globalVars->currentTime;
    const auto oldFrameTime = memory->globalVars->frameTime;
    const auto oldIsFirstTimePredicted = interfaces->prediction->isFirstTimePredicted;
    const auto oldInPrediction = interfaces->prediction->inPrediction;
    const auto oldSeed = *memory->predictionRandomSeed;

    *memory->predictionRandomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;

    memory->globalVars->currentTime = memory->globalVars->serverTime();
    memory->globalVars->frameTime = interfaces->prediction->enginePaused ? 0 : memory->globalVars->intervalPerTick;
    interfaces->prediction->isFirstTimePredicted = false;
    interfaces->prediction->inPrediction = true;

    interfaces->gameMovement->startTrackPredictionErrors(localPlayer.get());

    localPlayer->updateButtonState(cmd->buttons);
    localPlayer->setLocalViewAngles(cmd->viewangles);

    localPlayer->runPreThink();
    localPlayer->runThink();

    memory->moveHelper->setHost(localPlayer.get());
    interfaces->prediction->setupMove(localPlayer.get(), cmd, memory->moveHelper, &moveData);
    interfaces->gameMovement->processMovement(localPlayer.get(), &moveData);
    interfaces->prediction->finishMove(localPlayer.get(), cmd, &moveData);

    localPlayer->runPostThink();

    interfaces->gameMovement->finishTrackPredictionErrors(localPlayer.get());
    memory->moveHelper->setHost(nullptr);

    localPlayer->setCurrentCommand(nullptr);
    *memory->predictionRandomSeed = oldSeed;

    memory->globalVars->currentTime = oldCurrentTime;
    memory->globalVars->frameTime = oldFrameTime;

    interfaces->prediction->isFirstTimePredicted = oldIsFirstTimePredicted;
    interfaces->prediction->inPrediction = oldInPrediction;

    inPrediction = false;
}

bool EnginePrediction::wasOnGround() noexcept
{
    return localPlayerWasOnGround;
}

int EnginePrediction::getFlags() noexcept
{
    return localPlayerFlags;
}

Vector EnginePrediction::getVelocity() noexcept
{
    return localPlayerVelocity;
}

bool EnginePrediction::isInPrediction() noexcept
{
    return inPrediction;
}