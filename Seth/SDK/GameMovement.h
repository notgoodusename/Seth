#pragma once

#include "Vector.h"
#include "VirtualMethod.h"

class Entity;

class MoveData
{
public:
	bool firstRunOfFunctions : 1;
	bool gameCodeMovedPlayer : 1;

	int playerHandle;

	int impulseCommand;
	Vector vecViewAngles;
	Vector vecAbsViewAngles;
	int buttons;
	int oldButtons;
	float forwardMove;
	float sideMove;
	float upMove;

	float maxSpeed;
	float clientMaxSpeed;

	Vector vecVelocity;
	Vector vecAngles;
	Vector vecOldAngles;

	float outStepHeight;
	Vector outWishVel;
	Vector outJumpVel;

	Vector vecConstraintCenter;
	float constraintRadius;
	float constraintWidth;
	float constraintSpeedFactor;

	Vector vecAbsOrigin;
};

class GameMovement {
public:
	virtual			~GameMovement(void) {}

	// Process the current movement command
	virtual void	processMovement(Entity* pPlayer, MoveData* pMove) = 0;
	virtual void	startTrackPredictionErrors(Entity* pPlayer) = 0;
	virtual void	finishTrackPredictionErrors(Entity* pPlayer) = 0;
	virtual void	DiffPrint(char const* fmt, ...) = 0;

	// Allows other parts of the engine to find out the normal and ducked player bbox sizes
	virtual Vector GetPlayerMins(bool ducked) const = 0;
	virtual Vector GetPlayerMaxs(bool ducked) const = 0;
	virtual Vector GetPlayerViewOffset(bool ducked) const = 0;
	/*
    VIRTUAL_METHOD(void, processMovement, 1, (Entity* localPlayer, MoveData* moveData), (this, localPlayer, moveData))
    VIRTUAL_METHOD(void, startTrackPredictionErrors, 2, (Entity* localPlayer), (this, localPlayer))
    VIRTUAL_METHOD(void, finishTrackPredictionErrors, 3, (Entity* localPlayer), (this, localPlayer))
    VIRTUAL_METHOD(Vector&, getPlayerViewOffset, 7, (bool ducked), (this, ducked))
	*/
};
