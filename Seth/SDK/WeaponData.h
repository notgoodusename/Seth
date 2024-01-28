#pragma once

#include "Pad.h"

class FileWeaponInfo
{
public:
	PAD(4)

	bool parsedScript;
	bool loadedHudElements;
	char className[80];
	char printName[80];
	char viewModel[80];
	char worldModel[80];
	char animationPrefix[16];
	int slot;
	int position;
	int maxClip1;
	int maxClip2;
	int defaultClip1;
	int defaultClip2;
	int weight;
	int rumbleEffect;
	bool autoSwitchTo;
	bool autoSwitchFrom;
	int flags;
	char ammo1[32];
	char ammo2[32];

	char shootSounds[16][80];

	int ammoType;
	int ammo2Type;
	bool meleeWeapon;
	bool builtRightHanded;
	bool allowFlipping;

	int spriteCount;
	void* iconActive;
	void* iconInactive;
	void* iconAmmo;
	void* iconAmmo2;
	void* iconCrosshair;
	void* iconAutoaim;
	void* iconZoomedCrosshair;
	void* iconZoomedAutoaim;
	void* iconSmall;

	bool showUsageHint;
};

struct WeaponData
{
	int damage;
	int bulletsPerShot;
	float range;
	float spread;
	float punchAngle;
	float timeFireDelay;
	float timeIdle;
	float timeIdleEmpty;
	float timeReloadStart;
	float timeReload;
	bool drawCrosshair;
	int projectile;
	int ammoPerShot;
	float projectileSpeed;
	float smackDelay;
	bool useRapidFireCrits;
};

class TFWeaponInfo : public FileWeaponInfo
{
public:
	PAD(4)

	WeaponData weaponData[2];
	int weaponType;
	bool grenade;
	float damageRadius;
	float primerTime;
	bool lowerWeapon;
	bool suppressGrenTimer;
	bool hasTeamSkins_Viewmodel;
	bool hasTeamSkins_Worldmodel;
	char muzzleFlashModel[128];
	float muzzleFlashModelDuration;
	char muzzleFlashParticleEffect[128];
	char tracerEffect[128];
	bool doInstantEjectBrass;
	char brassModel[128];
	char explosionSound[128];
	char explosionEffect[128];
	char explosionPlayerEffect[128];
	char explosionWaterEffect[128];
	bool dontDrop;
};