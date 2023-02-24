#pragma once
#include "../SDK/matrix3x4.h"
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"
#include "../SDK/FrameStage.h"
#include "../SDK/ModelInfo.h"

#include <array>
#include <deque>

struct UserCmd;
enum class FrameStage;

namespace Animations
{
	void init() noexcept;
	void reset() noexcept;
	void handlePlayers(FrameStage) noexcept;

	struct Players
	{
		Players()
		{
			this->clear();
		}

		struct Record {
			std::deque<Vector> positions;
			Vector origin;
			Vector absAngle;
			Vector mins;
			Vector maxs;
			float simulationTime;
			matrix3x4 matrix[MAXSTUDIOBONES];
		};

		std::deque<Record> backtrackRecords;

		std::array<matrix3x4, MAXSTUDIOBONES> matrix;

		Vector mins{}, maxs{};
		Vector origin{};

		int handle = -1;

		float simulationTime{ -1.0f };
		int chokedPackets{ 0 };
		bool gotMatrix{ false };

		void clear()
		{
			gotMatrix = false;
			simulationTime = -1.0f;

			origin = Vector{};
			mins = Vector{};
			maxs = Vector{};

			backtrackRecords.clear();
		}

		void reset()
		{
			clear();
			chokedPackets = 0;
		}
	};
	Players getPlayer(int index) noexcept;
	Players* setPlayer(int index) noexcept;
	std::array<Animations::Players, 65> getPlayers() noexcept;
	std::array<Animations::Players, 65>* setPlayers() noexcept;
	const std::deque<Players::Record>* getBacktrackRecords(int index) noexcept;
}