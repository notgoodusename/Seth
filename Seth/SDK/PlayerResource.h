#pragma once
#include <string>

#include "../Netvars.h"
#include "VirtualMethod.h"

struct Vector;

/*
class PlayerResource
{
public:
	
	NETVAR(ping, "CPlayerResource", "m_iPing", void*)
    NETVAR(score, "CPlayerResource", "m_iScore", void*)
    NETVAR(deaths, "CPlayerResource", "m_iDeaths", void*)
    NETVAR(connected, "CPlayerResource", "m_bConnected", void*)
    NETVAR(team, "CPlayerResource", "m_iTeam", void*)
    NETVAR(alive, "CPlayerResource", "m_bAlive", void*)
    NETVAR(health, "CPlayerResource", "m_iHealth", void*)
    NETVAR(accountID, "CPlayerResource", "m_iAccountID", void*)
    NETVAR(valid, "CPlayerResource", "m_bValid", void*)
    NETVAR(userID, "CPlayerResource", "m_iUserID", void*)
};
*/

class TFPlayerResource
{
public:
	//PlayerResource
	int getPing(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iPing"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getScore(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iScore"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getDeaths(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iDeaths"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	bool getConnected(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_bConnected"));
		return *reinterpret_cast<bool*>(this + offset + index);
	}

	int getTeam(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iTeam"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	bool isAlive(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_bAlive"));
		return *reinterpret_cast<bool*>(this + offset + index);
	}

	int getHealth(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iHealth"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	unsigned int getAccountID(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iAccountID"));
		return *reinterpret_cast<unsigned int*>(this + offset + 4 * index);
	}

	bool isValid(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_bValid"));
		return *reinterpret_cast<bool*>(this + offset + index);
	}

	bool getUserID(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CPlayerResource->m_iUserID"));
		return *reinterpret_cast<bool*>(this + offset + index);
	}

	const char* getPlayerName(int index) noexcept
	{
		return *reinterpret_cast<const char**>(this + 1364U + 4 * index);
	}

	//TFPlayerResource
	int getTotalScore(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iTotalScore"));
		return *reinterpret_cast<unsigned int*>(this + offset + 4 * index);
	}

	int getMaxHealth(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iMaxHealth"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getMaxBuffedHealth(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iMaxBuffedHealth"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getPlayerClass(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iPlayerClass"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	bool isArenaSpectator(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_bArenaSpectator"));
		return *reinterpret_cast<int*>(this + offset + index);
	}

	int getActiveDominations(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iActiveDominations"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	float getNextRespawnTime(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_flNextRespawnTime"));
		return *reinterpret_cast<float*>(this + offset + 4 * index);
	}

	int getChargeLevel(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iChargeLevel"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getDamage(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iDamage")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	int getDamageAssist(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iDamageAssist")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	int getDamageBoss(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iDamageBoss")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	int getHealing(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iHealing")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	int getHealingAssist(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iHealingAssist"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getDamageBlocked(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iDamageBlocked"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getCurrencyCollected(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iCurrencyCollected"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getBonusPoints(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iBonusPoints"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getPlayerLevel(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iPlayerLevel"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int* getStreaks(int index) noexcept
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iStreaks"));
		return reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getUpgradeRefundCredits(int index) noexcept 
	{
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iUpgradeRefundCredits"));
		return *reinterpret_cast<int*>(this + offset + 4 * index);
	}

	int getBuybackCredits(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iBuybackCredits")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}
	
	NETVAR(getPartyLeaderRedTeamIndex, "CTFPlayerResource", "m_iPartyLeaderRedTeamIndex", int)
	NETVAR(getPartyLeaderBlueTeamIndex, "CTFPlayerResource", "m_iPartyLeaderBlueTeamIndex", int)
	NETVAR(getEventTeamStatus, "CTFPlayerResource", "m_iEventTeamStatus", int)

	int getPlayerClassWhenKilled(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iPlayerClassWhenKilled")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	int getConnectionState(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_iConnectionState")); 
		return *reinterpret_cast<int*>(this + offset + 4 * index); 
	}

	float getConnectTime(int index) noexcept 
	{ 
		static auto offset = Netvars::get(fnv::hash("CTFPlayerResource->m_flConnectTime")); 
		return *reinterpret_cast<float*>(this + offset + 4 * index);
	}
};