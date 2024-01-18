#pragma once
#include <deque>
#include <vector>

#include "../SDK/ModelInfo.h"
#include "../SDK/Vector.h"

struct UserCmd;

struct LocalPlayerInfo;
struct PlayerTarget;

namespace TargetSystem
{
	void updateFrame() noexcept;
	void updateTick(UserCmd* cmd) noexcept;

	void reset() noexcept;

	void setPriority(int handle, int priority) noexcept;

	const LocalPlayerInfo& local() noexcept;
	const std::vector<PlayerTarget>& playerTargets(int sortType = -1) noexcept;
	const PlayerTarget* playerByHandle(int handle) noexcept;
};

struct LocalPlayerInfo 
{
	void update() noexcept;

	int handle;
	Vector origin, eyePosition, viewAngles;
};

struct Target
{
	Target(Entity* entity) noexcept;

	int handle;
	int priority{ 1 };
	float simulationTime{ -1.0f };
	
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
		Vector eyeAngle{ };
		Vector worldSpaceCenter{ };

		float simulationTime{ };

		std::array<matrix3x4, MAXSTUDIOBONES> matrix;
		
	};

	bool isAlive{ true };

	std::deque<Record> playerData;
};

enum SortType
{
    Distance,
    Fov
};