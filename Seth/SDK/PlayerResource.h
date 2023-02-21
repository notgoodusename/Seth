#pragma once
#include <string>

#include "../Netvars.h"
#include "VirtualMethod.h"

struct Vector;

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

class TFPlayerResource : PlayerResource
{
public:
	NETVAR(totalScore, "CTFPlayerResource", "m_iTotalScore", void*)
	NETVAR(maxHealth, "CTFPlayerResource", "m_iMaxHealth", void*)
	NETVAR(maxBuffedHealth, "CTFPlayerResource", "m_iMaxBuffedHealth", void*)
	NETVAR(playerClass, "CTFPlayerResource", "m_iPlayerClass", void*)
	NETVAR(arenaSpectator, "CTFPlayerResource", "m_bArenaSpectator", void*)
	NETVAR(activeDominations, "CTFPlayerResource", "m_iActiveDominations", void*)
	NETVAR(nextRespawnTime, "CTFPlayerResource", "m_flNextRespawnTime", void*)
	NETVAR(chargeLevel, "CTFPlayerResource", "m_iChargeLevel", void*)
	NETVAR(damage, "CTFPlayerResource", "m_iDamage", void*)
	NETVAR(damageAssist, "CTFPlayerResource", "m_iDamageAssist", void*)
	NETVAR(damageBoss, "CTFPlayerResource", "m_iDamageBoss", void*)
	NETVAR(healing, "CTFPlayerResource", "m_iHealing", void*)
	NETVAR(healingAssist, "CTFPlayerResource", "m_iHealingAssist", void*)
	NETVAR(damageBlocked, "CTFPlayerResource", "m_iDamageBlocked", void*)
	NETVAR(currencyCollected, "CTFPlayerResource", "m_iCurrencyCollected", void*)
	NETVAR(bonusPoints, "CTFPlayerResource", "m_iBonusPoints", void*)
	NETVAR(playerLevel, "CTFPlayerResource", "m_iPlayerLevel", void*)
	NETVAR(streaks, "CTFPlayerResource", "m_iStreaks", void*)
	NETVAR(upgradeRefundCredits, "CTFPlayerResource", "m_iUpgradeRefundCredits", void*)
	NETVAR(buybackCredits, "CTFPlayerResource", "m_iBuybackCredits", void*)
	NETVAR(partyLeaderRedTeamIndex, "CTFPlayerResource", "m_iPartyLeaderRedTeamIndex", int)
	NETVAR(partyLeaderBlueTeamIndex, "CTFPlayerResource", "m_iPartyLeaderBlueTeamIndex", int)
	NETVAR(eventTeamStatus, "CTFPlayerResource", "m_iEventTeamStatus", int)
	NETVAR(playerClassWhenKilled, "CTFPlayerResource", "m_iPlayerClassWhenKilled", void*)
	NETVAR(connectionState, "CTFPlayerResource", "m_iConnectionState", void*)
	NETVAR(connectTime, "CTFPlayerResource", "m_flConnectTime", void*)
};
