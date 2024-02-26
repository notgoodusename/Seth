#include "Tickbase.h"

#include "../SDK/ClientState.h"
#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Network.h"

int targetTickShift{ 0 };
int tickShift{ 0 };
int shiftCommandNumber{ 0 };
int rechargeCommandNumber{ 0 };
int shiftedTickbase{ 0 };

int ticksAllowedForProcessing{ 0 };
int chokedPackets{ 0 };

int totalPauseTicks{ 0 };

int tickCountAtShift{ 0 };

bool recharging{ 0 };
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

    if (!config->misc.tickBase.enabled)
    {
        hasHadTickbaseActive = false;
        targetTickShift = 0;
        return;
    }

    if (!config->misc.tickBase.warpKey.isActive() && !config->misc.tickBase.doubleTapKey.isActive())
    {
        if (hasHadTickbaseActive)
            shift(cmd, ticksAllowedForProcessing, true);
        hasHadTickbaseActive = false;
        return;
    }

    if (config->misc.tickBase.warpKey.isActive() || config->misc.tickBase.doubleTapKey.isActive())
        targetTickShift = config->misc.tickBase.ticksToShift;

    //We do -1 to leave 1 tick to fakelag
    targetTickShift = std::clamp(targetTickShift, 0, MAX_COMMANDS - 1);
    hasHadTickbaseActive = true;
}

void Tickbase::end(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;

    if (!config->misc.tickBase.warpKey.isActive() && !config->misc.tickBase.doubleTapKey.isActive())
    {
        targetTickShift = 0;
        return;
    }

    if(config->misc.tickBase.warpKey.isActive())
        shift(cmd, targetTickShift);

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    if (isAttacking(cmd, activeWeapon) && config->misc.tickBase.doubleTapKey.isActive())
        shift(cmd, targetTickShift);
}

static int timeTillRecharge() noexcept
{
    const float time = config->misc.tickBase.timeTillRecharge;

    if (config->misc.tickBase.autoRecharge)
        return static_cast<int>(time / memory->globalVars->intervalPerTick);

    if (config->misc.tickBase.rechargeKey.isActive())
        return INT_MIN;

    return INT_MAX;
}

bool Tickbase::shift(UserCmd* cmd, int shiftAmount, bool forceShift) noexcept
{
    if (!canShift(shiftAmount, forceShift))
        return false;

    tickCountAtShift = memory->globalVars->tickCount;
    shiftedTickbase = shiftAmount;
    shiftCommandNumber = cmd->commandNumber;
    tickShift = shiftAmount;
    return true;
}

bool Tickbase::canRun() noexcept
{
    static bool lastResult = false;

    if (!interfaces->engine->isInGame() || !interfaces->engine->isConnected() || !memory->clientState)
    {
        ticksAllowedForProcessing = 0;
        chokedPackets = 0;
        recharging = false;
        lastResult = true;
        return true;
    }

    if (!localPlayer || !localPlayer->isAlive() || !targetTickShift)
    {
        ticksAllowedForProcessing = 0;
        recharging = false;
        lastResult = true;
        return true;
    }

    static int timeRecharge = 0;

    if (lastResult)
        timeRecharge = timeTillRecharge();

    if ((ticksAllowedForProcessing < targetTickShift 
        || chokedPackets > MAX_COMMANDS - targetTickShift)
        && memory->globalVars->tickCount - tickCountAtShift > timeRecharge)
    {
        ticksAllowedForProcessing = min(ticksAllowedForProcessing++, MAX_COMMANDS);
        chokedPackets = max(chokedPackets--, 0);

        rechargeCommandNumber = memory->clientState->lastOutgoingCommand + memory->clientState->chokedCommands + 1;

        if (lastResult)
        {
            if (targetTickShift > ticksAllowedForProcessing)
                totalPauseTicks = max(max(targetTickShift - ticksAllowedForProcessing, 0), totalPauseTicks);

            if (chokedPackets > MAX_COMMANDS - targetTickShift)
                totalPauseTicks = max(max(chokedPackets - (MAX_COMMANDS - targetTickShift), 0), totalPauseTicks);
        }

        recharging = true;
        lastResult = false;
        return false;
    }

    recharging = false;
    lastResult = true;
    return true;
}

bool Tickbase::canShift(int shiftAmount, bool forceShift) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return false;

    if (!shiftAmount || shiftAmount > ticksAllowedForProcessing || memory->globalVars->tickCount - tickCountAtShift <= 1)
        return false;

    if (forceShift)
        return true;

    return true;
}

//Straight out of AdjustPlayerTimeBase
static void adjustPlayerTimeBase(int simulationTicks) noexcept
{
    int tickBase = localPlayer->tickBase();

    const auto network = interfaces->engine->getNetworkChannel();
    if (!network || !memory->clientState)
        return;

    const int serverTime = memory->clientState->clockDrift.serverTick + 1;
    if (memory->globalVars->maxClients == 1)
    {
        localPlayer->tickBase() = serverTime - simulationTicks + 1;
    }
    else
    {
        static auto clockCorrection = interfaces->cvar->findVar("sv_clockcorrection_msecs");
        if (!clockCorrection)
            return;

        const float correctionSeconds = std::clamp(clockCorrection->getFloat() / 1000.0f, 0.0f, 1.0f);
        const int correctionTicks = timeToTicks(correctionSeconds);

        const int idealFinalTick = serverTime + correctionTicks;

        const int estimatedFinalTick = tickBase + simulationTicks;

        const int tooFastLimit = idealFinalTick + correctionTicks;
        const int tooSlowLimit = idealFinalTick - correctionTicks;

        if (estimatedFinalTick > tooFastLimit || estimatedFinalTick < tooSlowLimit)
            localPlayer->tickBase() = idealFinalTick - simulationTicks + 1;
    }
}

bool Tickbase::setCorrectTickbase(int commandNumber) noexcept
{
    const auto network = interfaces->engine->getNetworkChannel();
    if (!network || !memory->clientState)
        return true;

    if (rechargeCommandNumber == commandNumber)
    {
        if (memory->globalVars->maxClients == 1)
        {
            localPlayer->tickBase() += totalPauseTicks;
        }
        else
        {
            //sv_clockcorrection_msecs is not networked, but we dont care
            static auto clockCorrection = interfaces->cvar->findVar("sv_clockcorrection_msecs");
            if (!clockCorrection)
                return false;

            const float correctionSeconds = std::clamp(clockCorrection->getFloat() / 1000.0f, 0.0f, 1.0f);
            const int correctionTicks = timeToTicks(correctionSeconds);

            if (totalPauseTicks > correctionTicks)
                localPlayer->tickBase() += totalPauseTicks;
        }

        localPlayer->tickBase()++;
        return false;
    }

    if (commandNumber == shiftCommandNumber + 1)
        adjustPlayerTimeBase(shiftedTickbase);

    return true;
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

void Tickbase::updateChokedCommands(int currentChokedCommands) noexcept
{
    if (currentChokedCommands > chokedPackets)
        chokedPackets = currentChokedCommands;
}

bool& Tickbase::isFinalTick() noexcept
{
    return finalTick;
}

bool& Tickbase::isShifting() noexcept
{
    return shifting;
}

bool& Tickbase::isRecharging() noexcept
{
    return recharging;
}

void Tickbase::updateInput() noexcept
{
    config->misc.tickBase.rechargeKey.handleToggle();
    config->misc.tickBase.doubleTapKey.handleToggle();
    config->misc.tickBase.warpKey.handleToggle();
}

void Tickbase::reset() noexcept
{
    hasHadTickbaseActive = false;
    
    recharging = false;
    shifting = false;

    ticksAllowedForProcessing = 0;
    chokedPackets = 0;
    
    shiftCommandNumber = 0;
    rechargeCommandNumber = 0;
    tickCountAtShift = 0;
    shiftedTickbase = 0;
    tickShift = 0;
}