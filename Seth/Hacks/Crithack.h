#pragma once

#include <deque>

class Entity;
class GameEvent;
struct ImDrawList;
struct UserCmd;

namespace Crithack
{
	void run(UserCmd*) noexcept;
	void draw(ImDrawList* drawList) noexcept;
	void updatePlayers() noexcept;
	void updateInput() noexcept;
	void reset() noexcept;

	void handleEvent(GameEvent* event) noexcept;
	void handleCanFireRandomCriticalShot(float critChance, Entity* entity) noexcept;
	bool isAttackCriticalHandler(Entity* entity) noexcept;
	
	void setCorrectDamage(float damage) noexcept;
	bool protectData() noexcept;

	struct PlayerHealthInfo
	{
	public:
		PlayerHealthInfo(int handle, int syncedHealth) : handle { handle }, syncedHealth { syncedHealth } { }

		int handle{ 0 };
		int syncedHealth{ 0 };
		int oldHealth{ 0 };
		int newHealth{ 0 };
	};

	void updateHealth(int handle, int newHealth) noexcept;
}