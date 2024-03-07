#pragma once

struct UserCmd;
struct ImDrawList;

namespace AutoVaccinator
{
	void run(UserCmd* cmd) noexcept;
	void draw(ImDrawList* drawList) noexcept;
	void reset() noexcept;
}