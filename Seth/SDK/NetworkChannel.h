#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

class NetworkChannel
{
public:

    VIRTUAL_METHOD(float, getLatency, 9, (int flow), (this, flow))
    VIRTUAL_METHOD(bool, sendNetMsg, 37, (void* msg, bool forceReliable = false, bool voice = false), (this, msg, forceReliable, voice))

	PAD(4)
    int connectionState;
    int outSequenceNr;
    int inSequenceNr;
    int outSequenceNrAck;
    int outReliableState;
    int inReliableState;
    int chokedPackets;
};