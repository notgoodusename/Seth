#pragma once

#include "NetworkChannel.h"
#include "Pad.h"
#include "Vector.h"

class ClientState
{
public:
    PAD(16)
    NetworkChannel* networkChannel;
    PAD(396)
    int	deltaTick;
    PAD(272)
    int maxClients;
    PAD(18540)
    int lastOutgoingCommand;
    int chokedCommands;
    int lastCommandAck;
};