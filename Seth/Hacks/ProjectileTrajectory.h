#pragma once

struct ImDrawList;
struct UserCmd;

namespace ProjectileTrajectory
{
	void calculate(UserCmd* cmd) noexcept;
	void draw(ImDrawList* drawList) noexcept;
}