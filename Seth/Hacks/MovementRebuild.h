#pragma once

#include "../SDK/GameMovement.h"
#include "../SDK/EngineTrace.h"
#include "../SDK/Entity.h"
#include "../SDK/Vector.h"

namespace MovementRebuild
{
	void init() noexcept;

	void setEntity(Entity* player) noexcept;
	Vector runPlayerMove() noexcept;

	Vector getPlayerMins() noexcept;
	Vector getPlayerMaxs() noexcept;

	void tracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, Trace& pm) noexcept;

	void tryTouchGround(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, int collisionGroup, Trace& pm) noexcept;

	unsigned int playerSolidMask(bool brushOnly = false) noexcept;

	struct info {
		Entity* player;
		Entity* groundEntity;
		Vector velocity;
		Vector baseVelocity;
		Vector position;
		Vector obbMins;
		Vector obbMaxs;
		Vector eyeAngles;
		float sideMove;
		float forwardMove;
		float upMove;
		float surfaceFriction;
		int waterLevel;
		float maxSpeed;
	};

	struct Cvars {
		ConVar* accelerate;
		ConVar* airAccelerate;
		ConVar* bounce;
		ConVar* friction;
		ConVar* gravity;
		ConVar* stopSpeed;
		ConVar* maxVelocity;
		ConVar* optimizedMovement;
		ConVar* parachuteMaxSpeedZ;
	};

	void waterMove() noexcept;

	void friction() noexcept;

	void airAccelerate(Vector& wishdir, float wishspeed, float accel) noexcept;

	void airMove() noexcept;

	void accelerate(Vector& wishdir, float wishspeed, float accel) noexcept;

	void walkMove() noexcept;

	void stayOnGround() noexcept;

	void fullWalkMove() noexcept;

	void startGravity() noexcept;
	void finishGravity() noexcept;

	int tryPlayerMove(Vector* firstDest = NULL, Trace* firstTrace = NULL) noexcept;

	void checkVelocity() noexcept;

	int	clipVelocity(Vector& in, Vector& normal, Vector& out, float overbounce) noexcept;

	int stuck() noexcept;

	bool checkWater() noexcept;

	void categorizePosition() noexcept;

	void categorizeGroundSurface(Trace& pm) noexcept;

	int testPlayerPosition(const Vector& pos, int collisionGroup, Trace& pm) noexcept;

	void setGroundEntity(Trace* pm) noexcept;

	void stepMove(Vector& vecDestination, Trace& trace) noexcept;

	void tryTouchGroundInQuadrants(const Vector& start, const Vector& end, unsigned int mask, int collisionGroup, Trace& pm) noexcept;
};