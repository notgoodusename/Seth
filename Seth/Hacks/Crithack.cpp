#include "Crithack.h"

#include "../SDK/AttributeManager.h"
#include "../SDK/Cvar.h"
#include "../SDK/Entity.h"
#include "../SDK/GameEvent.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/MD5.h"
#include "../SDK/MemAlloc.h"
#include "../SDK/Prediction.h"

#include "../Config.h"
#include "../GUI.h"
#include "../Hooks.h"
#include "../Interfaces.h"
#include "../StrayElements.h"
#include "../imguiCustom.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

#include <vector>

//https://www.unknowncheats.me/forum/team-fortress-2-a/613371-ateris-technologies-v3-source-code.html
//https://www.unknowncheats.me/forum/team-fortress-2-a/430169-perfect-crithack-engine-manipulation.html

//crit data
bool protect{ false };
bool canCrit{ false };
bool critBan{ false };

float correctCritChance{ 0.0f };

int wishFireRandomSeed{ 0 };

std::unordered_map<int, std::vector<int>> critTicks;
std::unordered_map<int, std::vector<int>> skipTicks;

//damage data
float correctDamage{ 0.f };

float rangedDamage = 0.f;
float critDamage = 0.f;
float boostedDamage = 0.f;
float meleeDamage = 0.f;

//player data
std::vector<Crithack::PlayerHealthInfo> playerHealthInfo;

//fill the critTicks/skipTicks
int lastCommandNumberScanned{ 0 };

//drawing info!!
bool canActiveWeaponRandomCrit{ false };
int activeWeaponIndex{ 0 };
int activeWeaponSlot{ 0 };

float cost{ 0.0f };
int potentialCrits{ 0 };

float critTime{ 0.0f };
bool isRapidFireCrits{ false };
int critChecks{ 0 };
int critSeedRequests{ 0 };
float critTokenBucket{ 0.0f };
float lastRapidFireCritCheckTime{ 0.0f };

static auto playerHealthInfoByHandle(int handle) noexcept
{
	const auto it = std::ranges::find(playerHealthInfo, handle, &Crithack::PlayerHealthInfo::handle);
	return it != playerHealthInfo.end() ? &(*it) : nullptr;
}

int decryptOrEncryptSeed(Entity* activeWeapon, const int seed) 
{
	int extra = activeWeapon->index() << 8 | localPlayer->index();

	if (activeWeapon->slot() == WeaponSlots::SLOT_MELEE)
		extra <<= 8;

	return extra ^ seed;
}

bool isPureCritCommand(Entity* activeWeapon, int commandNumber, int range, bool lower) noexcept
{
	const auto randomSeed = MD5_PseudoRandom(commandNumber) & MASK_SIGNED;

	memory->randomSeed(decryptOrEncryptSeed(activeWeapon, randomSeed));

	return lower ? memory->randomInt(0, 9999) < range : memory->randomInt(0, 9999) > range;
}	

int getPotentialCrits(Entity* activeWeapon) noexcept
{
	auto bucket = activeWeapon->critTokenBucket();
	int crits = 0;
	for (; crits < (activeWeapon->slot() == SLOT_MELEE ? 100 : 20); crits++)
		if (bucket >= cost)
			bucket -= cost;
		else
			break;
	potentialCrits = crits;
	return crits;
}

void updateCmds(UserCmd* cmd, Entity* activeWeapon, int cmdsToScan) noexcept
{
	//dont have this weapon in anywhere :/ - so its invalid
	auto& crits = critTicks[activeWeapon->index()];
	auto& skips = skipTicks[activeWeapon->index()];

	//clear old cmds
	std::erase_if(crits,
		[&](const auto& commandNumber) { return commandNumber < cmd->commandNumber; });
	std::erase_if(skips,
		[&](const auto& commandNumber) { return commandNumber < cmd->commandNumber; });

	static constexpr unsigned int critsMaxSize = 25U;
	static constexpr unsigned int skipsMaxSize = 25U;

	if (crits.size() < critsMaxSize || skips.size() < skipsMaxSize)
	{
		if (!lastCommandNumberScanned || lastCommandNumberScanned <= cmd->commandNumber)
			lastCommandNumberScanned = cmd->commandNumber + (cmd->commandNumber % 1024 * cmd->commandNumber % 1024);
		
		int initialCommandNumber = lastCommandNumberScanned;
		int currentCommandsScanned = 0;

		if (lastCommandNumberScanned == INT_MAX || lastCommandNumberScanned < 0)
			return;

		//just scan till its full
		for (int i = lastCommandNumberScanned; i < lastCommandNumberScanned + cmdsToScan; i++)
		{
			if (i == INT_MAX || i < 0)
				break;

			if (isPureCritCommand(activeWeapon, i, 1, true) 
				&& crits.size() < critsMaxSize)
				critTicks[activeWeapon->index()].push_back(i);
			else if (isPureCritCommand(activeWeapon, i, 9998, false)
				&& skips.size() < skipsMaxSize)
				skipTicks[activeWeapon->index()].push_back(i);

			currentCommandsScanned = i - initialCommandNumber + 1;

			if (crits.size() >= critsMaxSize && skips.size() >= skipsMaxSize)
				break;
		}
		lastCommandNumberScanned += currentCommandsScanned;
	}
}

bool canForceCrit(Entity* activeWeapon) noexcept
{
	if (activeWeapon->critTokenBucket() < cost)
		return false;

	if (activeWeapon->isRapidFireCrits() && activeWeapon->critChecks() < (activeWeapon->critSeedRequests() + 1) * 10)
		return false;

	if (activeWeapon->slot() != SLOT_MELEE && critBan)
		return false;

	return true;
}

void calculateIfCanCrit(int commandNumber, Entity* activeWeapon) noexcept
{
	canCrit = false;

	const auto seedBackup = *memory->predictionRandomSeed;

	const auto backupStart = 0xA54/*m_flCritTokenBucket*/;
	const auto backupEnd = 0xB60/*m_flLastRapidFireCritCheckTime*/ + sizeof(float);
	const auto backupSize = backupEnd - backupStart;

	float newObservedCritChance = 0.f;

	static void* backup = memory->memAlloc->alloc(backupSize);

	//backup weapon state
	memcpy(backup, static_cast<void*>(activeWeapon + backupStart), backupSize);

	activeWeapon->currentSeed()++;
	activeWeapon->lastRapidFireCritCheckTime() = 0.0f;

	*memory->predictionRandomSeed = MD5_PseudoRandom(commandNumber) & 0x7FFFFFFF;

	protect = true;
	{
		const auto weaponMode = activeWeapon->weaponMode();
		activeWeapon->weaponMode() = 0;
		
		static auto original = hooks->calcIsAttackCritical.getOriginal<void>();
		
		original(activeWeapon);

		newObservedCritChance = activeWeapon->observedCritChance();
		activeWeapon->weaponMode() = weaponMode;
	}
	protect = false;
	
	canCrit = activeWeapon->critShot();

	memcpy(static_cast<void*>(activeWeapon + backupStart), backup, backupSize);
	activeWeapon->observedCritChance() = newObservedCritChance;
	*memory->predictionRandomSeed = seedBackup;
}

float calculateCost(Entity* activeWeapon) noexcept
{
	const int seedRequests = activeWeapon->critSeedRequests();
	const int critChecks = activeWeapon->critChecks();

	float mult = 1.f;
	if (seedRequests > 0 && critChecks > 0 && activeWeapon->slot() != 2)
		mult = Helpers::remapValClamped(static_cast<float>(seedRequests) / static_cast<float>(critChecks), 0.1f, 1.f, 1.f, 3.f);

	cost = correctDamage * mult * (activeWeapon->slot() == 2 ? 0.5f : 3.0f);
	return cost;
}

int getDamageTillUnban() noexcept
{
	if (correctCritChance == 0.0f)
		return 0;

	//simple maths optimization!!
	const float requiredDamage = ((2.0f/3.0f) * critDamage * correctCritChance - rangedDamage * correctCritChance + critDamage / 3.0f) / correctCritChance;

	return static_cast<int>(std::ceilf(requiredDamage));
}

void Crithack::handleEvent(GameEvent* event) noexcept
{
	if (!localPlayer)
		return;

	const auto eventName = fnv::hashRuntime(event->getName());
	if (eventName == fnv::hash("player_hurt"))
	{
		const auto attacked = interfaces->engine->getPlayerForUserID(event->getInt("userid"));
		const auto attacker = interfaces->engine->getPlayerForUserID(event->getInt("attacker"));
		const auto crit = event->getBool("crit") || event->getBool("minicrit");
		auto damage = static_cast<float>(event->getInt("damageamount"));
		const auto health = event->getInt("health");
		const auto weaponId = event->getInt("weaponid");

		if (attacker != localPlayer->index() || attacked == attacker)
			return;

		const auto entity = interfaces->entityList->getEntity(attacked);
		if (!entity)
			return;

		const auto& player = playerHealthInfoByHandle(entity->handle());
		if (!player)
			return;

		const auto healthDelta = player->syncedHealth - health;
		player->syncedHealth = health;

		if (damage > healthDelta && health == 0)
			damage = static_cast<float>(healthDelta);

		auto activeWeapon = localPlayer->getActiveWeapon();
		if (!activeWeapon)
			return;

		if (static_cast<int>(activeWeapon->weaponId()) != weaponId)
		{
			activeWeapon = nullptr;
			auto& weapons = localPlayer->weapons();

			for (auto weaponHandle : weapons) {
				if (weaponHandle == -1)
					break;

				auto weapon = reinterpret_cast<Entity*>(interfaces->entityList->getEntityFromHandle(weaponHandle));
				if (!weapon || static_cast<int>(weapon->weaponId()) != weaponId)
					continue;

				activeWeapon = weapon;
				break;
			}
		}

		if (!activeWeapon)
			return;

		//TODO: fix damage desync!!, still to do? i have no clue
		if (activeWeapon->slot() == SLOT_MELEE)
		{
			meleeDamage += damage;
		}
		else
		{
			if (crit)
				if (localPlayer->isCritBoosted())
					boostedDamage += damage;
				else
					critDamage += damage;
		}
	}
	else if (eventName == fnv::hash("teamplay_round_start"))
	{
		correctDamage = 0.0f;
		rangedDamage = 0.f;
		critDamage = 0.f;
		boostedDamage = 0.f;
		meleeDamage = 0.f;

		canCrit = false;
		critBan = false;

		playerHealthInfo.clear();
	}
}

void Crithack::handleCanFireRandomCriticalShot(float critChance, Entity* activeWeapon) noexcept
{
	activeWeapon->observedCritChance() = 0.0f;
	correctCritChance = 0.0f;
	critBan = false;

	if (activeWeapon->slot() == SLOT_MELEE)
		return;

	const float normalizedDamage = critDamage / 3.f;
	const float damage = normalizedDamage + (rangedDamage > 0.0f ? rangedDamage : 1.0f) - critDamage;

	activeWeapon->observedCritChance() = normalizedDamage / damage;

	correctCritChance = critChance + 0.1f;
	//if less than 0 it means that the crit damage is greater than the non crit damage, which means we are crit banned!!
	critBan = activeWeapon->observedCritChance() >= correctCritChance || activeWeapon->observedCritChance() < 0.0f;
}

bool Crithack::isAttackCriticalHandler(Entity* entity) noexcept
{
	if (!interfaces->prediction->isFirstTimePredicted)
		return false;

	static int lastTickcount { -1 };

	if (lastTickcount == memory->globalVars->tickCount)
		return false;

	lastTickcount = memory->globalVars->tickCount;
	
	if (entity->weaponId() == WeaponId::FLAMETHROWER || entity->weaponId() == WeaponId::MINIGUN)
	{
		auto newAmmoCount = localPlayer->getAmmoCount(entity->primaryAmmoType());
		static auto oldAmmoCount = newAmmoCount;
		
		const auto hasFiredBullet = oldAmmoCount != newAmmoCount;

		oldAmmoCount = newAmmoCount;

		if (!hasFiredBullet)
			return false;
	}

	if (wishFireRandomSeed != 0)
	{
		*memory->predictionRandomSeed = wishFireRandomSeed;
		wishFireRandomSeed = 0;
	}

	return true;
}

void updateDamage() noexcept
{
	if (!localPlayer)
		return;

	const auto playerResource = StrayElements::getPlayerResource();
	if (!playerResource)
		return;

	const float playerResourceDamage = static_cast<float>(playerResource->getDamage(localPlayer->index()));
	
	static float maxDamage = 0.0f;
	maxDamage = max(playerResourceDamage, maxDamage);

	rangedDamage = playerResourceDamage - critDamage - boostedDamage - meleeDamage;

	//so if we have dealt damage in the past and playerdamage == 0 then yes we reset
	if (playerResourceDamage <= 0.0f && maxDamage != 0.0f)
	{
		maxDamage = 0.0f;
		rangedDamage = 0.0f;
		critDamage = 0.0f;
		boostedDamage = 0.0f;
		meleeDamage = 0.0f;
	}
}

void updateDrawInfo() noexcept
{
	if (!localPlayer)
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	canActiveWeaponRandomCrit = activeWeapon->canWeaponRandomCrit();
	activeWeaponIndex = activeWeapon->index();
	activeWeaponSlot = activeWeapon->slot();
	calculateCost(activeWeapon);
	getPotentialCrits(activeWeapon);
	critTime = activeWeapon->critTime();
	isRapidFireCrits = activeWeapon->isRapidFireCrits();
	critChecks = activeWeapon->critChecks();
	critSeedRequests = activeWeapon->critSeedRequests();
	critTokenBucket = activeWeapon->critTokenBucket();
	lastRapidFireCritCheckTime = activeWeapon->lastRapidFireCritCheckTime();
}

void Crithack::run(UserCmd* cmd) noexcept
{
	updateDamage();
	updateDrawInfo();

	static auto weaponCriticals = interfaces->cvar->findVar("tf_weapon_criticals");
	if (weaponCriticals->getInt() <= 0)
		return;

	if (!config->misc.critHack.enabled)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
		return;

	if (!activeWeapon->canWeaponRandomCrit())
	{
		critTicks[activeWeapon->index()].clear();
		skipTicks[activeWeapon->index()].clear();
		return;
	}

	if (activeWeapon->weaponId() == WeaponId::MINIGUN && cmd->buttons & UserCmd::IN_ATTACK)
		cmd->buttons &= ~UserCmd::IN_ATTACK2;

	//scan 256 ticks per tick
	updateCmds(cmd, activeWeapon, 256);

	if (localPlayer->isCritBoosted())
		return;

	bool shouldCrit = canForceCrit(activeWeapon) && config->misc.forceCritKey.isActive();
	
	auto& crits = critTicks[activeWeapon->index()];
	auto& skips = skipTicks[activeWeapon->index()];

	static int critIndex = 0, skipIndex = 0;
	if (!crits.empty())
	{
		critIndex = critIndex % crits.size();
		calculateIfCanCrit(crits[critIndex], activeWeapon);
		critIndex++;
	}

	if (activeWeapon->isRapidFireCrits() 
		&& (activeWeapon->lastRapidFireCritCheckTime() + 1.f > memory->globalVars->serverTime()))
		return;

	const bool attacking = isAttacking(cmd, activeWeapon);
	if (!attacking && activeWeapon->slot() != SLOT_MELEE)
		return;
	
	if (shouldCrit)
	{
		if (crits.empty())
			return;

		critIndex = critIndex % crits.size();

		cmd->commandNumber = crits[critIndex];
		wishFireRandomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;
		
		if(activeWeapon->slot() == SLOT_MELEE && attacking)
			crits.erase(crits.begin() + critIndex);

		critIndex++;
	}
	else if(config->misc.critHack.skipRandomCrits)
	{
		if (skips.empty())
			return;

		skipIndex = skipIndex % skips.size();

		const auto skipCommandNumber = skips[skipIndex];

		cmd->commandNumber = skips[skipIndex];
		wishFireRandomSeed = MD5_PseudoRandom(cmd->commandNumber) & MASK_SIGNED;

		if (activeWeapon->slot() == SLOT_MELEE && attacking)
			skips.erase(skips.begin() + skipIndex);

		skipIndex++;
	}
}

static ImVec2 renderText(const ImVec2& pos, const Color4& textColor, float& textOffset, ImDrawList* drawList, const char* fmt, ...) noexcept
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	const auto textSize = ImGui::CalcTextSize(buffer);

	const auto horizontalOffset = textSize.x / 2;
	const auto verticalOffset = textSize.y + textOffset;

	const auto shadowColor = Helpers::calculateColor(Color4{ 0.02f, 0.07f, 0.17f, 1.0f }); //dark blue
	const auto color = Helpers::calculateColor(textColor);
	drawList->AddText({ pos.x - horizontalOffset + 1.0f, pos.y - verticalOffset + 1.0f }, shadowColor, buffer);
	drawList->AddText({ pos.x - horizontalOffset, pos.y - verticalOffset }, color, buffer);

	textOffset -= textSize.y;

	return textSize;
}

static void renderBar(const ImVec2& pos, const ImVec2& barSize, const float& fraction, float& offset, ImDrawList* drawList)
{
	const float width = barSize.x;
	const float height = barSize.y;
	constexpr float thickeness = 1.0f;
	constexpr float barSizing = 10.0f; //bigger = less bar

	constexpr Color4 backgroundColor{ 0.02f, 0.07f, 0.17f, 1.0f }; //dark blue
	constexpr Color4 startingColor{ 0.0f, 0.75f, 1.0f, 1.0f }; //blue
	constexpr Color4 endColor{ 1.0f, 1.0f, 1.0f, 1.0f }; //white
	const Color4 lerpColor{ 
		Helpers::lerp(fraction, startingColor.color[0], endColor.color[0]), 
		Helpers::lerp(fraction, startingColor.color[1], endColor.color[1]),
		Helpers::lerp(fraction, startingColor.color[2], endColor.color[2]),
		Helpers::lerp(fraction, startingColor.color[3], endColor.color[3])};

	const auto barColorStart = Helpers::calculateColor(startingColor);
	const auto barColorEnd = Helpers::calculateColor(lerpColor);
	const auto barBackgroundColor = Helpers::calculateColor(backgroundColor);

	const float ratio = Helpers::lerp(fraction, ((2.0f * thickeness + 2.0f * barSizing)/width), 1.0f);

	//Add a bit of offset cuz its too clusttered
	offset -= 7.5f;

	drawList->AddRectFilled(
		ImVec2{ pos.x + barSizing, pos.y - offset },
		ImVec2{ pos.x + width - barSizing, pos.y - offset + height },
		barBackgroundColor);

	drawList->AddRectFilledMultiColor(
		ImVec2{ pos.x + thickeness + barSizing, pos.y - offset + thickeness },
		ImVec2{ pos.x - thickeness - barSizing + width * ratio, pos.y - offset + height - thickeness },
		barColorStart, barColorEnd, barColorEnd, barColorStart);

	offset -= height;	
}

void Crithack::draw(ImDrawList* drawList) noexcept
{
	if (!config->misc.critHack.enabled)
		return;

	//we using windows!!
	//TextWrapped doesnt have centering and antialising, so the fix is?
	//easy just use the window pos instead of some random ass pos

	if (config->misc.critHack.pos != ImVec2{}) {
		ImGui::SetNextWindowPos(config->misc.critHack.pos);
		config->misc.critHack.pos = {};
	}

	ImGui::SetNextWindowSize({ 250.0f, 0.f }, ImGuiCond_Once);
	ImGui::SetNextWindowSizeConstraints({ 250.0f, 0.f }, { 250.0f, FLT_MAX });

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (!gui->isOpen())
		windowFlags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.5f });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.5f);
	ImGui::Begin("Crit indicator", nullptr, windowFlags);
	ImGui::PopStyleVar(3);

	ImGui::End();

	static ImVec2 windowSize{ 0.0f, 0.0f };
	static ImVec2 windowPos{ 0.0f, 0.0f };
	static ImVec2 pos{ 0.0f, 0.0f };
	const auto window = ImGui::FindWindowByName("Crit indicator");
	if (window)
	{
		windowPos = window->Pos;
		pos.x = window->Pos.x + window->Size.x / 2.0f;
		pos.y = window->Pos.y + window->Size.y / 2.0f;

		windowSize = window->Size;
	}

	static const Color4 red{ 1.0f, 0.0f, 0.0f, 1.0f };
	static const Color4 green{ 0.0f, 1.0f, 0.0f, 1.0f };
	static const Color4 blue{ 0.0f, 0.75f, 1.0f, 1.0f };
	static const Color4 yellow{ 1.0f, 1.0f, 0.0f, 1.0f };

	float offset = 0.0f;
	if (gui->isOpen())
	{
		renderText(pos, blue, offset, drawList, "Crit indicator");
		return;
	}

	if (!localPlayer)
		return;

	static auto weaponCriticals = interfaces->cvar->findVar("tf_weapon_criticals");
	static auto weaponCriticalsBucketCap = interfaces->cvar->findVar("tf_weapon_criticals_bucket_cap");

	int critsPossible = 0;
	if (cost > 0.0f)
		critsPossible = static_cast<int>(static_cast<float>(weaponCriticalsBucketCap->getInt()) / cost);

	int currentFound = 0;

	auto& crits = critTicks[activeWeaponIndex];
	auto& skips = skipTicks[activeWeaponIndex];

	/*
	renderText(pos, red, offset, drawList, "Ranged damage %.2f", rangedDamage);
	renderText(pos, green, offset, drawList, "Crit damage %.2f", critDamage);
	renderText(pos, blue, offset, drawList, "Boosted damage %.2f", boostedDamage);
	renderText(pos, yellow, offset, drawList, "Melee damage %.2f", meleeDamage);
	renderText(pos, blue, offset, drawList, "Crit banned %s", critBan ? "true" : "false");
	renderText(pos, red, offset, drawList, "Can crit %s", canCrit ? "true" : "false");
	renderText(pos, green, offset, drawList, "Correct crit chance %.4f", correctCritChance);
	renderText(pos, blue, offset, drawList, "Cost %.2f", cost);
	//renderText(pos, yellow, offset, drawList, "Can force crit %s", canForceCrit(activeWeapon) ? "true" : "false");
	renderText(pos, red, offset, drawList, "Crit seed request %i", critSeedRequests);
	renderText(pos, green, offset, drawList, "Crit checks %i", critChecks);
	renderText(pos, yellow, offset, drawList, "Last command number %.4f", lastCommandNumberScanned);
	renderText(pos, red, offset, drawList, "Wait %.2fs", memory->globalVars->serverTime());
	renderText(pos, yellow, offset, drawList, "Wait %.2fs", activeWeapon->lastRapidFireCritCheckTime());
	*/

	if (weaponCriticals->getInt() <= 0)
	{
		renderText(pos, red, offset, drawList, "Server doesnt allow crits");
		return;
	}

	if (localPlayer->isCritBoosted())
	{
		renderText(pos, blue, offset, drawList, "Crit boosted");
		return;
	}

	if (!canActiveWeaponRandomCrit)
	{
		renderText(pos, red, offset, drawList, "Weapon cannot crit");
		return;
	}

	if (critTime > memory->globalVars->serverTime())
	{
		float ratio = std::clamp((critTime - memory->globalVars->serverTime()) / 2.f, 0.0f, 1.0f);
		renderText(pos, blue, offset, drawList, "Streaming crits %.2fs", ratio);
		renderBar(windowPos, ImVec2{ windowSize.x, 12.0f }, ratio, offset, drawList);
		return;
	}

	if (activeWeaponSlot != SLOT_MELEE && critBan)
	{
		renderText(pos, red, offset, drawList, "Crit banned");
		if(const int damage = getDamageTillUnban(); damage > 0)
			renderText(pos, red, offset, drawList, "Deal %i damage", damage);
		return;
	}	

	if (crits.empty() || skips.empty())
	{
		renderText(pos, yellow, offset, drawList, "Scanning for crits...");
		return;
	}

	if (isRapidFireCrits && critChecks < (critSeedRequests + 1) * 10)
	{
		renderText(pos, red, offset, drawList, "Crit throttled");
		renderText(pos, red, offset, drawList, "Unbalanced crit ratio");

		const float checks = static_cast<float>(critChecks);
		const float required = (critSeedRequests + 1) * 10.f;
		if (checks > 0.0f)
			renderText(pos, red, offset, drawList, "%.0f/100", (checks / required) * 100.f);

		if (lastRapidFireCritCheckTime + 1.f > memory->globalVars->serverTime())
		{
			const float fraction = std::clamp(((lastRapidFireCritCheckTime + 1.f) - memory->globalVars->serverTime()), 0.0f, 1.0f);
			renderText(pos, red, offset, drawList, "Wait %.2fs", fraction);
			renderBar(windowPos, ImVec2{ windowSize.x, 12.0f }, fraction, offset, drawList);
		}
		return;
	}

	if (cost > critTokenBucket)
	{
		renderText(pos, red, offset, drawList, "Cannot crit");
		renderText(pos, red, offset, drawList, "Cost: %i > Tokens: %i", static_cast<int>(cost), static_cast<int>(critTokenBucket));
		return;
	}

	if (lastRapidFireCritCheckTime + 1.f > memory->globalVars->serverTime())
	{
		renderText(pos, red, offset, drawList, "Cannot crit");
		const float fraction = std::clamp(((lastRapidFireCritCheckTime + 1.f) - memory->globalVars->serverTime()), 0.0f, 1.0f);
		renderText(pos, red, offset, drawList, "Wait %.2fs", fraction);
		renderBar(windowPos, ImVec2{ windowSize.x, 12.0f }, fraction, offset, drawList);
		return;
	}

	//maybe add safe damage?
	renderText(pos, green, offset, drawList, "Can crit");
	renderText(pos, green, offset, drawList, "%i/%i Potential crits", potentialCrits, critsPossible);
	
	if (potentialCrits < critsPossible && activeWeaponSlot != SLOT_MELEE && cost > 0.0f)
	{
		const float fraction = std::clamp((critTokenBucket - (static_cast<float>(potentialCrits) * cost)) / cost, 0.0f, 1.0f);
		 
		renderBar(windowPos, ImVec2{ windowSize.x, 12.0f }, fraction, offset, drawList);
	}
}

void Crithack::updatePlayers() noexcept
{
	if (!localPlayer)
		return;

	const auto playerResource = StrayElements::getPlayerResource();
	if (!playerResource)
		return;

	for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
	{
		const auto entity = interfaces->entityList->getEntity(i);
		if (!entity || !entity->isPlayer() || entity == localPlayer.get())
			continue;

		const auto player = playerHealthInfoByHandle(entity->handle());
		if (const auto player = playerHealthInfoByHandle(entity->handle())) {
			player->syncedHealth = playerResource->getHealth(entity->index());
		}
		else {
			PlayerHealthInfo newPlayer{ entity->handle(), playerResource->getHealth(entity->index()) };
			playerHealthInfo.emplace_back(newPlayer);
		}
	}
}

void Crithack::updateHealth(int handle, int newHealth) noexcept
{
	if (const auto& player = playerHealthInfoByHandle(handle))
	{
		if(newHealth > player->syncedHealth)
			player->syncedHealth = newHealth;
		player->newHealth = newHealth;
	}
}

void Crithack::setCorrectDamage(float damage) noexcept
{
	correctDamage = damage;
}

bool Crithack::protectData() noexcept
{
	return protect;
}

void Crithack::updateInput() noexcept
{
	config->misc.forceCritKey.handleToggle();
}

void Crithack::reset() noexcept
{
	protect = false;
	canCrit = false;
	critBan = false;

	correctCritChance = 0.0f;

	wishFireRandomSeed = 0;

	critTicks.clear();
	skipTicks.clear();

	correctDamage = 0.0f;
	rangedDamage = 0.f;
	critDamage = 0.f;
	boostedDamage = 0.f;
	meleeDamage = 0.f;

	playerHealthInfo.clear();

	lastCommandNumberScanned = 0;

	canActiveWeaponRandomCrit = false;
	activeWeaponIndex = 0;
	activeWeaponSlot = 0;

	cost = 0.0f;
	potentialCrits = 0;

	critTime = 0.0f;
	isRapidFireCrits = false;
	critChecks = 0;
	critSeedRequests = 0;
	critTokenBucket = 0.0f;
	lastRapidFireCritCheckTime = 0.0f;
}