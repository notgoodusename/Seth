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

		int handle = -1;

		float simulationTime{ -1.0f };
		bool gotMatrix{ false };

		void clear()
		{
			gotMatrix = false;
			simulationTime = -1.0f;

			origin = Vector{};
			eyeAngle = Vector{};
			absAngle = Vector{};
			mins = Vector{};
			maxs = Vector{};

			backtrackRecords.clear();
		}
	};

	bool isSkippingAnimStateUpdate() noexcept;
	bool isLocalUpdating() noexcept;

	const std::array<Animations::Players, 65>& getPlayers() noexcept;
	const std::deque<Players::Record>* getBacktrackRecords(int index) noexcept;
}