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

	float calculateCost(Entity* activeWeapon) noexcept;

	void handleEvent(GameEvent* event) noexcept;
	void resync() noexcept;
	int getDamageTillUnban() noexcept;
	void handleCanFireRandomCriticalShot(float critChance, Entity* entity) noexcept;
	bool isAttackCriticalHandler() noexcept;
	
	void setCorrectDamage(float damage) noexcept;
	bool protectData() noexcept;

	struct PlayerHealthInfo
	{
	public:
		PlayerHealthInfo(int handle, int syncedHealth) : handle { handle }, syncedHealth { syncedHealth } { }

		void a()
		{
			handle = 0;
		}

		int handle{ 0 };
		int syncedHealth{ 0 };
		int oldHealth{ 0 };
		int newHealth{ 0 };
	};

	void updateHealth(int handle, int newHealth) noexcept;
}

/*
	u32 __crithack::get_damage_till_unban() {
	void __crithack::update_damage() {
	bool __crithack::get_total_crits( u32& potential_crits, u32& crits ) const {

	bool __crithack::force( const bool should_crit ) {
	void __crithack::compute_can_crit()
	bool __crithack::is_attack_critical_handler()
	void __crithack::handle_fire_game_event( interface_game_event* game_event ) {
	void __crithack::handle_can_fire_random_critical_shot( float crit_chance ) {
	void __crithack::fill() {
	u32 __crithack::decrypt_or_encrypt_seed( _weapon* localplayer_wep, const u32 seed ) {
		bool __crithack::is_pure_crit_command( const i32 command_number, const i32 range, const bool lower_than ) {

	void __crithack::fix_heavy_rev_bug() {
*/