#pragma once

struct itemSetting
{
    bool enabled{ true };
    int itemId{ -1 };
    int definitionOverrideIndex{ -1 };
    int unusualEffect{ 0 };
    int particleEffect{ 0 };
    int killStreak{ 0 };
    int killStreakTier{ 0 };
    int sheen{ 0 };
    bool festivizied{ false };
    bool australium{ false };
};

namespace SkinChanger
{
	void run() noexcept;
}