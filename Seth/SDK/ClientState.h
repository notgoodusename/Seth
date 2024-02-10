#pragma once

#include "Network.h"
#include "Pad.h"
#include "Vector.h"

class ClientState
{
public:
    PAD(16)
    NetworkChannel* networkChannel;
    PAD(320)
    struct ClockDrift
    {
    public:
        float clockOffsets[16];
        int currentOffset = 0;
        int serverTick = 0;
        int clientTick = 0;
    } clockDrift;
    int	deltaTick;
    PAD(272)
    int maxClients;
    PAD(18540)
    int lastOutgoingCommand;
    int chokedCommands;
    int lastCommandAck;
};