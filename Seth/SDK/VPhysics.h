#pragma once

#include "Pad.h"
#include "Vector.h"
#include "VirtualMethod.h"

#include <functional>

class PhysicsCollide;
class PhysicsCollisionSet;
class PhysicsObjectPairHash;

constexpr float maxVelocity = 2000.0f;
constexpr float maxAngularVelocity = 360.0f * 10.0f;

constexpr float DEFAULT_MIN_FRICTION_MASS = 10.0f;
constexpr float DEFAULT_MAX_FRICTION_MASS = 2500.0f;

struct ObjectParameters
{
public:
	Vector* massCenterOverride{ };
	float mass{ };
	float inertia{ };
	float damping{ };
	float rotDamping{ };
	float rotInertiaLimit{ };
	const char* name{ };
	void* gameData{ };
	float volume{ };
	float dragCoefficient{ };
	bool enableCollisions{ };
};

struct PhysicsPerformanceParameters
{
public:
	int maxCollisionsPerObjectPerTimestep;	
	int maxCollisionChecksPerTimestep;
	float maxVelocity;
	float maxAngularVelocity;
	float lookAheadTimeObjectsVsWorld;
	float lookAheadTimeObjectsVsObject;
	float minFrictionMass;
	float maxFrictionMass;

	void setDefaults() noexcept
	{
		maxCollisionsPerObjectPerTimestep = 6;
		maxCollisionChecksPerTimestep = 250;
		maxVelocity = maxVelocity;
		maxAngularVelocity = maxAngularVelocity;
		lookAheadTimeObjectsVsWorld = 1.0f;
		lookAheadTimeObjectsVsObject = 0.5f;
		minFrictionMass = DEFAULT_MIN_FRICTION_MASS;
		maxFrictionMass = DEFAULT_MAX_FRICTION_MASS;
	}
};

class PhysicsObject
{
public:
	VIRTUAL_METHOD(void, wake, 24, (), (this))

	VIRTUAL_METHOD(void, setDragCoefficient, 36, (float* drag, float* angularDrag), (this, drag, angularDrag))
	VIRTUAL_METHOD(void, setPosition, 45, (const Vector& worldPosition, const Vector& angles, bool isTeleport), (this, std::cref(worldPosition), std::cref(angles)))
	VIRTUAL_METHOD(void, getPosition, 47, (Vector* worldPosition, Vector* angles), (this, worldPosition, angles))
	VIRTUAL_METHOD(void, setVelocity, 49, (const Vector* velocity, const Vector* angularVelocity), (this, velocity, angularVelocity))

	void* gameData{ };
	void* object{ };
	const PhysicsCollide* collide{ };
	void* shadow{ };
	Vector dragBasis{ };
	Vector angularDragBasis{ };
	bool shadowTempGravityDisable : 5{ };
	bool hasTouchedDynamic : 1{ };
	bool asleepSinceCreation : 1{ };
	bool forceSilentDelete : 1{ };
	unsigned char sleepState : 2{ };
	unsigned char hingedAxis : 3{ };
	unsigned char collideType : 3{ };
	unsigned short gameIndex{ };
	unsigned short materialIndex{ };
	unsigned short activeIndex{ };
	unsigned short callbacks{ };
	unsigned short gameFlags{ };
	unsigned int contentsMask{ };
	float volume{ };
	float buoyancyRatio{ };
	float dragCoefficient{ };
	float angularDragCoefficient{ };
};

class PhysicsEnviroment
{
public:
	VIRTUAL_METHOD(void, setGravity, 3, (const Vector& gravityVector), (this, std::cref(gravityVector)))
	VIRTUAL_METHOD(void, setAirDensity, 5, (float density), (this, density))

	VIRTUAL_METHOD(PhysicsObject*, createPolyObject, 7, (const PhysicsCollide* collisionModel, int materialIndex, const Vector& position, const Vector& angles, ObjectParameters* parameters), (this, collisionModel, materialIndex, std::cref(position), std::cref(angles), parameters))

	VIRTUAL_METHOD(void, simulate, 34, (float deltaTime), (this, deltaTime))
	VIRTUAL_METHOD(void, resetSimulationClock, 39, (), (this))
	VIRTUAL_METHOD(void, setPerformanceSettings, 59, (const PhysicsPerformanceParameters* settings), (this, settings))
};

class Physics
{
public:
	VIRTUAL_METHOD(PhysicsEnviroment*, createEnviroment, 5, (), (this))
	VIRTUAL_METHOD(void, destroyEnviroment, 6, (), (this))
	VIRTUAL_METHOD(PhysicsEnviroment*, getActiveEnviromentByIndex, 7, (int index), (this, index))

	VIRTUAL_METHOD(PhysicsObjectPairHash* , createObjectPairHash, 8, (), (this))

	VIRTUAL_METHOD(void, destroyObjectPairHash, 9, (PhysicsObjectPairHash* hash), (this, hash))

	VIRTUAL_METHOD(PhysicsCollisionSet*, findOrCreateCollisionSet, 10, (unsigned int id, int maxElementCount), (this, id, maxElementCount))
	VIRTUAL_METHOD(PhysicsCollisionSet*, findCollisionSet, 11, (unsigned int id), (this, id))

	VIRTUAL_METHOD(void, destroyAllCollisionSets, 12, (), (this))
};