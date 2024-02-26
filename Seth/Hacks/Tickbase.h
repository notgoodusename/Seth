#pragma once

struct UserCmd;

namespace Tickbase
{
	void start(UserCmd* cmd) noexcept;
	void end(UserCmd* cmd) noexcept;

	bool shift(UserCmd* cmd, int shiftAmount, bool forceShift = false) noexcept;

	bool canRun() noexcept;
	bool canShift(int shiftAmount, bool forceShift = false) noexcept;

	bool setCorrectTickbase(int commandNumber) noexcept;

	int getTargetTickShift() noexcept;
	int getTickshift() noexcept;

	void resetTickshift() noexcept;

	void updateChokedCommands(int currentChokedCommands) noexcept;

	bool& isFinalTick() noexcept;
	bool& isShifting() noexcept;
	bool& isRecharging() noexcept;

	void updateInput() noexcept;
	void reset() noexcept;
}