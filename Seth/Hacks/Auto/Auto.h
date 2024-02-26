#pragma once

struct UserCmd;

namespace Auto
{
	void run(UserCmd* cmd) noexcept;
	void updateInput() noexcept;
}