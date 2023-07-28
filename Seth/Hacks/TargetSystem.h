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

	const LocalPlayerInfo& local() noexcept;
	const std::vector<PlayerTarget>& playerTargets(int sortType = -1) noexcept;
	const PlayerTarget* playerByHandle(int handle) noexcept;
};

struct LocalPlayerInfo {
	void update() noexcept;

	int handle;
	Vector origin, eyePosition, viewAngles;
};

struct Target
{
	Target(Entity* entity) noexcept;

	int handle;
};

struct PlayerTarget : Target
{
	PlayerTarget(Entity* entity) noexcept;

	void update(Entity* entity) noexcept;

	struct Record {
		std::vector<Vector> positions;
		Vector origin;
		Vector absAngle;
		Vector eyeAngle;
		Vector mins;
		Vector maxs;
		float simulationTime;
		matrix3x4 matrix[MAXSTUDIOBONES];
	};

	std::deque<Record> backtrackRecords;

	std::array<matrix3x4, MAXSTUDIOBONES> matrix;

	Vector mins{}, maxs{};
	Vector origin{}, absAngle{};
	Vector eyeAngle{};

	float simulationTime{ -1.0f };
	bool isValid{ false };

	float distanceToLocal{ 0.0f };
	float fovFromLocal{ 0.0f };
};

enum SortType
{
    Distance,
    Fov
};