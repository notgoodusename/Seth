#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

class Entity;
class MoveData;
class MoveHelper;
struct UserCmd;

class Prediction {
public:
    PAD(4);
    std::uintptr_t lastGround;
    bool inPrediction;
    bool isFirstTimePredicted;
    bool oldCLPredictValue;
    bool enginePaused;
	int	commandsPredicted;
	int	serverCommandsAcknowledged;
	int	previousAckHadErrors;
	int	incomingPacketNumber;
	float idealPitch;

    VIRTUAL_METHOD(void, update, 3, (int startFrame, bool validFrame, int incAck, int outCmd), (this, startFrame, validFrame, incAck, outCmd))
    VIRTUAL_METHOD(void, setupMove, 18, (Entity* localPlayer, UserCmd* cmd, MoveHelper* moveHelper, MoveData* moveData), (this, localPlayer, cmd, moveHelper, moveData))
    VIRTUAL_METHOD(void, finishMove, 19, (Entity* localPlayer, UserCmd* cmd, MoveData* moveData), (this, localPlayer, cmd, moveData))
};
