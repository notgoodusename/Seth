#pragma once

#include <deque>

struct UserCmd;

namespace Crithack
{
	void run(UserCmd*) noexcept;
	void reset() noexcept;
}