#include <array>
#include <cwctype>
#include <fstream>
#include <functional>
#include <string>
#include <ShlObj.h>
#include <Windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_stdlib.h"

#include "imguiCustom.h"

#include "Hacks/Misc.h"

#include "GUI.h"
#include "Config.h"
#include "Helpers.h"
#include "Hooks.h"
#include "Interfaces.h"

#include "SDK/InputSystem.h"

constexpr auto windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

static ImFont* addFontFromVFONT(const std::string& path, float size, const ImWchar* glyphRanges, bool merge) noexcept
{
    auto file = Helpers::loadBinaryFile(path);
    if (!Helpers::decodeVFONT(file))
        return nullptr;

    ImFontConfig cfg;
    cfg.FontData = file.data();
    cfg.FontDataSize = file.size();
    cfg.FontDataOwnedByAtlas = false;
    cfg.MergeMode = merge;
    cfg.GlyphRanges = glyphRanges;
    cfg.SizePixels = size;

    return ImGui::GetIO().Fonts->AddFont(&cfg);
}

GUI::GUI() noexcept
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScrollbarSize = 11.5f;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImFontConfig cfg;
    cfg.SizePixels = 15.0f;

    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        const std::filesystem::path path{ pathToFonts };
        CoTaskMemFree(pathToFonts);

        fonts.normal15px = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 15.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.normal15px)
            io.Fonts->AddFontDefault(&cfg);

        fonts.tahoma34 = io.Fonts->AddFontFromFileTTF((path / "tahoma.ttf").string().c_str(), 34.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma34)
            io.Fonts->AddFontDefault(&cfg);

        fonts.tahoma28 = io.Fonts->AddFontFromFileTTF((path / "tahomabd.ttf").string().c_str(), 28.0f, &cfg, Helpers::getFontGlyphRanges());
        if (!fonts.tahoma28)
            io.Fonts->AddFontDefault(&cfg);

        cfg.MergeMode = true;
        static constexpr ImWchar symbol[]{
            0x2605, 0x2605, // ★
            0
        };
        io.Fonts->AddFontFromFileTTF((path / "seguisym.ttf").string().c_str(), 15.0f, &cfg, symbol);
        cfg.MergeMode = false;
    }

    if (!fonts.normal15px)
        io.Fonts->AddFontDefault(&cfg);
    if (!fonts.tahoma28)
        io.Fonts->AddFontDefault(&cfg);
    if (!fonts.tahoma34)
        io.Fonts->AddFontDefault(&cfg);
    addFontFromVFONT("csgo/panorama/fonts/notosanskr-regular.vfont", 15.0f, io.Fonts->GetGlyphRangesKorean(), true);
    addFontFromVFONT("csgo/panorama/fonts/notosanssc-regular.vfont", 15.0f, io.Fonts->GetGlyphRangesChineseFull(), true);
    constexpr auto unicodeFontSize = 16.0f;
    fonts.unicodeFont = addFontFromVFONT("csgo/panorama/fonts/notosans-bold.vfont", unicodeFontSize, Helpers::getFontGlyphRanges(), false);
}

void GUI::render() noexcept
{
    renderGuiStyle();
}

#include "InputUtil.h"

static void hotkey3(const char* label, KeyBind& key, float samelineOffset = 0.0f, const ImVec2& size = { 100.0f, 0.0f }) noexcept
{
    const auto id = ImGui::GetID(label);
    ImGui::PushID(label);

    ImGui::TextUnformatted(label);
    ImGui::SameLine(samelineOffset);

    if (ImGui::GetActiveID() == id) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_ButtonActive));
        ImGui::Button("...", size);
        ImGui::PopStyleColor();

        ImGui::GetCurrentContext()->ActiveIdAllowOverlap = true;
        if ((!ImGui::IsItemHovered() && ImGui::GetIO().MouseClicked[0]) || key.setToPressedKey())
            ImGui::ClearActiveID();
    }
    else if (ImGui::Button(key.toString(), size)) {
        ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
    }

    ImGui::PopID();
}

ImFont* GUI::getTahoma28Font() const noexcept
{
    return fonts.tahoma28;
}

ImFont* GUI::getUnicodeFont() const noexcept
{
    return fonts.unicodeFont;
}

void GUI::handleToggle() noexcept
{
    if (config->misc.menuKey.isPressed()) {
        open = !open;
        if (!open)
            interfaces->inputSystem->resetInputState();
    }
}

static void menuBarItem(const char* name, bool& enabled) noexcept
{
    if (ImGui::MenuItem(name)) {
        enabled = true;
        ImGui::SetWindowFocus(name);
        ImGui::SetWindowPos(name, { 100.0f, 100.0f });
    }
}

void GUI::renderAimbotWindow() noexcept
{
    static const char* hitboxes[]{ "Head","Body","Pelvis" };
    static bool hitbox[ARRAYSIZE(hitboxes)] = { false, false, false };
    static std::string previewvalue = "";
    bool once = false;

    ImGui::PushID("Key");
    ImGui::hotkey2("Key", config->aimbotKey);
    ImGui::PopID();
    ImGui::Separator();
    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "Hitscan\0Projectile\0Melee\0");
    ImGui::PopID();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 230.0f);

    switch (currentCategory)
    {
        case 0:
        {
            ImGui::SameLine();
            ImGui::Checkbox("Enabled", &config->aimbot.hitscan.enabled);
            ImGui::Checkbox("Aimlock", &config->aimbot.hitscan.aimlock);
            ImGui::Checkbox("Silent", &config->aimbot.hitscan.silent);
            ImGuiCustom::colorPicker("Draw fov", config->aimbotFov);
            ImGui::Checkbox("Friendly fire", &config->aimbot.hitscan.friendlyFire);
            ImGui::Checkbox("Target backtrack", &config->aimbot.hitscan.targetBacktrack);
            ImGui::Checkbox("Ignore cloaked", &config->aimbot.hitscan.ignoreCloaked);
            ImGui::Checkbox("Scoped only", &config->aimbot.hitscan.scopedOnly);
            ImGui::Checkbox("Auto shoot", &config->aimbot.hitscan.autoShoot);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("This option disables smoothing, if you want to keep it use triggerbot");
            ImGui::Checkbox("Auto scope", &config->aimbot.hitscan.autoScope);
            ImGui::Checkbox("Auto rev", &config->aimbot.hitscan.autoRev);
            ImGui::Checkbox("Auto extinguish team", &config->aimbot.hitscan.autoExtinguishTeam);
            ImGui::Checkbox("Wait for headshot", &config->aimbot.hitscan.waitForHeadshot);
            ImGui::Checkbox("Wait for charge", &config->aimbot.hitscan.waitForHeadshot);

            ImGui::Combo("Sort method", &config->aimbot.hitscan.sortMethod, "Distance\0Fov\0");
            for (size_t i = 0; i < ARRAYSIZE(hitbox); i++)
            {
                hitbox[i] = (config->aimbot.hitscan.hitboxes & 1 << i) == 1 << i;
            }
            if (ImGui::BeginCombo("Hitbox", previewvalue.c_str()))
            {
                previewvalue = "";
                for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
                {
                    ImGui::Selectable(hitboxes[i], &hitbox[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                }
                ImGui::EndCombo();
            }
            for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
            {
                if (!once)
                {
                    previewvalue = "";
                    once = true;
                }
                if (hitbox[i])
                {
                    previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
                    config->aimbot.hitscan.hitboxes |= 1 << i;
                }
                else
                {
                    config->aimbot.hitscan.hitboxes &= ~(1 << i);
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(240.0f);
            ImGui::SliderFloat("Fov", &config->aimbot.hitscan.fov, 0.0f, 255.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Smooth", &config->aimbot.hitscan.smooth, 1.0f, 100.0f, "%.2f");
            break;
        }
        case 1:
            ImGui::SameLine();
            ImGui::Checkbox("Enabled", &config->aimbot.projectile.enabled);
            break;
        case 2:
        {
            ImGui::SameLine();
            ImGui::Checkbox("Enabled", &config->aimbot.melee.enabled);
            ImGui::Checkbox("Aimlock", &config->aimbot.melee.aimlock);
            ImGui::Checkbox("Silent", &config->aimbot.melee.silent);
            ImGui::Checkbox("Friendly fire", &config->aimbot.melee.friendlyFire);
            ImGui::Checkbox("Ignore cloaked", &config->aimbot.melee.ignoreCloaked);
            ImGui::Checkbox("Target backtrack", &config->aimbot.melee.targetBacktrack);
            ImGui::Checkbox("Auto hit", &config->aimbot.melee.autoHit);
            ImGui::Checkbox("Auto backstab", &config->aimbot.melee.autoBackstab);
            ImGui::Combo("Sort method", &config->aimbot.melee.sortMethod, "Distance\0Fov\0");
            ImGui::NextColumn();
            ImGui::PushItemWidth(240.0f);
            ImGui::SliderFloat("Fov", &config->aimbot.melee.fov, 0.0f, 255.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
            break;
        }
        default:
            break;
    }
}

void GUI::renderTriggerbotWindow() noexcept
{
    static const char* hitboxes[]{ "Head","Chest","Stomach","Arms","Legs" };
    static bool hitbox[ARRAYSIZE(hitboxes)] = { false, false, false, false, false };
    static std::string previewvalue = "";

    ImGui::hotkey2("Key", config->triggerbotKey, 80.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    ImGui::Combo("", &currentCategory, "Hitscan\0Projectile\0Melee\0");
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &config->triggerbot.enabled);
    switch (currentCategory)
    {
        case 0:
        {
            ImGui::Checkbox("Friendly fire", &config->triggerbot.friendlyFire);
            ImGui::Checkbox("Scoped only", &config->triggerbot.scopedOnly);
            ImGui::SetNextItemWidth(85.0f);

            for (size_t i = 0; i < ARRAYSIZE(hitbox); i++)
            {
                hitbox[i] = (config->triggerbot.hitboxes & 1 << i) == 1 << i;
            }

            if (ImGui::BeginCombo("Hitbox", previewvalue.c_str()))
            {
                previewvalue = "";
                for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
                {
                    ImGui::Selectable(hitboxes[i], &hitbox[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
                }
                ImGui::EndCombo();
            }
            for (size_t i = 0; i < ARRAYSIZE(hitboxes); i++)
            {
                if (i == 0)
                    previewvalue = "";

                if (hitbox[i])
                {
                    previewvalue += previewvalue.size() ? std::string(", ") + hitboxes[i] : hitboxes[i];
                    config->triggerbot.hitboxes |= 1 << i;
                }
                else
                {
                    config->triggerbot.hitboxes &= ~(1 << i);
                }
            }
            ImGui::PushItemWidth(220.0f);
            ImGui::PushItemWidth(220.0f);
            ImGui::SliderInt("Shot delay", &config->triggerbot.shotDelay, 0, 250, "%d ms");
        }
    default:
        break;
    }
}

void GUI::renderFakelagWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 300.f);
    ImGui::Checkbox("Enabled", &config->fakelag.enabled);
    ImGui::Combo("Mode", &config->fakelag.mode, "Static\0Adaptative\0Random\0");
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Limit", &config->fakelag.limit, 1, 16, "%d");
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::Columns(1);
}

void GUI::renderAntiAimWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 300.f);
    ImGui::Checkbox("Enabled", &config->antiAim.enabled);
    ImGui::Combo("Pitch", &config->antiAim.pitch, "Off\0Down\0Zero\0Up\0");
    ImGui::Combo("Yaw base", reinterpret_cast<int*>(&config->antiAim.yawBase), "Off\0Forward\0Backward\0Right\0Left\0Spin\0");
    ImGui::Combo("Yaw modifier", reinterpret_cast<int*>(&config->antiAim.yawModifier), "Off\0Jitter\0");
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Yaw add", &config->antiAim.yawAdd, -180, 180, "%d");
    ImGui::PopItemWidth();

    if (config->antiAim.yawModifier == 1) //Jitter
        ImGui::SliderInt("Jitter yaw range", &config->antiAim.jitterRange, 0, 90, "%d");

    if (config->antiAim.yawBase == Yaw::spin)
    {
        ImGui::PushItemWidth(220.0f);
        ImGui::SliderInt("Spin base", &config->antiAim.spinBase, -180, 180, "%d");
        ImGui::PopItemWidth();
    }

    ImGui::Checkbox("At targets", &config->antiAim.atTargets);

    ImGui::hotkey2("Forward", config->antiAim.manualForward, 60.f);
    ImGui::hotkey2("Backward", config->antiAim.manualBackward, 60.f);
    ImGui::hotkey2("Right", config->antiAim.manualRight, 60.f);
    ImGui::hotkey2("Left", config->antiAim.manualLeft, 60.f);

    ImGui::NextColumn();
    ImGui::Columns(1);
}

void GUI::renderBacktrackWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 300.f);
    ImGui::Checkbox("Enabled", &config->backtrack.enabled);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Time limit", &config->backtrack.timeLimit, 1, 200, "%d ms");
    ImGui::PopItemWidth();
    ImGui::NextColumn();
    ImGui::Checkbox("Enabled Fake Latency", &config->backtrack.fakeLatency);
    ImGui::PushItemWidth(220.0f);
    ImGui::SliderInt("Ping", &config->backtrack.fakeLatencyAmount, 1, 800, "%d ms");
    ImGui::PopItemWidth();
    ImGui::Columns(1);
}

void GUI::renderChamsWindow() noexcept
{
    ImGui::hotkey2("Key", config->chamsKey, 80.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);

    static int material = 1;
    constexpr std::array categories{ "Allies", "Enemies", "Local player", "Backtrack", "Fake angles", "Buildings", "World", "NPCs" };

    if (ImGui::Combo("", &currentCategory, "Allies\0Enemies\0Local player\0Backtrack\0Fake angles\0Buildings\0World\0NPCs\0"))
        material = 1;

    ImGui::PopID();

    Config::Chams* currentItem;

    if (currentCategory == 5) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Enemies\0Allies\0");
        ImGui::PopID();
        auto& cfg = config->buildingChams;
        switch (currentType) {
        case 0: currentItem = &cfg.all; break;
        case 1: currentItem = &cfg.enemies; break;
        case 2: currentItem = &cfg.allies; break;
        }
    }
    else if (currentCategory == 6)
    {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(2);
        ImGui::Combo("", &currentType, "All\0Ammo\0Health\0Other\0");
        ImGui::PopID();
        auto& cfg = config->worldChams;
        switch (currentType) {
        case 0: currentItem = &cfg.all; break;
        case 1: currentItem = &cfg.ammoPacks; break;
        case 2: currentItem = &cfg.healthPacks; break;
        case 3: currentItem = &cfg.other; break;
        }
    }
    else {
        currentItem = &config->chams[categories[currentCategory]];
    }

    ImGui::SameLine();

    if (material <= 1)
        ImGuiCustom::arrowButtonDisabled("##left", ImGuiDir_Left);
    else if (ImGui::ArrowButton("##left", ImGuiDir_Left))
        --material;

    ImGui::SameLine();
    ImGui::Text("%d", material);

    ImGui::SameLine();

    if (material >= int(currentItem->materials.size()))
        ImGuiCustom::arrowButtonDisabled("##right", ImGuiDir_Right);
    else if (ImGui::ArrowButton("##right", ImGuiDir_Right))
        ++material;

    ImGui::SameLine();

    auto& chams{ currentItem->materials[material - 1] };

    ImGui::Checkbox("Enabled", &chams.enabled);
    ImGui::Separator();
    if (currentCategory != 6 && currentCategory != 7)
        ImGui::Checkbox("Health based", &chams.healthBased);
    ImGui::Checkbox("Blinking", &chams.blinking);
    ImGui::Combo("Material", &chams.material, "Normal\0Flat\0");
    ImGui::Checkbox("Wireframe", &chams.wireframe);
    ImGui::Checkbox("Cover", &chams.cover);
    ImGui::Checkbox("Ignore-Z", &chams.ignorez);
    ImGuiCustom::colorPicker("Color", chams);
}

void GUI::renderGlowWindow() noexcept
{
    ImGui::hotkey2("Key", config->glowKey, 80.0f);
    ImGui::Separator();

    static int currentCategory{ 0 };
    ImGui::PushItemWidth(110.0f);
    ImGui::PushID(0);
    constexpr std::array categories{ "Allies", "Enemies", "Planting", "Defusing", "Local Player", "Weapons", "C4", "Planted C4", "Chickens", "Defuse Kits", "Projectiles", "Hostages" };
    ImGui::Combo("", &currentCategory, categories.data(), categories.size());
    ImGui::PopID();
    Config::GlowItem* currentItem;
    if (currentCategory <= 3) {
        ImGui::SameLine();
        static int currentType{ 0 };
        ImGui::PushID(1);
        ImGui::Combo("", &currentType, "All\0Visible\0Occluded\0");
        ImGui::PopID();
        auto& cfg = config->playerGlow[categories[currentCategory]];
        switch (currentType) {
        case 0: currentItem = &cfg.all; break;
        case 1: currentItem = &cfg.visible; break;
        case 2: currentItem = &cfg.occluded; break;
        }
    }
    else {
        currentItem = &config->glow[categories[currentCategory]];
    }

    ImGui::SameLine();
    ImGui::Checkbox("Enabled", &currentItem->enabled);
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 150.0f);
    ImGui::Checkbox("Health based", &currentItem->healthBased);

    ImGuiCustom::colorPicker("Color", *currentItem);

    ImGui::NextColumn();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::Combo("Style", &currentItem->style, "Default\0Rim3d\0Edge\0Edge Pulse\0");

    ImGui::Columns(1);
}

void GUI::renderStreamProofESPWindow() noexcept
{
    ImGui::hotkey2("Key", config->streamProofESP.key, 80.0f);
    ImGui::Separator();

    static std::size_t currentCategory;
    static auto currentItem = "All";

    constexpr auto getConfigShared = [](std::size_t category, const char* item) noexcept -> Shared& {
        switch (category) {
        case 0: default: return config->streamProofESP.enemies[item];
        case 1: return config->streamProofESP.allies[item];
        case 2: return config->streamProofESP.buildings[item];
        case 3: return config->streamProofESP.world[item];
        }
    };

    constexpr auto getConfigPlayer = [](std::size_t category, const char* item) noexcept -> Player& {
        switch (category) {
        case 0: default: return config->streamProofESP.enemies[item];
        case 1: return config->streamProofESP.allies[item];
        }
    };

    if (ImGui::BeginListBox("##list", { 170.0f, 300.0f })) {
        constexpr std::array categories{ "Enemies", "Allies", "Buildings", "World" };

        for (std::size_t i = 0; i < categories.size(); ++i) {
            if (ImGui::Selectable(categories[i], currentCategory == i && std::string_view{ currentItem } == "All")) {
                currentCategory = i;
                currentItem = "All";
            }

            if (ImGui::BeginDragDropSource()) {
                switch (i) {
                case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, "All"), sizeof(Player), ImGuiCond_Once); break;
                case 2: ImGui::SetDragDropPayload("Buildings", &config->streamProofESP.buildings["All"], sizeof(Buildings), ImGuiCond_Once); break;
                case 3: ImGui::SetDragDropPayload("World", &config->streamProofESP.world["All"], sizeof(World), ImGuiCond_Once); break;
                default: break;
                }
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                    const auto& data = *(Player*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->streamProofESP.buildings["All"] = data; break;
                    case 3: config->streamProofESP.world["All"] = data; break;
                    default: break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Buildings")) {
                    const auto& data = *(Buildings*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->streamProofESP.buildings["All"] = data; break;
                    case 3: config->streamProofESP.world["All"] = data; break;
                    default: break;
                    }
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("World")) {
                    const auto& data = *(World*)payload->Data;

                    switch (i) {
                    case 0: case 1: getConfigPlayer(i, "All") = data; break;
                    case 2: config->streamProofESP.buildings["All"] = data; break;
                    case 3: config->streamProofESP.world["All"] = data; break;
                    default: break;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::PushID(i);
            ImGui::Indent();

            const auto items = [](std::size_t category) noexcept -> std::vector<const char*> {
                switch (category) {
                case 0:
                case 1: return { };
                case 2: return { "Allies", "Enemies" };
                case 3: return { };
                default: return { };
                }
            }(i);

            const auto categoryEnabled = getConfigShared(i, "All").enabled;

            for (std::size_t j = 0; j < items.size(); ++j) {
                static bool selectedSubItem;
                if (!categoryEnabled || getConfigShared(i, items[j]).enabled) {
                    if (ImGui::Selectable(items[j], currentCategory == i && !selectedSubItem && std::string_view{ currentItem } == items[j])) {
                        currentCategory = i;
                        currentItem = items[j];
                        selectedSubItem = false;
                    }

                    if (ImGui::BeginDragDropSource()) {
                        switch (i) {
                        case 0: case 1: ImGui::SetDragDropPayload("Player", &getConfigPlayer(i, items[j]), sizeof(Player), ImGuiCond_Once); break;
                        case 2: ImGui::SetDragDropPayload("Buildings", &config->streamProofESP.buildings[items[j]], sizeof(Buildings), ImGuiCond_Once); break;
                        case 3: ImGui::SetDragDropPayload("World", &config->streamProofESP.world[items[j]], sizeof(World), ImGuiCond_Once); break;
                        default: break;
                        }
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Player")) {
                            const auto& data = *(Player*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->streamProofESP.buildings[items[j]] = data; break;
                            case 3: config->streamProofESP.world[items[j]] = data; break;
                            default: break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Buildings")) {
                            const auto& data = *(Buildings*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->streamProofESP.buildings[items[j]] = data; break;
                            case 3: config->streamProofESP.world[items[j]] = data; break;
                            default: break;
                            }
                        }

                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("World")) {
                            const auto& data = *(World*)payload->Data;

                            switch (i) {
                            case 0: case 1: getConfigPlayer(i, items[j]) = data; break;
                            case 2: config->streamProofESP.buildings[items[j]] = data; break;
                            case 3: config->streamProofESP.world[items[j]] = data; break;
                            default: break;
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }
            ImGui::Unindent();
            ImGui::PopID();
        }
        ImGui::EndListBox();
    }

    ImGui::SameLine();
    if (ImGui::BeginChild("##child", { 400.0f, 0.0f })) {
        auto& sharedConfig = getConfigShared(currentCategory, currentItem);

        ImGui::Checkbox("Enabled", &sharedConfig.enabled);
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 260.0f);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::BeginCombo("Font", config->getSystemFonts()[sharedConfig.font.index].c_str())) {
            for (size_t i = 0; i < config->getSystemFonts().size(); i++) {
                bool isSelected = config->getSystemFonts()[i] == sharedConfig.font.name;
                if (ImGui::Selectable(config->getSystemFonts()[i].c_str(), isSelected, 0, { 250.0f, 0.0f })) {
                    sharedConfig.font.index = i;
                    sharedConfig.font.name = config->getSystemFonts()[i];
                    config->scheduleFontLoad(sharedConfig.font.name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        constexpr auto spacing = 250.0f;
        ImGuiCustom::colorPicker("Snapline", sharedConfig.snapline);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        ImGui::Combo("##1", &sharedConfig.snapline.type, "Bottom\0Top\0Crosshair\0");
        ImGui::SameLine(spacing);
        ImGuiCustom::colorPicker("Box", sharedConfig.box);
        ImGui::SameLine();

        ImGui::PushID("Box");

        if (ImGui::Button("..."))
            ImGui::OpenPopup("");

        if (ImGui::BeginPopup("")) {
            ImGui::SetNextItemWidth(95.0f);
            ImGui::Combo("Type", &sharedConfig.box.type, "2D\0" "2D corners\0" "3D\0" "3D corners\0");
            ImGui::SetNextItemWidth(275.0f);
            ImGui::SliderFloat3("Scale", sharedConfig.box.scale.data(), 0.0f, 0.50f, "%.2f");
            ImGuiCustom::colorPicker("Fill", sharedConfig.box.fill);
            ImGui::EndPopup();
        }

        ImGui::PopID();

        ImGuiCustom::colorPicker("Name", sharedConfig.name);
        ImGui::SameLine(spacing);

        if (currentCategory < 2) {
            auto& playerConfig = getConfigPlayer(currentCategory, currentItem);

            ImGuiCustom::colorPicker("Weapon", playerConfig.weapon);
            ImGuiCustom::colorPicker("Skeleton", playerConfig.skeleton);
        
            ImGui::SameLine(spacing);
            ImGui::Checkbox("Health Bar", &playerConfig.healthBar.enabled);
            ImGui::SameLine();

            ImGui::PushID("Health Bar");

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::SetNextItemWidth(95.0f);
                ImGui::Combo("Type", &playerConfig.healthBar.type, "Gradient\0Solid\0Health-based\0");
                if (playerConfig.healthBar.type == HealthBar::Solid) {
                    ImGui::SameLine();
                    ImGuiCustom::colorPicker("", static_cast<Color4&>(playerConfig.healthBar));
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();

            ImGui::Checkbox("Disable on cloaked", &playerConfig.disableOnCloaked);
        } else if (currentCategory == 2) {
            auto& buildingConfig = config->streamProofESP.buildings[currentItem];

            ImGuiCustom::colorPicker("Owner", buildingConfig.owner);

            ImGui::Checkbox("Health Bar", &buildingConfig.healthBar.enabled);
            ImGui::SameLine();

            ImGui::PushID("Health Bar");

            if (ImGui::Button("..."))
                ImGui::OpenPopup("");

            if (ImGui::BeginPopup("")) {
                ImGui::SetNextItemWidth(95.0f);
                ImGui::Combo("Type", &buildingConfig.healthBar.type, "Gradient\0Solid\0Health-based\0");
                if (buildingConfig.healthBar.type == HealthBar::Solid) {
                    ImGui::SameLine();
                    ImGuiCustom::colorPicker("", static_cast<Color4&>(buildingConfig.healthBar));
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        ImGui::SetNextItemWidth(115.0f);
        ImGui::InputFloat("Text Cull Distance", &sharedConfig.textCullDistance, 0.4f, 0.8f, "%.1fm");
        sharedConfig.textCullDistance = std::clamp(sharedConfig.textCullDistance, 0.0f, 999.9f);
    }

    ImGui::EndChild();
}

void GUI::renderVisualsWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 280.0f);
    ImGui::Checkbox("Disable post-processing", &config->visuals.disablePostProcessing);
    ImGui::Checkbox("Inverse ragdoll gravity", &config->visuals.inverseRagdollGravity);
    ImGui::Checkbox("No fog", &config->visuals.noFog);

    ImGui::Checkbox("No scope overlay", &config->visuals.noScopeOverlay);

    ImGui::NextColumn();
    ImGui::Checkbox("Thirdperson", &config->visuals.thirdperson);
    ImGui::SameLine();
    ImGui::PushID("Thirdperson Key");
    ImGui::hotkey2("", config->visuals.thirdpersonKey);
    ImGui::PopID();
    ImGui::Checkbox("Freecam", &config->visuals.freeCam);
    ImGui::SameLine();
    ImGui::PushID("Freecam Key");
    ImGui::hotkey2("", config->visuals.freeCamKey);
    ImGui::PopID();
    ImGui::PushItemWidth(290.0f);
    ImGui::PushID(1);
    ImGui::SliderInt("", &config->visuals.freeCamSpeed, 1, 10, "Freecam speed: %d");
    ImGui::PopID();
    ImGui::PushID(2);
    ImGui::SliderInt("", &config->visuals.fov, -60, 60, "FOV: %d");
    ImGui::PopID();

    ImGuiCustom::colorPicker("Bullet Tracers", config->visuals.bulletTracers.color.data(), &config->visuals.bulletTracers.color[3], nullptr, nullptr, &config->visuals.bulletTracers.enabled);
    ImGuiCustom::colorPicker("Bullet Impacts", config->visuals.bulletImpacts.color.data(), &config->visuals.bulletImpacts.color[3], nullptr, nullptr, &config->visuals.bulletImpacts.enabled);
    ImGui::SliderFloat("Bullet Impacts time", &config->visuals.bulletImpactsTime, 0.1f, 5.0f, "Bullet Impacts time: %.2fs");
    ImGuiCustom::colorPicker("On Hit Hitbox", config->visuals.onHitHitbox.color.color.data(), &config->visuals.onHitHitbox.color.color[3], nullptr, nullptr, &config->visuals.onHitHitbox.color.enabled);
    ImGui::SliderFloat("On Hit Hitbox Time", &config->visuals.onHitHitbox.duration, 0.1f, 60.0f, "On Hit Hitbox time: % .2fs");

    ImGui::Checkbox("Viewmodel", &config->visuals.viewModel.enabled);
    ImGui::SameLine();

    if (bool ccPopup = ImGui::Button("Edit"))
        ImGui::OpenPopup("##viewmodel");

    if (ImGui::BeginPopup("##viewmodel"))
    {
        ImGui::PushItemWidth(290.0f);
        ImGui::SliderFloat("##x", &config->visuals.viewModel.x, -20.0f, 20.0f, "X: %.4f");
        ImGui::SliderFloat("##y", &config->visuals.viewModel.y, -20.0f, 20.0f, "Y: %.4f");
        ImGui::SliderFloat("##z", &config->visuals.viewModel.z, -20.0f, 20.0f, "Z: %.4f");
        ImGui::SliderFloat("##roll", &config->visuals.viewModel.roll, -90.0f, 90.0f, "Viewmodel roll: %.2f");
        ImGui::PopItemWidth();
        ImGui::EndPopup();
    }

    ImGui::Columns(1);
}

void GUI::renderMiscWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 230.0f);
    hotkey3("Menu Key", config->misc.menuKey);
    ImGui::Checkbox("Anti AFK kick", &config->misc.antiAfkKick);
    ImGui::Checkbox("Auto strafe", &config->misc.autoStrafe);
    ImGui::Checkbox("Bunny hop", &config->misc.bunnyHop);

    ImGui::Checkbox("Edge Jump", &config->misc.edgeJump);
    ImGui::SameLine();
    ImGui::PushID("Edge Jump Key");
    ImGui::hotkey2("", config->misc.edgeJumpKey);
    ImGui::PopID();

    ImGui::Checkbox("Auto accept", &config->misc.autoAccept);
    ImGui::Checkbox("Reveal votes", &config->misc.revealVotes);

    ImGui::Checkbox("Spectator list", &config->misc.spectatorList.enabled);
    ImGui::SameLine();

    ImGui::PushID("Spectator list");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("No Title Bar", &config->misc.spectatorList.noTitleBar);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Keybinds list", &config->misc.keybindList.enabled);
    ImGui::SameLine();

    ImGui::PushID("Keybinds list");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("No Title Bar", &config->misc.keybindList.noTitleBar);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::PushID("Player List");
    ImGui::Checkbox("Player List", &config->misc.playerList.enabled);
    ImGui::SameLine();

    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("Steam ID", &config->misc.playerList.steamID);
        ImGui::Checkbox("Health", &config->misc.playerList.health);
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Watermark", &config->misc.watermark.enabled);
    ImGuiCustom::colorPicker("Offscreen Enemies", config->misc.offscreenEnemies, &config->misc.offscreenEnemies.enabled);
    ImGui::SameLine();
    ImGui::PushID("Offscreen Enemies");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {
        ImGui::Checkbox("Health Bar", &config->misc.offscreenEnemies.healthBar.enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        ImGui::Combo("Type", &config->misc.offscreenEnemies.healthBar.type, "Gradient\0Solid\0Health-based\0");
        if (config->misc.offscreenEnemies.healthBar.type == HealthBar::Solid) {
            ImGui::SameLine();
            ImGuiCustom::colorPicker("", static_cast<Color4&>(config->misc.offscreenEnemies.healthBar));
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();

    ImGui::Checkbox("Sv pure bypass", &config->misc.svPureBypass);
    ImGui::Checkbox("Unlock hidden cvars", &config->misc.unhideConvars);

    ImGuiCustom::colorPicker("Logger", config->misc.logger);
    ImGui::SameLine();

    ImGui::PushID("Logger");
    if (ImGui::Button("..."))
        ImGui::OpenPopup("");

    if (ImGui::BeginPopup("")) {

        static bool modes[2]{ false, false };
        static const char* mode[]{ "Console", "Event log" };
        static std::string previewvaluemode = "";
        for (size_t i = 0; i < ARRAYSIZE(modes); i++)
        {
            modes[i] = (config->misc.loggerOptions.modes & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo("Log output", previewvaluemode.c_str()))
        {
            previewvaluemode = "";
            for (size_t i = 0; i < ARRAYSIZE(modes); i++)
            {
                ImGui::Selectable(mode[i], &modes[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(modes); i++)
        {
            if (i == 0)
                previewvaluemode = "";

            if (modes[i])
            {
                previewvaluemode += previewvaluemode.size() ? std::string(", ") + mode[i] : mode[i];
                config->misc.loggerOptions.modes |= 1 << i;
            }
            else
            {
                config->misc.loggerOptions.modes &= ~(1 << i);
            }
        }

        static bool events[4]{ false, false, false, false };
        static const char* event[]{ "Damage dealt", "Damage received", "Hostage taken", "Bomb plants" };
        static std::string previewvalueevent = "";
        for (size_t i = 0; i < ARRAYSIZE(events); i++)
        {
            events[i] = (config->misc.loggerOptions.events & 1 << i) == 1 << i;
        }
        if (ImGui::BeginCombo("Log events", previewvalueevent.c_str()))
        {
            previewvalueevent = "";
            for (size_t i = 0; i < ARRAYSIZE(events); i++)
            {
                ImGui::Selectable(event[i], &events[i], ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups);
            }
            ImGui::EndCombo();
        }
        for (size_t i = 0; i < ARRAYSIZE(events); i++)
        {
            if (i == 0)
                previewvalueevent = "";

            if (events[i])
            {
                previewvalueevent += previewvalueevent.size() ? std::string(", ") + event[i] : event[i];
                config->misc.loggerOptions.events |= 1 << i;
            }
            else
            {
                config->misc.loggerOptions.events &= ~(1 << i);
            }
        }

        ImGui::EndPopup();
    }
    ImGui::PopID();

    if (ImGui::Button("Unhook"))
        hooks->uninstall();

    ImGui::Columns(1);
}

void GUI::renderConfigWindow() noexcept
{
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnOffset(1, 170.0f);

    static bool incrementalLoad = false;
    ImGui::Checkbox("Incremental Load", &incrementalLoad);

    ImGui::PushItemWidth(160.0f);

    auto& configItems = config->getConfigs();
    static int currentConfig = -1;

    static std::string buffer;

    timeToNextConfigRefresh -= ImGui::GetIO().DeltaTime;
    if (timeToNextConfigRefresh <= 0.0f) {
        config->listConfigs();
        if (const auto it = std::find(configItems.begin(), configItems.end(), buffer); it != configItems.end())
            currentConfig = std::distance(configItems.begin(), it);
        timeToNextConfigRefresh = 0.1f;
    }

    if (static_cast<std::size_t>(currentConfig) >= configItems.size())
        currentConfig = -1;

    if (ImGui::ListBox("", &currentConfig, [](void* data, int idx, const char** out_text) {
        auto& vector = *static_cast<std::vector<std::string>*>(data);
        *out_text = vector[idx].c_str();
        return true;
        }, &configItems, configItems.size(), 5) && currentConfig != -1)
            buffer = configItems[currentConfig];

        ImGui::PushID(0);
        if (ImGui::InputTextWithHint("", "config name", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (currentConfig != -1)
                config->rename(currentConfig, buffer.c_str());
        }
        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::PushItemWidth(100.0f);

        if (ImGui::Button("Open config directory"))
            config->openConfigDir();

        if (ImGui::Button("Create config", { 100.0f, 25.0f }))
            config->add(buffer.c_str());

        if (ImGui::Button("Reset config", { 100.0f, 25.0f }))
            ImGui::OpenPopup("Config to reset");

        if (ImGui::BeginPopup("Config to reset")) {
            static constexpr const char* names[]{ "Whole", "Aimbot", "Triggerbot", "Backtrack", "Anti Aim", "Fakelag", "Glow", "Chams", "ESP", "Visuals", "Misc" };
            for (int i = 0; i < IM_ARRAYSIZE(names); i++) {
                if (i == 1) ImGui::Separator();

                if (ImGui::Selectable(names[i])) {
                    switch (i) {
                    case 0: config->reset(); break;
                    case 1: config->aimbot = { }; config->aimbotKey.reset(); break;
                    case 2: config->triggerbot = { }; config->triggerbotKey.reset();  break;
                    case 3: config->backtrack = { };  break;
                    case 4: config->antiAim = { }; break;
                    case 6: config->fakelag = { }; break;
                    case 7: config->glow = { }; config->glowKey.reset(); break;
                    case 8: config->chams = { }; config->chamsKey.reset(); break;
                    case 9: config->streamProofESP = { }; break;
                    case 10: config->visuals = { }; break;
                    case 11: config->misc = { }; break;
                    default: break;
                    }
                }
            }
            ImGui::EndPopup();
        }
        if (currentConfig != -1) {
            if (ImGui::Button("Load selected", { 100.0f, 25.0f })) {
                config->load(currentConfig, incrementalLoad);
            }
            if (ImGui::Button("Save selected", { 100.0f, 25.0f }))
                ImGui::OpenPopup("##reallySave");
            if (ImGui::BeginPopup("##reallySave"))
            {
                ImGui::TextUnformatted("Are you sure?");
                if (ImGui::Button("No", { 45.0f, 0.0f }))
                    ImGui::CloseCurrentPopup();
                ImGui::SameLine();
                if (ImGui::Button("Yes", { 45.0f, 0.0f }))
                {
                    config->save(currentConfig);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::Button("Delete selected", { 100.0f, 25.0f }))
                ImGui::OpenPopup("##reallyDelete");
            if (ImGui::BeginPopup("##reallyDelete"))
            {
                ImGui::TextUnformatted("Are you sure?");
                if (ImGui::Button("No", { 45.0f, 0.0f }))
                    ImGui::CloseCurrentPopup();
                ImGui::SameLine();
                if (ImGui::Button("Yes", { 45.0f, 0.0f }))
                {
                    config->remove(currentConfig);
                    if (static_cast<std::size_t>(currentConfig) < configItems.size())
                        buffer = configItems[currentConfig];
                    else
                        buffer.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::Columns(1);
}

void Active() { ImGuiStyle* Style = &ImGui::GetStyle(); Style->Colors[ImGuiCol_Button] = ImColor(25, 30, 34); Style->Colors[ImGuiCol_ButtonActive] = ImColor(25, 30, 34); Style->Colors[ImGuiCol_ButtonHovered] = ImColor(25, 30, 34); }
void Hovered() { ImGuiStyle* Style = &ImGui::GetStyle(); Style->Colors[ImGuiCol_Button] = ImColor(19, 22, 27); Style->Colors[ImGuiCol_ButtonActive] = ImColor(19, 22, 27); Style->Colors[ImGuiCol_ButtonHovered] = ImColor(19, 22, 27); }



void GUI::renderGuiStyle() noexcept
{
    ImGuiStyle* Style = &ImGui::GetStyle();
    Style->WindowRounding = 5.5;
    Style->WindowBorderSize = 2.5;
    Style->ChildRounding = 5.5;
    Style->FrameBorderSize = 2.5;
    Style->Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 0);
    Style->Colors[ImGuiCol_ChildBg] = ImColor(31, 31 ,31);
    Style->Colors[ImGuiCol_Button] = ImColor(25, 30, 34);
    Style->Colors[ImGuiCol_ButtonHovered] = ImColor(25, 30, 34);
    Style->Colors[ImGuiCol_ButtonActive] = ImColor(19, 22, 27);

    Style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(25, 30, 34);
    Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(25, 30, 34);
    Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(25, 30, 34);

    static auto Name = "Menu";
    static auto Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    static int activeTab = 1;
    static int activeSubTabAimbot = 1;
    static int activeSubTabVisuals = 1;
    static int activeSubTabMisc = 1;
    static int activeSubTabConfigs = 1;

    if (ImGui::Begin(Name, NULL, Flags))
    {
        Style->Colors[ImGuiCol_ChildBg] = ImColor(25, 30, 34);

        ImGui::BeginChild("##Back", ImVec2{ 704, 434 }, false);
        {
            ImGui::SetCursorPos(ImVec2{ 2, 2 });

            Style->Colors[ImGuiCol_ChildBg] = ImColor(19, 22, 27);

            ImGui::BeginChild("##Main", ImVec2{ 700, 430 }, false);
            {
                ImGui::BeginChild("##UP", ImVec2{ 700, 45 }, false);
                {
                    ImGui::SetCursorPos(ImVec2{ 10, 6 });
                    ImGui::PushFont(fonts.tahoma34); ImGui::Text("Seth"); ImGui::PopFont();

                    float pos = 305;
                    ImGui::SetCursorPos(ImVec2{ pos, 0 });
                    if (activeTab == 1) Active(); else Hovered();
                    if (ImGui::Button("Aimbot", ImVec2{ 75, 45 }))
                        activeTab = 1;

                    pos += 80;

                    ImGui::SetCursorPos(ImVec2{ pos, 0 });
                    if (activeTab == 2) Active(); else Hovered();
                    if (ImGui::Button("Visuals", ImVec2{ 75, 45 }))
                        activeTab = 2;

                    pos += 80;

                    ImGui::SetCursorPos(ImVec2{ pos, 0 });
                    if (activeTab == 3) Active(); else Hovered();
                    if (ImGui::Button("Misc", ImVec2{ 75, 45 }))
                        activeTab = 3;

                    pos += 80;

                    ImGui::SetCursorPos(ImVec2{ pos, 0 });
                    if (activeTab == 4) Active(); else Hovered();
                    if (ImGui::Button("Configs", ImVec2{ 75, 45 }))
                        activeTab = 4;
                }
                ImGui::EndChild();

                ImGui::SetCursorPos(ImVec2{ 0, 45 });
                Style->Colors[ImGuiCol_ChildBg] = ImColor(25, 30, 34);
                Style->Colors[ImGuiCol_Button] = ImColor(25, 30, 34);
                Style->Colors[ImGuiCol_ButtonHovered] = ImColor(25, 30, 34);
                Style->Colors[ImGuiCol_ButtonActive] = ImColor(19, 22, 27);
                ImGui::BeginChild("##Childs", ImVec2{ 700, 365 }, false);
                {
                    ImGui::SetCursorPos(ImVec2{ 15, 5 });
                    Style->ChildRounding = 0;
                    ImGui::BeginChild("##Left", ImVec2{ 155, 320 }, false);
                    {
                        switch (activeTab)
                        {
                        case 1: //Aimbot
                            ImGui::SetCursorPosY(10);
                            if (ImGui::Button("Main                    ", ImVec2{ 80, 20 })) activeSubTabAimbot = 1;
                            if (ImGui::Button("Triggerbot              ", ImVec2{ 80, 20 })) activeSubTabAimbot = 2;
                            if (ImGui::Button("Backtrack               ", ImVec2{ 80, 20 })) activeSubTabAimbot = 3;
                            if (ImGui::Button("AntiAim                 ", ImVec2{ 80, 20 })) activeSubTabAimbot = 4;
                            if (ImGui::Button("FakeLag                 ", ImVec2{ 80, 20 })) activeSubTabAimbot = 5;
                            break;
                        case 2: //Visuals
                            ImGui::SetCursorPosY(10);
                            if (ImGui::Button("Main                    ", ImVec2{ 80, 20 })) activeSubTabVisuals = 1;
                            if (ImGui::Button("Esp                     ", ImVec2{ 80, 20 })) activeSubTabVisuals = 2;
                            if (ImGui::Button("Chams                   ", ImVec2{ 80, 20 })) activeSubTabVisuals = 3;
                            if (ImGui::Button("Glow                    ", ImVec2{ 80, 20 })) activeSubTabVisuals = 4;
                            break;
                        case 3: //Misc
                            ImGui::SetCursorPosY(10);
                            if (ImGui::Button("Main                    ", ImVec2{ 80, 20 })) activeSubTabMisc = 1;
                            break;
                        default:
                            break;
                        }

                        ImGui::EndChild();

                        ImGui::SetCursorPos(ImVec2{ 100, 5 });
                        Style->Colors[ImGuiCol_ChildBg] = ImColor(29, 34, 38);
                        Style->ChildRounding = 5;
                        ImGui::BeginChild("##SubMain", ImVec2{ 590, 350 }, false);
                        {
                            ImGui::SetCursorPos(ImVec2{ 10, 10 });
                            switch (activeTab)
                            {
                            case 1: //Aimbot
                                switch (activeSubTabAimbot)
                                {
                                case 1:
                                    //Main
                                    renderAimbotWindow();
                                    break;
                                case 2:
                                    //Triggerbot
                                    renderTriggerbotWindow();
                                    break;
                                case 3:
                                    //Backtrack
                                    renderBacktrackWindow();
                                    break;
                                case 4:
                                    //Anti aim
                                    renderAntiAimWindow();
                                    break;
                                case 5:
                                    //FakeLag
                                    renderFakelagWindow();
                                    break;
                                default:
                                    break;
                                }
                                break;
                            case 2: // Visuals
                                switch (activeSubTabVisuals)
                                {
                                case 1:
                                    //Main
                                    renderVisualsWindow();
                                    break;
                                case 2:
                                    //Esp
                                    renderStreamProofESPWindow();
                                    break;
                                case 3:
                                    //Chams
                                    renderChamsWindow();
                                    break;
                                case 4:
                                    //Glow
                                    renderGlowWindow();
                                    break;
                                default:
                                    break;
                                }
                                break;
                            case 3:
                                switch (activeSubTabMisc)
                                {
                                case 1:
                                    //Main
                                    renderMiscWindow();
                                    break;
                                default:
                                    break;
                                }
                                break;
                            case 4:
                                //Configs
                                renderConfigWindow();
                                break;
                            default:
                                break;
                            }
                        }
                        ImGui::EndChild();
                    }
                    ImGui::EndChild();

                    ImGui::SetCursorPos(ImVec2{ 0, 410 });
                    Style->Colors[ImGuiCol_ChildBg] = ImColor(45, 50, 54);
                    Style->ChildRounding = 0;
                    ImGui::BeginChild("##Text", ImVec2{ 700, 20 }, false);
                    {
                        ImGui::SetCursorPos(ImVec2{ 2, 2 });
                        ImGui::Text("https://github.com/notgoodusename/Seth");
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    Style->Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.75f);
}
