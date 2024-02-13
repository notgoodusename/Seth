#include "ProjectileTrajectory.h"
#include "ProjectileSimulation.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/LocalPlayer.h"

#include "../Config.h"
#include "../Helpers.h"
#include "../Interfaces.h"

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include <vector>

std::vector<Vector> projectilePositions;
std::mutex mutex;

void ProjectileTrajectory::calculate(UserCmd* cmd) noexcept
{
	mutex.lock();

	projectilePositions.clear();

	if (!config->visuals.projectileTrajectory.enabled)
	{
		mutex.unlock();
		return;
	}

	if (!localPlayer || !localPlayer->isAlive())
	{
		mutex.unlock();
		return;
	}

	const auto activeWeapon = localPlayer->getActiveWeapon();
	if (!activeWeapon)
	{
		mutex.unlock();
		return;
	}

	const auto weaponType = activeWeapon->getWeaponType();
	if (weaponType != WeaponType::PROJECTILE)
	{
		mutex.unlock();
		return;
	}

	const auto projectileWeaponInfo = ProjectileSimulation::getProjectileWeaponInfo(localPlayer.get(), activeWeapon, cmd->viewangles);
	if (!ProjectileSimulation::init(projectileWeaponInfo))
	{
		mutex.unlock();
		return;
	}

	if (projectileWeaponInfo.offset.y == 0.0f 
		|| projectileWeaponInfo.weaponId == WeaponId::FLAMETHROWER 
		|| projectileWeaponInfo.weaponId == WeaponId::FLAMETHROWER_ROCKET
		|| projectileWeaponInfo.weaponId == WeaponId::FLAME_BALL)
	{
		mutex.unlock();
		return;
	}

	const int maxTicks = timeToTicks(projectileWeaponInfo.maxTime == 0.f ? 7.5f : projectileWeaponInfo.maxTime);

	//start pos
	projectilePositions.push_back(ProjectileSimulation::getOrigin());
	//TODO: check for hull sizes correctly!! rockets are just raycasts!
	for (int step = 1; step <= maxTicks; step++)
	{
		ProjectileSimulation::runTick();
		
		Vector origin = ProjectileSimulation::getOrigin();

		Trace trace;
		//compare last pos with current one, if hit we break!
		interfaces->engineTrace->traceRay({ projectilePositions[projectilePositions.size() - 1U], origin, Vector{ -2.0f, -2.0f, -2.0f }, Vector{ 2.0f, 2.0f, 2.0f } }, MASK_SOLID, TraceFilterArc{ localPlayer.get() }, trace);

		if (trace.didHit())
			origin = trace.endpos;

		projectilePositions.push_back(origin);
		if (trace.didHit())
			break;
	}
	mutex.unlock();
}

static std::pair<std::array<ImVec2, 8>, std::size_t> convexHull(std::array<ImVec2, 8> points) noexcept
{
	std::swap(points[0], *std::min_element(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.y < b.y || (a.y == b.y && a.x < b.x); }));

	constexpr auto orientation = [](const ImVec2& a, const ImVec2& b, const ImVec2& c) {
		return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
	};

	std::sort(points.begin() + 1, points.end(), [&](const auto& a, const auto& b) {
		const auto o = orientation(points[0], a, b);
		return o == 0.0f ? ImLengthSqr(points[0] - a) < ImLengthSqr(points[0] - b) : o < 0.0f;
		});

	std::array<ImVec2, 8> hull;
	std::size_t count = 0;

	for (const auto& p : points) {
		while (count >= 2 && orientation(hull[count - 2], hull[count - 1], p) >= 0.0f)
			--count;
		hull[count++] = p;
	}

	return std::make_pair(hull, count);
}

void ProjectileTrajectory::draw(ImDrawList* drawList) noexcept
{
	if (!config->visuals.projectileTrajectory.enabled)
		return;

	if (!localPlayer || !localPlayer->isAlive())
		return;

	const auto trailColor = Helpers::calculateColor(config->visuals.projectileTrajectory.trailColor);
	const auto bboxColor = Helpers::calculateColor(config->visuals.projectileTrajectory.bboxColor);

	static std::vector<Vector> savedPositions;

	if (mutex.try_lock())
	{
		savedPositions = projectilePositions;

		mutex.unlock();
	}

	if (savedPositions.empty() || savedPositions.size() <= 1U)
		return;

	std::vector<ImVec2> drawPositions;

	for (const auto& projectilePosition : savedPositions)
	{
		ImVec2 drawPosition;
		if (Helpers::worldToScreen(projectilePosition, drawPosition, false))
			drawPositions.push_back(drawPosition);
	}

	for (int i = 0; i < static_cast<int>(drawPositions.size() - 1U); i++)
	{
		const auto& oldPosition = drawPositions[i];
		const auto& newPosition = drawPositions[i + 1];
		drawList->AddLine(oldPosition, newPosition, trailColor & IM_COL32_A_MASK, 2.0f);
		drawList->AddLine(oldPosition, newPosition, trailColor, 1.5f);
	}

	if (savedPositions.back().notNull())
	{
		std::array<ImVec2, 8> vertices;

		const Vector maxs = savedPositions.back() + Vector{ 2.0f, 2.0f, 2.0f };
		const Vector mins = savedPositions.back() + Vector{ -2.0f, -2.0f, -2.0f };
		for (int i = 0; i < 8; ++i) {
			const Vector point{ i & 1 ? maxs.x : mins.x,
								i & 2 ? maxs.y : mins.y,
								i & 4 ? maxs.z : mins.z };

			if (!Helpers::worldToScreen(point, vertices[i]))
				return;
		}

		auto [hull, count] = convexHull(vertices);
		std::reverse(hull.begin(), hull.begin() + count);
		drawList->AddConvexPolyFilled(hull.data(), count, bboxColor);

		for (int i = 0; i < 8; ++i) {
			for (int j = 1; j <= 4; j <<= 1) {
				if (!(i & j))
					drawList->AddLine(vertices[i], vertices[i + j], bboxColor);
			}
		}
	}
}