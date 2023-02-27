#define NOMINMAX

#include "../imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui_internal.h"

#include "../Config.h"
#include "../fnv.h"
#include "../GameData.h"
#include "../Helpers.h"
#include "../Memory.h"

#include "StreamProofESP.h"

#include "../SDK/Engine.h"
#include "../SDK/GlobalVars.h"

#include <limits>
#include <tuple>

static constexpr auto operator-(float sub, const std::array<float, 3>& a) noexcept
{
    return Vector{ sub - a[0], sub - a[1], sub - a[2] };
}

struct BoundingBox {
private:
    bool valid;
public:
    ImVec2 min, max;
    std::array<ImVec2, 8> vertices;

    BoundingBox(const Vector& mins, const Vector& maxs, const std::array<float, 3>& scale, const matrix3x4* matrix = nullptr) noexcept
    {
        min.y = min.x = std::numeric_limits<float>::max();
        max.y = max.x = -std::numeric_limits<float>::max();

        const auto scaledMins = mins + (maxs - mins) * 2 * (0.25f - scale);
        const auto scaledMaxs = maxs - (maxs - mins) * 2 * (0.25f - scale);

        for (int i = 0; i < 8; ++i) {
            const Vector point{ i & 1 ? scaledMaxs.x : scaledMins.x,
                                i & 2 ? scaledMaxs.y : scaledMins.y,
                                i & 4 ? scaledMaxs.z : scaledMins.z };

            if (!Helpers::worldToScreen(matrix ? point.transform(*matrix) : point, vertices[i])) {
                valid = false;
                return;
            }

            min.x = std::min(min.x, vertices[i].x);
            min.y = std::min(min.y, vertices[i].y);
            max.x = std::max(max.x, vertices[i].x);
            max.y = std::max(max.y, vertices[i].y);
        }
        valid = true;
    }

    BoundingBox(const BaseData& data, const std::array<float, 3>& scale) noexcept : BoundingBox{ data.obbMins, data.obbMaxs, scale, &data.coordinateFrame } {}
    BoundingBox(const Vector& center) noexcept : BoundingBox{ center - 2.0f, center + 2.0f, { 0.25f, 0.25f, 0.25f } } {}

    operator bool() const noexcept
    {
        return valid;
    }
};

static ImDrawList* drawList;

static void addLineWithShadow(const ImVec2& p1, const ImVec2& p2, ImU32 col) noexcept
{
    drawList->AddLine(p1 + ImVec2{ 1.0f, 1.0f }, p2 + ImVec2{ 1.0f, 1.0f }, col & IM_COL32_A_MASK);
    drawList->AddLine(p1, p2, col);
}

// convex hull using Graham's scan
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

static void addRectFilled(const ImVec2& p1, const ImVec2& p2, ImU32 col, bool shadow) noexcept
{
    if (shadow)
        drawList->AddRectFilled(p1 + ImVec2{ 1.0f, 1.0f }, p2 + ImVec2{ 1.0f, 1.0f }, col & IM_COL32_A_MASK);
    drawList->AddRectFilled(p1, p2, col);
}

static void renderBox(const BoundingBox& bbox, const Box& config) noexcept
{
    if (!config.enabled)
        return;

    const ImU32 color = Helpers::calculateColor(config);
    const ImU32 fillColor = Helpers::calculateColor(config.fill);

    switch (config.type) {
    case Box::_2d:
        if (config.fill.enabled)
            drawList->AddRectFilled(bbox.min + ImVec2{ 1.0f, 1.0f }, bbox.max - ImVec2{ 1.0f, 1.0f }, fillColor, config.rounding, ImDrawFlags_RoundCornersAll);
        else
            drawList->AddRect(bbox.min + ImVec2{ 1.0f, 1.0f }, bbox.max + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK, config.rounding, ImDrawFlags_RoundCornersAll);
        drawList->AddRect(bbox.min, bbox.max, color, config.rounding, ImDrawFlags_RoundCornersAll);
        break;
    case Box::_2dCorners: {
        if (config.fill.enabled) {
            drawList->AddRectFilled(bbox.min + ImVec2{ 1.0f, 1.0f }, bbox.max - ImVec2{ 1.0f, 1.0f }, fillColor, config.rounding, ImDrawFlags_RoundCornersAll);
        }

        const bool wantsShadow = !config.fill.enabled;

        const auto quarterWidth = IM_FLOOR((bbox.max.x - bbox.min.x) * 0.25f);
        const auto quarterHeight = IM_FLOOR((bbox.max.y - bbox.min.y) * 0.25f);

        addRectFilled(bbox.min, { bbox.min.x + 1.0f, bbox.min.y + quarterHeight }, color, wantsShadow);
        addRectFilled(bbox.min, { bbox.min.x + quarterWidth, bbox.min.y + 1.0f }, color, wantsShadow);

        addRectFilled({ bbox.max.x, bbox.min.y }, { bbox.max.x - quarterWidth, bbox.min.y + 1.0f }, color, wantsShadow);
        addRectFilled({ bbox.max.x - 1.0f, bbox.min.y }, { bbox.max.x, bbox.min.y + quarterHeight }, color, wantsShadow);

        addRectFilled({ bbox.min.x, bbox.max.y }, { bbox.min.x + 1.0f, bbox.max.y - quarterHeight }, color, wantsShadow);
        addRectFilled({ bbox.min.x, bbox.max.y - 1.0f }, { bbox.min.x + quarterWidth, bbox.max.y }, color, wantsShadow);

        addRectFilled(bbox.max, { bbox.max.x - quarterWidth, bbox.max.y - 1.0f }, color, wantsShadow);
        addRectFilled(bbox.max, { bbox.max.x - 1.0f, bbox.max.y - quarterHeight }, color, wantsShadow);
        break;
    }
    case Box::_3d:
        if (config.fill.enabled) {
            auto [hull, count] = convexHull(bbox.vertices);
            std::reverse(hull.begin(), hull.begin() + count); // make them clockwise for antialiasing
            drawList->AddConvexPolyFilled(hull.data(), count, fillColor);
        } else {
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j))
                        drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK);
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j))
                    drawList->AddLine(bbox.vertices[i], bbox.vertices[i + j], color);
            }
        }
        break;
    case Box::_3dCorners:
        if (config.fill.enabled) {
            auto [hull, count] = convexHull(bbox.vertices);
            std::reverse(hull.begin(), hull.begin() + count); // make them clockwise for antialiasing
            drawList->AddConvexPolyFilled(hull.data(), count, fillColor);
        } else {
            for (int i = 0; i < 8; ++i) {
                for (int j = 1; j <= 4; j <<= 1) {
                    if (!(i & j)) {
                        drawList->AddLine(bbox.vertices[i] + ImVec2{ 1.0f, 1.0f }, ImVec2{ bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f } + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK);
                        drawList->AddLine(ImVec2{ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f } + ImVec2{ 1.0f, 1.0f }, bbox.vertices[i + j] + ImVec2{ 1.0f, 1.0f }, color & IM_COL32_A_MASK);
                    }
                }
            }
        }

        for (int i = 0; i < 8; ++i) {
            for (int j = 1; j <= 4; j <<= 1) {
                if (!(i & j)) {
                    drawList->AddLine(bbox.vertices[i], { bbox.vertices[i].x * 0.75f + bbox.vertices[i + j].x * 0.25f, bbox.vertices[i].y * 0.75f + bbox.vertices[i + j].y * 0.25f }, color);
                    drawList->AddLine({ bbox.vertices[i].x * 0.25f + bbox.vertices[i + j].x * 0.75f, bbox.vertices[i].y * 0.25f + bbox.vertices[i + j].y * 0.75f }, bbox.vertices[i + j], color);
                }
            }
        }
        break;
    }
}

static ImVec2 renderText(float distance, float cullDistance, const Color4& textCfg, const char* text, const ImVec2& pos, bool centered = true, bool adjustHeight = true) noexcept
{
    if (cullDistance && Helpers::units2meters(distance) > cullDistance)
        return { };

    const auto textSize = ImGui::CalcTextSize(text);

    const auto horizontalOffset = centered ? textSize.x / 2 : 0.0f;
    const auto verticalOffset = adjustHeight ? textSize.y : 0.0f;

    const auto color = Helpers::calculateColor(textCfg);
    drawList->AddText({ pos.x - horizontalOffset + 1.0f, pos.y - verticalOffset + 1.0f }, color & IM_COL32_A_MASK, text);
    drawList->AddText({ pos.x - horizontalOffset, pos.y - verticalOffset }, color, text);

    return textSize;
}

static void drawSnapline(const Snapline& config, const ImVec2& min, const ImVec2& max) noexcept
{
    if (!config.enabled)
        return;

    const auto& screenSize = ImGui::GetIO().DisplaySize;

    ImVec2 p1, p2;
    p1.x = screenSize.x / 2;
    p2.x = (min.x + max.x) / 2;

    switch (config.type) {
    case Snapline::Bottom:
        p1.y = screenSize.y;
        p2.y = max.y;
        break;
    case Snapline::Top:
        p1.y = 0.0f;
        p2.y = min.y;
        break;
    case Snapline::Crosshair:
        p1.y = screenSize.y / 2;
        p2.y = (min.y + max.y) / 2;
        break;
    default:
        return;
    }

    drawList->AddLine(p1, p2, Helpers::calculateColor(config), config.thickness);
}

struct FontPush {
    FontPush(const std::string& name, float distance)
    {
        if (const auto it = config->getFonts().find(name); it != config->getFonts().end()) {
            distance *= GameData::local().fov / 90.0f;

            ImGui::PushFont([](const Config::Font& font, float dist) {
                if (dist <= 400.0f)
                    return font.big;
                if (dist <= 1000.0f)
                    return font.medium;
                return font.tiny;
            }(it->second, distance));
        }
        else {
            ImGui::PushFont(nullptr);
        }
    }

    ~FontPush()
    {
        ImGui::PopFont();
    }
};

static void drawHealthBar(const HealthBar& config, const ImVec2& pos, float height, int health, int maxHealth) noexcept
{
    if (!config.enabled)
        return;

    constexpr float width = 3.0f;

    drawList->PushClipRect(pos + ImVec2{ 0.0f, ((static_cast<float>(maxHealth) - static_cast<float>(health)) / static_cast<float>(maxHealth)) * height }, pos + ImVec2{ width + 1.0f, height + 1.0f });

    if (config.type == HealthBar::Gradient) {
        const auto green = Helpers::calculateColor(0, 255, 0, 255);
        const auto yellow = Helpers::calculateColor(255, 255, 0, 255);
        const auto red = Helpers::calculateColor(255, 0, 0, 255);

        ImVec2 min = pos;
        ImVec2 max = min + ImVec2{ width, height / 2.0f };

        drawList->AddRectFilled(min + ImVec2{ 1.0f, 1.0f }, pos + ImVec2{ width + 1.0f, height + 1.0f }, Helpers::calculateColor(0, 0, 0, 255));

        drawList->AddRectFilledMultiColor(ImFloor(min), ImFloor(max), green, green, yellow, yellow);
        min.y += height / 2.0f;
        max.y += height / 2.0f;
        drawList->AddRectFilledMultiColor(ImFloor(min), ImFloor(max), yellow, yellow, red, red);
    } else {
        const auto color = config.type == HealthBar::HealthBased ? Helpers::healthColor(std::clamp(static_cast<float>(health) / static_cast<float>(maxHealth), 0.0f, 1.0f)) : Helpers::calculateColor(config);
        drawList->AddRectFilled(pos + ImVec2{ 1.0f, 1.0f }, pos + ImVec2{ width + 1.0f, height + 1.0f }, color & IM_COL32_A_MASK);
        drawList->AddRectFilled(pos, pos + ImVec2{ width, height }, color);
    }

    drawList->PopClipRect();
}

static void renderPlayerBox(const PlayerData& playerData, const Player& config) noexcept
{
    const BoundingBox bbox{ playerData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);

    ImVec2 offsetMins{}, offsetMaxs{};

    const auto height = (bbox.max.y - bbox.min.y);
    drawHealthBar(config.healthBar, bbox.min - ImVec2{ 5.0f, 0.0f }, height, playerData.health, playerData.maxHealth);

    FontPush font{ config.font.name, playerData.distanceToLocal };

    if (config.healthBar.enabled)
    {
        const auto textSize = ImGui::CalcTextSize(std::to_string(playerData.health).c_str());
        const auto position = bbox.min - ImVec2{ 5.0f + (textSize.x/2.0f), 0.0f } + ImVec2{ 0.0f, std::clamp((static_cast<float>(playerData.maxHealth) - static_cast<float>(playerData.health)) / static_cast<float>(playerData.maxHealth), 0.0f, 1.0f) * height };
        renderText(playerData.distanceToLocal, config.textCullDistance, Color4(), std::to_string(playerData.health).c_str(), { position.x , position.y });
    }

    if (config.name.enabled) {
        const auto nameSize = renderText(playerData.distanceToLocal, config.textCullDistance, config.name, playerData.name.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 2 });
        offsetMins.y -= nameSize.y + 2;
    }

    if (config.weapon.enabled && !playerData.activeWeapon.empty()) {
        const auto weaponTextSize = renderText(playerData.distanceToLocal, config.textCullDistance, config.weapon, playerData.activeWeapon.c_str(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y + 1 }, true, false);
        offsetMaxs.y += weaponTextSize.y + 2.0f;
    }

    drawSnapline(config.snapline, bbox.min + offsetMins, bbox.max + offsetMaxs);
}

static void renderBuildingBox(const BuildingsData& buildingData, const Buildings& config) noexcept
{
    const BoundingBox bbox{ buildingData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(config.snapline, bbox.min, bbox.max);

    const auto height = (bbox.max.y - bbox.min.y);
    drawHealthBar(config.healthBar, bbox.min - ImVec2{ 5.0f, 0.0f }, height, buildingData.health, buildingData.maxHealth);

    FontPush font{ config.font.name, buildingData.distanceToLocal };

    if (config.healthBar.enabled)
    {
        const auto textSize = ImGui::CalcTextSize(std::to_string(buildingData.health).c_str());
        const auto position = bbox.min - ImVec2{ 5.0f + (textSize.x / 2.0f), 0.0f } + ImVec2{ 0.0f, std::clamp((static_cast<float>(buildingData.maxHealth) - static_cast<float>(buildingData.health)) / static_cast<float>(buildingData.maxHealth), 0.0f, 1.0f) * height };
        renderText(buildingData.distanceToLocal, config.textCullDistance, Color4(), std::to_string(buildingData.health).c_str(), { position.x , position.y });
    }

    if (config.name.enabled)
        renderText(buildingData.distanceToLocal, config.textCullDistance, config.name, buildingData.name.data(), { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 2 });

    if (config.owner.enabled)
        renderText(buildingData.distanceToLocal, config.textCullDistance, config.name, buildingData.owner.data(), { (bbox.min.x + bbox.max.x) / 2, bbox.max.y - 5 });
}

static void renderEntityBox(const BaseData& entityData, const char* name, const Shared& config) noexcept
{
    const BoundingBox bbox{ entityData, config.box.scale };

    if (!bbox)
        return;

    renderBox(bbox, config.box);
    drawSnapline(config.snapline, bbox.min, bbox.max);

    FontPush font{ config.font.name, entityData.distanceToLocal };

    if (config.name.enabled)
        renderText(entityData.distanceToLocal, config.textCullDistance, config.name, name, { (bbox.min.x + bbox.max.x) / 2, bbox.min.y - 5 });
}

static void drawPlayerSkeleton(const ColorToggleThickness& config, const std::vector<std::pair<Vector, Vector>>& bones) noexcept
{
    if (!config.enabled)
        return;

    const auto color = Helpers::calculateColor(config);

    std::vector<std::pair<ImVec2, ImVec2>> points, shadowPoints;

    for (const auto& [bone, parent] : bones) {
        ImVec2 bonePoint;
        if (!Helpers::worldToScreen(bone, bonePoint))
            continue;

        ImVec2 parentPoint;
        if (!Helpers::worldToScreen(parent, parentPoint))
            continue;

        points.emplace_back(bonePoint, parentPoint);
        shadowPoints.emplace_back(bonePoint + ImVec2{ 1.0f, 1.0f }, parentPoint + ImVec2{ 1.0f, 1.0f });
    }

    for (const auto& [bonePoint, parentPoint] : shadowPoints)
        drawList->AddLine(bonePoint, parentPoint, color & IM_COL32_A_MASK, config.thickness);

    for (const auto& [bonePoint, parentPoint] : points)
        drawList->AddLine(bonePoint, parentPoint, color, config.thickness);
}

static bool renderPlayerEsp(const PlayerData& playerData, const Player& playerConfig) noexcept
{
    if (!playerConfig.enabled)
        return false;

    if (playerData.isCloaked && playerConfig.disableOnCloaked)
        return false;

    Helpers::setAlphaFactor(Helpers::getAlphaFactor() * playerData.fadingAlpha());

    renderPlayerBox(playerData, playerConfig);
    drawPlayerSkeleton(playerConfig.skeleton, playerData.bones);

    Helpers::setAlphaFactor(1.0f);

    return true;
}

static bool renderBuildingEsp(const BuildingsData& buildingData, const Buildings& buildingConfig) noexcept
{
    if (!buildingConfig.enabled)
        return false;

    renderBuildingBox(buildingData, buildingConfig);
    return true;
}

static void renderEntityEsp(const BaseData& entityData, const std::unordered_map<std::string, Shared>& map, const char* name) noexcept
{
    if (const auto cfg = map.find(name); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    } else if (const auto cfg = map.find("All"); cfg != map.cend() && cfg->second.enabled) {
        renderEntityBox(entityData, name, cfg->second);
    }
}

void StreamProofESP::render() noexcept
{
    if (!config->streamProofESP.key.isActive())
        return;

    drawList = ImGui::GetBackgroundDrawList();

    GameData::Lock lock;

    for (const auto& player : GameData::players()) {
        if ((player.dormant && player.fadingAlpha() == 0.0f) || !player.alive || !player.inViewFrustum)
            continue;

        auto& playerConfig = player.enemy ? config->streamProofESP.enemies : config->streamProofESP.allies;
        renderPlayerEsp(player, playerConfig["All"]);
    }

    for (const auto& building : GameData::buildings()) {
        if (!building.alive)
            continue;

        if(!renderBuildingEsp(building, config->streamProofESP.buildings["All"]))
            renderBuildingEsp(building, building.enemy ? config->streamProofESP.buildings["Enemies"] : config->streamProofESP.buildings["Allies"]);
    }

    for (const auto& world : GameData::world()) {
        if (config->streamProofESP.world["All"].enabled)
            break;

        renderEntityBox(world, world.name.data(), config->streamProofESP.world["All"]);
    }
}

void StreamProofESP::updateInput() noexcept
{
    config->streamProofESP.key.handleToggle();
}
