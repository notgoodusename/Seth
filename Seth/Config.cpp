#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include <ShlObj.h>

#include "nlohmann/json.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Config.h"
#include "Helpers.h"

#include "SDK/Platform.h"

int CALLBACK fontCallback(const LOGFONTW* lpelfe, const TEXTMETRICW*, DWORD, LPARAM lParam)
{
    const wchar_t* const fontName = reinterpret_cast<const ENUMLOGFONTEXW*>(lpelfe)->elfFullName;

    if (fontName[0] == L'@')
        return TRUE;

    if (HFONT font = CreateFontW(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName)) {

        DWORD fontData = GDI_ERROR;

        if (HDC hdc = CreateCompatibleDC(nullptr)) {
            SelectObject(hdc, font);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(font);

        if (fontData == GDI_ERROR) {
            if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
                reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(buff);
        }
    }
    return TRUE;
}

Config::Config() noexcept
{
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        CoTaskMemFree(pathToDocuments);
    }

    path /= "Seth";
    listConfigs();

    load(u8"default.json", false);

    LOGFONTW logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = L'\0';

    EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);

    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}
#pragma region  Read

static void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color4&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, Color3& c)
{
    read(j, "Color", c.color);
    read(j, "Rainbow", c.rainbow);
    read(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, ColorToggle3& ct)
{
    from_json(j, static_cast<Color3&>(ct));
    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, ColorToggleRounding& ctr)
{
    from_json(j, static_cast<ColorToggle&>(ctr));

    read(j, "Rounding", ctr.rounding);
}

static void from_json(const json& j, ColorToggleOutline& cto)
{
    from_json(j, static_cast<ColorToggle&>(cto));

    read(j, "Outline", cto.outline);
}

static void from_json(const json& j, ColorToggleThickness& ctt)
{
    from_json(j, static_cast<ColorToggle&>(ctt));

    read(j, "Thickness", ctt.thickness);
}

static void from_json(const json& j, ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<ColorToggleRounding&>(cttr));

    read(j, "Thickness", cttr.thickness);
}

static void from_json(const json& j, Font& f)
{
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty())
        config->scheduleFontLoad(f.name);

    if (const auto it = std::find_if(config->getSystemFonts().begin(), config->getSystemFonts().end(), [&f](const auto& e) { return e == f.name; }); it != config->getSystemFonts().end())
        f.index = std::distance(config->getSystemFonts().begin(), it);
    else
        f.index = 0;
}

static void from_json(const json& j, Snapline& s)
{
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read(j, "Type", s.type);
}

static void from_json(const json& j, Box& b)
{
    from_json(j, static_cast<ColorToggleRounding&>(b));

    read(j, "Type", b.type);
    read(j, "Scale", b.scale);
    read<value_t::object>(j, "Fill", b.fill);
}

static void from_json(const json& j, Shared& s)
{
    read(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snapline", s.snapline);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::object>(j, "Name", s.name);
    read(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json& j, Buildings& w)
{
    from_json(j, static_cast<Shared&>(w));
}

static void from_json(const json& j, World& p)
{
    from_json(j, static_cast<Shared&>(p));
}

static void from_json(const json& j, HealthBar& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
    read(j, "Type", o.type);
}

static void from_json(const json& j, Player& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Health Bar", p.healthBar);
    read<value_t::object>(j, "Skeleton", p.skeleton);
}

static void from_json(const json& j, OffscreenEnemies& o)
{
    from_json(j, static_cast<ColorToggle&>(o));

    read<value_t::object>(j, "Health Bar", o.healthBar);
}

static void from_json(const json& j, BulletTracers& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
}

static void from_json(const json& j, ImVec2& v)
{
    read(j, "X", v.x);
    read(j, "Y", v.y);
}

static void from_json(const json& j, Config::Aimbot& a)
{
    read<value_t::object>(j, "Hitscan", a.hitscan);
    read<value_t::object>(j, "Projectile", a.projectile);
    read<value_t::object>(j, "Melee", a.melee);
}

static void from_json(const json& j, Config::Aimbot::Hitscan& h)
{
    read(j, "Enabled", h.enabled);
    read(j, "Aimlock", h.aimlock);
    read(j, "Silent", h.silent);
    read(j, "Friendly fire", h.friendlyFire);
    read(j, "Target backtrack", h.targetBacktrack);
    read(j, "Ignore cloaked", h.ignoreCloaked);
    read(j, "Scoped only", h.scopedOnly);
    read(j, "Auto shoot", h.autoShoot);
    read(j, "Auto scope", h.autoScope);
    read(j, "Auto rev", h.autoRev);
    read(j, "Auto extinguish team", h.autoExtinguishTeam);
    read(j, "Wait for headshot", h.waitForHeadshot);
    read(j, "Wait for charge", h.waitForHeadshot);
    read(j, "Sort method", h.sortMethod);
    read(j, "Hitboxes", h.hitboxes);
    read(j, "Fov", h.fov);
    read(j, "Smooth", h.smooth);
}

static void from_json(const json& j, Config::Aimbot::Projectile& p)
{
    read(j, "Enabled", p.enabled);
}

static void from_json(const json& j, Config::Aimbot::Melee& m)
{
    read(j, "Enabled", m.enabled);
    read(j, "Aimlock", m.aimlock);
    read(j, "Silent", m.silent);
    read(j, "Auto backstab", m.autoBackstab);
    read(j, "Sort method", m.sortMethod);
    read(j, "Fov", m.fov);
    read(j, "Smooth", m.smooth);
}

static void from_json(const json& j, Config::Triggerbot& t)
{
    read(j, "Enabled", t.enabled);
    read(j, "Friendly fire", t.friendlyFire);
    read(j, "Scoped only", t.scopedOnly);
    read(j, "Hitboxes", t.hitboxes);
    read(j, "Shot delay", t.shotDelay);
}

static void from_json(const json& j, Config::AntiAim& a)
{
    read(j, "Enabled", a.enabled);
    read(j, "Pitch", a.pitch);
    read(j, "Yaw base", reinterpret_cast<int&>(a.yawBase));
    read(j, "Manual forward Key", a.manualForward);
    read(j, "Manual backward Key", a.manualBackward);
    read(j, "Manual right Key", a.manualRight);
    read(j, "Manual left Key", a.manualLeft);
    read(j, "Yaw modifier", a.yawModifier);
    read(j, "Yaw add", a.yawAdd);
    read(j, "Jitter Range", a.jitterRange);
    read(j, "Spin base", a.spinBase);
    read(j, "At targets", a.atTargets);
}

static void from_json(const json& j, Config::Fakelag& f)
{
    read(j, "Enabled", f.enabled);
    read(j, "Mode", f.mode);
    read(j, "Limit", f.limit);
}

static void from_json(const json& j, Config::Tickbase& t)
{
    read(j, "Doubletap", t.doubletap);
    read(j, "Hideshots", t.hideshots);
    read(j, "Teleport", t.teleport);
}

static void from_json(const json& j, Config::Backtrack& b)
{
    read(j, "Enabled", b.enabled);
    read(j, "Time limit", b.timeLimit);
    read(j, "Fake Latency", b.fakeLatency);
    read(j, "Fake Latency Amount", b.fakeLatencyAmount);
}

static void from_json(const json& j, Config::Chams::Material& m)
{
    from_json(j, static_cast<Color4&>(m));

    read(j, "Enabled", m.enabled);
    read(j, "Health based", m.healthBased);
    read(j, "Blinking", m.blinking);
    read(j, "Wireframe", m.wireframe);
    read(j, "Cover", m.cover);
    read(j, "Ignore-Z", m.ignorez);
    read(j, "Material", m.material);
}

static void from_json(const json& j, Config::Chams& c)
{
    read_array_opt(j, "Materials", c.materials);
}

static void from_json(const json& j, Config::GlowItem& g)
{
    from_json(j, static_cast<Color4&>(g));

    read(j, "Enabled", g.enabled);
    read(j, "Health based", g.healthBased);
    read(j, "Style", g.style);
}

static void from_json(const json& j, Config::PlayerGlow& g)
{
    read<value_t::object>(j, "All", g.all);
    read<value_t::object>(j, "Visible", g.visible);
    read<value_t::object>(j, "Occluded", g.occluded);
}

static void from_json(const json& j, Config::StreamProofESP& e)
{
    read(j, "Key", e.key);
    read(j, "Allies", e.allies);
    read(j, "Enemies", e.enemies);
    read(j, "Buildings", e.buildings);
    read(j, "World", e.world);
}

static void from_json(const json& j, Config::Visuals& v)
{
    read(j, "Disable post-processing", v.disablePostProcessing);
    read(j, "Inverse ragdoll gravity", v.inverseRagdollGravity);
    read(j, "No fog", v.noFog);
    read(j, "No scope overlay", v.noScopeOverlay);
    read(j, "Thirdperson", v.thirdperson);
    read(j, "Thirdperson key", v.thirdpersonKey);
    read(j, "Freecam", v.freeCam);
    read(j, "Freecam key", v.freeCamKey);
    read(j, "Freecam speed", v.freeCamSpeed);
    read(j, "FOV", v.fov);
    read<value_t::object>(j, "Bullet Tracers", v.bulletTracers);
    read<value_t::object>(j, "Bullet Impacts", v.bulletImpacts);
    read<value_t::object>(j, "Hitbox on Hit", v.onHitHitbox);
    read(j, "Bullet Impacts time", v.bulletImpactsTime);
    read<value_t::object>(j, "Viewmodel", v.viewModel);
}

static void from_json(const json& j, PurchaseList& pl)
{
    read(j, "Enabled", pl.enabled);
    read(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read(j, "Show Prices", pl.showPrices);
    read(j, "No Title Bar", pl.noTitleBar);
    read(j, "Mode", pl.mode);
    read<value_t::object>(j, "Pos", pl.pos);
}

static void from_json(const json& j, Config::Misc::SpectatorList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::KeyBindList& sl)
{
    read(j, "Enabled", sl.enabled);
    read(j, "No Title Bar", sl.noTitleBar);
    read<value_t::object>(j, "Pos", sl.pos);
}

static void from_json(const json& j, Config::Misc::PlayerList& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Steam ID", o.steamID);
    read(j, "Health", o.health);
    read<value_t::object>(j, "Pos", o.pos);
}

static void from_json(const json& j, Config::Misc::Watermark& o)
{
    read(j, "Enabled", o.enabled);
    read<value_t::object>(j, "Pos", o.pos);
}

static void from_json(const json& j, PreserveKillfeed& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Only Headshots", o.onlyHeadshots);
}

static void from_json(const json& j, KillfeedChanger& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Headshot", o.headshot);
    read(j, "Dominated", o.dominated);
    read(j, "Revenge", o.revenge);
    read(j, "Penetrated", o.penetrated);
    read(j, "Noscope", o.noscope);
    read(j, "Thrusmoke", o.thrusmoke);
    read(j, "Attackerblind", o.attackerblind);
}

static void from_json(const json& j, AutoBuy& o)
{
    read(j, "Enabled", o.enabled);
    read(j, "Primary weapon", o.primaryWeapon);
    read(j, "Secondary weapon", o.secondaryWeapon);
    read(j, "Armor", o.armor);
    read(j, "Utility", o.utility);
    read(j, "Grenades", o.grenades);
}

static void from_json(const json& j, Config::Misc::Logger& o)
{
    read(j, "Modes", o.modes);
    read(j, "Events", o.events);
}

static void from_json(const json& j, Config::Visuals::Viewmodel& vxyz)
{
    read(j, "Enabled", vxyz.enabled);
    read(j, "X", vxyz.x);
    read(j, "Y", vxyz.y);
    read(j, "Z", vxyz.z);
    read(j, "Roll", vxyz.roll);
}

static void from_json(const json& j, Config::Visuals::OnHitHitbox& h)
{
    read<value_t::object>(j, "Color", h.color);
    read(j, "Duration", h.duration);
}

static void from_json(const json& j, Config::Misc& m)
{
    read(j, "Menu key", m.menuKey);
    read(j, "Anti AFK kick", m.antiAfkKick);
    read(j, "Auto strafe", m.autoStrafe);
    read(j, "Bunny hop", m.bunnyHop);
    read(j, "Edge Jump", m.edgeJump);
    read(j, "Edge Jump Key", m.edgeJumpKey);
    read(j, "Auto accept", m.autoAccept);
    read(j, "Reveal votes", m.revealVotes);
    read<value_t::object>(j, "Spectator list", m.spectatorList);
    read<value_t::object>(j, "Keybind list", m.keybindList);
    read<value_t::object>(j, "Player list", m.playerList);
    read<value_t::object>(j, "Watermark", m.watermark);
    read<value_t::object>(j, "Offscreen Enemies", m.offscreenEnemies);
    read(j, "Sv pure bypass", m.svPureBypass);
    read(j, "Unlock hidden cvars", m.unhideConvars);
    read<value_t::object>(j, "Logger", m.logger);
    read<value_t::object>(j, "Logger options", m.loggerOptions);
}

void Config::load(size_t id, bool incremental) noexcept
{
    load((const char8_t*)configs[id].c_str(), incremental);
}

void Config::load(const char8_t* name, bool incremental) noexcept
{
    json j;

    if (std::ifstream in{ path / name }; in.good()) {
        j = json::parse(in, nullptr, false);
        if (j.is_discarded())
            return;
    } else {
        return;
    }

    if (!incremental)
        reset();

    read<value_t::object>(j, "Aimbot", aimbot);
    read(j, "Aimbot Key", aimbotKey);
    read<value_t::object>(j, "Draw aimbot fov", aimbotFov);

    read<value_t::object>(j, "Triggerbot", triggerbot);
    read(j, "Triggerbot Key", triggerbotKey);

    read<value_t::object>(j, "Anti aim", antiAim);
    read<value_t::object>(j, "Fakelag", fakelag);
    read<value_t::object>(j, "Tickbase", tickbase);
    read<value_t::object>(j, "Backtrack", backtrack);

    read(j["Glow"], "Items", glow);
    read(j["Glow"], "Players", playerGlow);
    read(j["Glow"], "Key", glowKey);

    read(j, "Chams", chams);
    read(j["Chams"], "Key", chamsKey);
    read<value_t::object>(j, "ESP", streamProofESP);
    read<value_t::object>(j, "Visuals", visuals);
    read<value_t::object>(j, "Misc", misc);
}

#pragma endregion

#pragma region  Write

static void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const Color3& o, const Color3& dummy = {})
{
    WRITE("Color", color);
    WRITE("Rainbow", rainbow);
    WRITE("Rainbow Speed", rainbowSpeed);
}

static void to_json(json& j, const ColorToggle3& o, const ColorToggle3& dummy = {})
{
    to_json(j, static_cast<const Color3&>(o), dummy);
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding);
}

static void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const ColorToggleOutline& o, const ColorToggleOutline& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Outline", outline);
}

static void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness);
}

static void to_json(json& j, const Font& o, const Font& dummy = {})
{
    WRITE("Name", name);
}

static void to_json(json& j, const Snapline& o, const Snapline& dummy = {})
{
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type);
}

static void to_json(json& j, const Box& o, const Box& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Type", type);
    WRITE("Scale", scale);
    WRITE("Fill", fill);
}

static void to_json(json& j, const Shared& o, const Shared& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Font", font);
    WRITE("Snapline", snapline);
    WRITE("Box", box);
    WRITE("Name", name);
    WRITE("Text Cull Distance", textCullDistance);
}

static void to_json(json& j, const HealthBar& o, const HealthBar& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Type", type);
}

static void to_json(json& j, const Player& o, const Player& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    WRITE("Weapon", weapon);
    WRITE("Health Bar", healthBar);
    WRITE("Skeleton", skeleton);
}

static void to_json(json& j, const Buildings& o, const Buildings& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
}

static void to_json(json& j, const OffscreenEnemies& o, const OffscreenEnemies& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);

    WRITE("Health Bar", healthBar);
}

static void to_json(json& j, const BulletTracers& o, const BulletTracers& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
}

static void to_json(json& j, const World& o, const World& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
}

static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
{
    WRITE("X", x);
    WRITE("Y", y);
}

static void to_json(json& j, const Config::Aimbot& o)
{
    const Config::Aimbot dummy;

    WRITE("Hitscan", hitscan);
    WRITE("Projectile", projectile);
    WRITE("Melee", melee);
}

static void to_json(json& j, const Config::Aimbot::Hitscan& o, const Config::Aimbot::Hitscan& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Aimlock", aimlock);
    WRITE("Silent", silent);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Target backtrack", targetBacktrack);
    WRITE("Ignore cloaked", ignoreCloaked);
    WRITE("Scoped only", scopedOnly);
    WRITE("Auto shoot", autoScope);
    WRITE("Auto scope", autoScope);
    WRITE("Auto rev", autoRev);
    WRITE("Auto extinguish team", autoExtinguishTeam);
    WRITE("Wait for headshot", waitForHeadshot);
    WRITE("Wait for charge", waitForHeadshot);
    WRITE("Sort method", sortMethod);
    WRITE("Hitboxes", hitboxes);
    WRITE("Fov", fov);
    WRITE("Smooth", smooth);
}

static void to_json(json& j, const Config::Aimbot::Projectile& o, const Config::Aimbot::Projectile& dummy = {})
{
    WRITE("Enabled", enabled);
}

static void to_json(json& j, const Config::Aimbot::Melee& o, const Config::Aimbot::Melee& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Aimlock", aimlock);
    WRITE("Silent", silent);
    WRITE("Auto backstab", autoBackstab);
    WRITE("Sort method", sortMethod);
    WRITE("Fov", fov);
    WRITE("Smooth", smooth);
}

static void to_json(json& j, const Config::Triggerbot& o, const Config::Triggerbot& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Friendly fire", friendlyFire);
    WRITE("Scoped only", scopedOnly);
    WRITE("Hitboxes", hitboxes);
    WRITE("Shot delay", shotDelay);
}

static void to_json(json& j, const Config::Chams::Material& o)
{
    const Config::Chams::Material dummy;

    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Blinking", blinking);
    WRITE("Wireframe", wireframe);
    WRITE("Cover", cover);
    WRITE("Ignore-Z", ignorez);
    WRITE("Material", material);
}

static void to_json(json& j, const Config::Chams& o)
{
    j["Materials"] = o.materials;
}

static void to_json(json& j, const Config::GlowItem& o, const  Config::GlowItem& dummy = {})
{
    to_json(j, static_cast<const Color4&>(o), dummy);
    WRITE("Enabled", enabled);
    WRITE("Health based", healthBased);
    WRITE("Style", style);
}

static void to_json(json& j, const  Config::PlayerGlow& o, const  Config::PlayerGlow& dummy = {})
{
    WRITE("All", all);
    WRITE("Visible", visible);
    WRITE("Occluded", occluded);
}

static void to_json(json& j, const Config::StreamProofESP& o, const Config::StreamProofESP& dummy = {})
{
    WRITE("Key", key);
    j["Allies"] = o.allies;
    j["Enemies"] = o.enemies;
    j["Buildings"] = o.buildings;
    j["World"] = o.world;
}

static void to_json(json& j, const Config::AntiAim& o, const Config::AntiAim& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Pitch", pitch);
    WRITE_ENUM("Yaw base", yawBase);
    to_json(j["Manual forward Key"], o.manualForward, KeyBind::NONE);
    to_json(j["Manual backward Key"], o.manualBackward, KeyBind::NONE);
    to_json(j["Manual right Key"], o.manualRight, KeyBind::NONE);
    to_json(j["Manual left Key"], o.manualLeft, KeyBind::NONE);
    WRITE("Yaw modifier", yawModifier);
    WRITE("Yaw add", yawAdd);
    WRITE("Jitter Range", jitterRange);
    WRITE("Spin base", spinBase);
    WRITE("At targets", atTargets);
}

static void to_json(json& j, const Config::Fakelag& o, const Config::Fakelag& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Mode", mode);
    WRITE("Limit", limit);
}

static void to_json(json& j, const Config::Tickbase& o, const Config::Tickbase& dummy = {})
{
    WRITE("Doubletap", doubletap);
    WRITE("Hideshots", hideshots);
    WRITE("Teleport", teleport);
}

static void to_json(json& j, const Config::Backtrack& o, const Config::Backtrack& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Time limit", timeLimit);
    WRITE("Fake Latency", fakeLatency);
    WRITE("Fake Latency Amount", fakeLatencyAmount);
}

static void to_json(json& j, const Config::Misc::SpectatorList& o, const Config::Misc::SpectatorList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Spectator list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::KeyBindList& o, const Config::Misc::KeyBindList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("No Title Bar", noTitleBar);

    if (const auto window = ImGui::FindWindowByName("Keybind list")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::PlayerList& o, const Config::Misc::PlayerList& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Steam ID", steamID);
    WRITE("Health", health);

    if (const auto window = ImGui::FindWindowByName("Player List")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const Config::Misc::Watermark& o, const Config::Misc::Watermark& dummy = {})
{
    WRITE("Enabled", enabled);

    if (const auto window = ImGui::FindWindowByName("Watermark")) {
        j["Pos"] = window->Pos;
    }
}

static void to_json(json& j, const PreserveKillfeed& o, const PreserveKillfeed& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Only Headshots", onlyHeadshots);
}

static void to_json(json& j, const KillfeedChanger& o, const KillfeedChanger& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Headshot", headshot);
    WRITE("Dominated", dominated);
    WRITE("Revenge", revenge);
    WRITE("Penetrated", penetrated);
    WRITE("Noscope", noscope);
    WRITE("Thrusmoke", thrusmoke);
    WRITE("Attackerblind", attackerblind);
}

static void to_json(json& j, const AutoBuy& o, const AutoBuy& dummy = {})
{
    WRITE("Enabled", enabled);
    WRITE("Primary weapon", primaryWeapon);
    WRITE("Secondary weapon", secondaryWeapon);
    WRITE("Armor", armor);
    WRITE("Utility", utility);
    WRITE("Grenades", grenades);
}

static void to_json(json& j, const Config::Misc::Logger& o, const Config::Misc::Logger& dummy = {})
{
    WRITE("Modes", modes);
    WRITE("Events", events);
}

static void to_json(json& j, const Config::Visuals::Viewmodel& o, const Config::Visuals::Viewmodel& dummy)
{
    WRITE("Enabled", enabled);
    WRITE("X", x);
    WRITE("Y", y);
    WRITE("Z", z);
    WRITE("Roll", roll);
}

static void to_json(json& j, const Config::Visuals::OnHitHitbox& o, const Config::Visuals::OnHitHitbox& dummy)
{
    WRITE("Color", color);
    WRITE("Duration", duration);
}

static void to_json(json& j, const Config::Misc& o)
{
    const Config::Misc dummy;

    WRITE("Menu key", menuKey);
    WRITE("Anti AFK kick", antiAfkKick);
    WRITE("Auto strafe", autoStrafe);
    WRITE("Bunny hop", bunnyHop);

    WRITE("Edge Jump", edgeJump);
    WRITE("Edge Jump Key", edgeJumpKey);
    WRITE("Auto accept", autoAccept);
    WRITE("Reveal votes", revealVotes);
    WRITE("Spectator list", spectatorList);
    WRITE("Keybind list", keybindList);
    WRITE("Player list", playerList);
    WRITE("Watermark", watermark);
    WRITE("Offscreen Enemies", offscreenEnemies);
    WRITE("Fast Stop", fastStop);
    WRITE("Sv pure bypass", svPureBypass);
    WRITE("Unlock hidden cvars", unhideConvars);
    WRITE("Logger", logger);
    WRITE("Logger options", loggerOptions);
}

static void to_json(json& j, const Config::Visuals& o)
{
    const Config::Visuals dummy;

    WRITE("Disable post-processing", disablePostProcessing);
    WRITE("Inverse ragdoll gravity", inverseRagdollGravity);
    WRITE("No fog", noFog);
    WRITE("No scope overlay", noScopeOverlay);
    WRITE("Thirdperson", thirdperson);
    WRITE("Thirdperson key", thirdpersonKey);
    WRITE("Freecam", freeCam);
    WRITE("Freecam key", freeCamKey);
    WRITE("Freecam speed", freeCamSpeed);
    WRITE("FOV", fov);
    WRITE("Bullet Tracers", bulletTracers);
    WRITE("Bullet Impacts", bulletImpacts);
    WRITE("Hitbox on Hit", onHitHitbox);
    WRITE("Bullet Impacts time", bulletImpactsTime);
    WRITE("Viewmodel", viewModel);
}

static void to_json(json& j, const ImVec4& o)
{
    j[0] = o.x;
    j[1] = o.y;
    j[2] = o.z;
    j[3] = o.w;
}


#pragma endregion

void removeEmptyObjects(json& j) noexcept
{
    for (auto it = j.begin(); it != j.end();) {
        auto& val = it.value();
        if (val.is_object() || val.is_array())
            removeEmptyObjects(val);
        if (val.empty() && !j.is_array())
            it = j.erase(it);
        else
            ++it;
    }
}

void Config::save(size_t id) const noexcept
{
    createConfigDir();

    if (std::ofstream out{ path / (const char8_t*)configs[id].c_str() }; out.good()) {
        json j;

        j["Aimbot"] = aimbot;
        to_json(j["Aimbot Key"], aimbotKey, KeyBind::NONE);
        j["Draw aimbot fov"] = aimbotFov;

        j["Triggerbot"] = triggerbot;
        to_json(j["Triggerbot Key"], triggerbotKey, KeyBind::NONE);

        j["Anti aim"] = antiAim;
        j["Fakelag"] = fakelag;
        j["Tickbase"] = tickbase;
        j["Backtrack"] = backtrack;

        j["Glow"]["Items"] = glow;
        j["Glow"]["Players"] = playerGlow;
        to_json(j["Glow"]["Key"], glowKey, KeyBind::NONE);

        j["Chams"] = chams;
        to_json(j["Chams"]["Key"], chamsKey, KeyBind::NONE);
        j["ESP"] = streamProofESP;
        j["Visuals"] = visuals;
        j["Misc"] = misc;

        removeEmptyObjects(j);
        out << std::setw(2) << j;
    }
}

void Config::add(const char* name) noexcept
{
    if (*name && std::find(configs.cbegin(), configs.cend(), name) == configs.cend()) {
        configs.emplace_back(name);
        save(configs.size() - 1);
    }
}

void Config::remove(size_t id) noexcept
{
    std::error_code ec;
    std::filesystem::remove(path / (const char8_t*)configs[id].c_str(), ec);
    configs.erase(configs.cbegin() + id);
}

void Config::rename(size_t item, const char* newName) noexcept
{
    std::error_code ec;
    std::filesystem::rename(path / (const char8_t*)configs[item].c_str(), path / (const char8_t*)newName, ec);
    configs[item] = newName;
}

void Config::reset() noexcept
{
    aimbot = { };
    antiAim = { };
    fakelag = { };
    tickbase = { };
    backtrack = { };
    triggerbot = { };
    chams = { };
    streamProofESP = { };
    visuals = { };
    misc = { };
}

void Config::listConfigs() noexcept
{
    configs.clear();

    std::error_code ec;
    std::transform(std::filesystem::directory_iterator{ path, ec },
        std::filesystem::directory_iterator{ },
        std::back_inserter(configs),
        [](const auto& entry) { return std::string{ (const char*)entry.path().filename().u8string().c_str() }; });
}

void Config::createConfigDir() const noexcept
{
    std::error_code ec; std::filesystem::create_directory(path, ec);
}

void Config::openConfigDir() const noexcept
{
    createConfigDir();
    ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

static auto getFontData(const std::string& fontName) noexcept
{
    HFONT font = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName.c_str());

    std::unique_ptr<std::byte[]> data;
    DWORD dataSize = GDI_ERROR;

    if (font) {
        HDC hdc = CreateCompatibleDC(nullptr);

        if (hdc) {
            SelectObject(hdc, font);
            dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

            if (dataSize != GDI_ERROR) {
                data = std::make_unique<std::byte[]>(dataSize);
                dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

                if (dataSize == GDI_ERROR)
                    data.reset();
            }
            DeleteDC(hdc);
        }
        DeleteObject(font);
    }
    return std::make_pair(std::move(data), dataSize);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& fontName : scheduledFonts) {
        if (fontName == "Default") {
            if (fonts.find("Default") == fonts.cend()) {
                ImFontConfig cfg;
                cfg.OversampleH = cfg.OversampleV = 1;
                cfg.PixelSnapH = true;
                cfg.RasterizerMultiply = 1.7f;

                Font newFont;

                cfg.SizePixels = 13.0f;
                newFont.big = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 10.0f;
                newFont.medium = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                cfg.SizePixels = 8.0f;
                newFont.tiny = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

                fonts.emplace(fontName, newFont);
                result = true;
            }
            continue;
        }

        const auto [fontData, fontDataSize] = getFontData(fontName);
        if (fontDataSize == GDI_ERROR)
            continue;

        if (fonts.find(fontName) == fonts.cend()) {
            const auto ranges = Helpers::getFontGlyphRanges();
            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;
            cfg.RasterizerMultiply = 1.7f;

            Font newFont;
            newFont.tiny = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 8.0f, &cfg, ranges);
            newFont.medium = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 10.0f, &cfg, ranges);
            newFont.big = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 13.0f, &cfg, ranges);
            fonts.emplace(fontName, newFont);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
