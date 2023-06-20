#pragma once

#include <deque>

struct UserCmd;

namespace Crithack
{
	void run(UserCmd*) noexcept;
	bool protectData() noexcept;
	void reset() noexcept;
}