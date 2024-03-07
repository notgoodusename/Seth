#pragma once

#include "../SDK/ClassId.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/Vector.h"

#include <deque>
#include <vector>

struct UserCmd;

struct LocalPlayerInfo;
struct ProjectileEntity;
struct PlayerTarget;
struct BuildingTarget;

namespace TargetSystem
{
	void updateFrame() noexcept;

	void reset() noexcept;

	void setPriority(int handle, int priority) noexcept;

	const LocalPlayerInfo& local() noexcept;

	const std::vector<PlayerTarget>& playerTargets(int sortType = -1) noexcept;
	const PlayerTarget* playerByHandle(int handle) noexcept;

	const std::vector<BuildingTarget>& buildingTargets(int sortType = -1) noexcept;
	const BuildingTarget* buildingByHandle(int handle) noexcept;

	const std::vector<ProjectileEntity>& projectilesVector() noexcept;

	const std::vector<int>& localStickiesHandles() noexcept;
};

struct LocalPlayerInfo 
{
	void update() noexcept;

	int handle;
	Vector origin, eyePosition, viewAngles;
};

struct ProjectileEntity
{
	ProjectileEntity(Entity* entity) noexcept;

	int handle;

	Vector origin{ };
	Vector mins{ }, maxs{ };
	ClassId classId{ 0 };
};

struct Target
{
	Target(Entity* entity) noexcept;

	int handle;
	int priority{ 1 };

	float distanceToLocal{ 0.0f };
	float fovFromLocal{ 0.0f };
};

struct PlayerTarget : Target
{
	PlayerTarget(Entity* entity) noexcept;

	void update(Entity* entity) noexcept;

	struct Record 
	{
		std::vector<Vector> headPositions; //Use this for headshoting
		std::vector<Vector> bodyPositions; //Use this for body

		Vector origin{ }, absAngle{ };
		Vector mins{ }, maxs{ };
		Vector minsPrescaled{ }, maxsPrescaled{ };
		Vector eyeAngle{ };

		float simulationTime{ };

		std::array<matrix3x4, MAXSTUDIOBONES> matrix;
	};

	bool isAlive{ true };
	float simulationTime{ -1.0f };

	std::deque<Record> playerData;
};

struct BuildingTarget : Target
{
	BuildingTarget(Entity* entity) noexcept;

	Vector origin{ };
	Vector mins{ }, maxs{ };

	int buildingType{ 0 };
};

enum SortType
{
    Distance,
    Fov
};