#pragma once

#include "../SDK/FrameStage.h"
#include "../SDK/Vector.h"

#include <array>

struct UserCmd;
struct Vector;

namespace EnginePrediction
{
	void reset() noexcept;

	void update() noexcept;
	void run(UserCmd* cmd) noexcept;

	bool wasOnGround() noexcept;
	int getFlags() noexcept;
	Vector getVelocity() noexcept;
	bool isInPrediction() noexcept;
}
