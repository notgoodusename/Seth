#pragma once
#include "../SDK/Vector.h"
#include "../SDK/Entity.h"

struct UserCmd;

namespace Animations
{
	void init() noexcept;
	void updateLocalAngles(UserCmd* cmd) noexcept;
	void reset() noexcept;

	Vector getLocalViewangles() noexcept;
}