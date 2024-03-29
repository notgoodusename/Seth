#pragma once

struct UserCmd;
struct ImDrawList;

namespace Auto
{
	void run(UserCmd* cmd) noexcept;
	void updateInput() noexcept;
	void draw(ImDrawList* drawList) noexcept;
	void reset() noexcept;
}