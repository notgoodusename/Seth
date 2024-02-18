#include "Tickbase.h"

#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Network.h"

int targetTickShift{ 0 };
int tickShift{ 0 };
int shiftCommand{ 0 };
int shiftedTickbase{ 0 };
int ticksAllowedForProcessing{ 0 };
int chokedPackets{ 0 };
int pauseTicks{ 0 };
int tickCountAtShift{ 0};
bool shifting{ false };
bool finalTick{ false };
bool hasHadTickbaseActive{ false };

void Tickbase::start(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
    {
        hasHadTickbaseActive = false;
        return;
    }

    if (const auto netChannel = interfaces->engine->getNetworkChannel(); netChannel)
        if (netChannel->chokedPackets > chokedPackets)
            chokedPackets = 0;//needs fixing

    if (!config->tickbase.warpKey.isActive())
    {
        if (hasHadTickbaseActive)
            shift(cmd, ticksAllowedForProcessing, true);
        hasHadTickbaseActive = false;
        return;
    }

    if (config->tickbase.warpKey.isActive())
    {
        targetTickShift = 19;
    }

    //We do -1 to leave 1 tick to fakelag
    targetTickShift = std::clamp(targetTickShift, 0, MAX_COMMANDS - 1);
    hasHadTickbaseActive = true;
}

void Tickbase::end(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config->tickbase.warpKey.isActive())
    {
        targetTickShift = 0;
        return;
    }

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    if (cmd->buttons & UserCmd::IN_ATTACK && canAttack(cmd, activeWeapon))
        shift(cmd, targetTickShift);
}

static int timeTillRecharge() noexcept
{
    constexpr float time = 2.5f;
    return static_cast<int>(time / memory->globalVars->intervalPerTick);
}

bool Tickbase::shift(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShift(shiftAmount, forceShift))
        return false;

    tickCountAtShift = memory->globalVars->tickCount;
    shiftedTickbase = shiftAmount;
    shiftCommand = cmd->commandNumber;
    tickShift = shiftAmount;
    return true;
}

bool Tickbase::canRun() noexcept
{
    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected())
    {
        ticksAllowedForProcessing = 0;
        chokedPackets = 0;
        pauseTicks = 0;
        return true;
    }

    if (!localPlayer || !localPlayer->isAlive() || !targetTickShift)
    {
        ticksAllowedForProcessing = 0;
        return true;
    }

    if ((ticksAllowedForProcessing < targetTickShift 
        || chokedPackets > MAX_COMMANDS - targetTickShift)
        && memory->globalVars->tickCount - tickCountAtShift > timeTillRecharge())
    {
        ticksAllowedForProcessing = min(ticksAllowedForProcessing++, MAX_COMMANDS);
        chokedPackets = max(chokedPackets--, 0);
        pauseTicks++;
        return false;
    }
    
    return true;
}

bool Tickbase::canShift(int shiftAmount, bool forceShift) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;

    if (!shiftAmount || shiftAmount > ticksAllowedForProcessing || memory->globalVars->tickCount - tickCountAtShift <= timeTillRecharge())
        return false;

    if (forceShift)
        return true;

    return true;
}

int Tickbase::getCorrectTickbase(int simulationTicks) noexcept
{
    int tickBase = localPlayer->tickBase();

    const auto network = interfaces->engine->getNetworkChannel();
    if (!network || !memory->clientState)
        return tickBase;

    //Straight out of AdjustPlayerTimeBase

    const int serverTime = memory->clientState->clockDrift.serverTick + 1;

    if (memory->globalVars->maxClients == 1)
    {
        tickBase = serverTime - simulationTicks + 1;
    }
    else
    {
        static auto clockCorrection = interfaces->cvar->findVar("sv_clockcorrection_msecs");

        const float correctionSeconds = std::clamp(clockCorrection->getFloat() / 1000.0f, 0.0f, 1.0f);
        const int correctionTicks = timeToTicks(correctionSeconds);

        const int idealFinalTick = serverTime + correctionTicks;
        
        const int latencyTicks = timeToTicks(network->getLatency(0)) - 1;

        tickBase = idealFinalTick - simulationTicks + 1;
    }

    return tickBase;
}

int& Tickbase::getShiftedTickbase() noexcept
{
    return shiftedTickbase;
}

int& Tickbase::pausedTicks() noexcept
{
    return pauseTicks;
}

int& Tickbase::getShiftCommandNumber() noexcept
{
    return shiftCommand;
}

int Tickbase::getTargetTickShift() noexcept
{
    return targetTickShift;
}

int Tickbase::getTickshift() noexcept
{
    return tickShift;
}

void Tickbase::resetTickshift() noexcept
{
    shiftedTickbase = tickShift;
    ticksAllowedForProcessing = max(ticksAllowedForProcessing - tickShift, 0);
    tickShift = 0;
}

bool& Tickbase::isFinalTick() noexcept
{
    return finalTick;
}

bool& Tickbase::isShifting() noexcept
{
    return shifting;
}

void Tickbase::updateInput() noexcept
{
    config->tickbase.warpKey.handleToggle();
}

void Tickbase::reset() noexcept
{
    hasHadTickbaseActive = false;
    pauseTicks = 0;
    chokedPackets = 0;
    tickShift = 0;
    shiftCommand = 0;
    shiftedTickbase = 0;
    ticksAllowedForProcessing = 0;
    tickCountAtShift = 0;
}