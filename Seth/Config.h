#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "imgui/imgui.h"

#include "Hacks/SkinChanger.h"

#include "ConfigStructs.h"
#include "InputUtil.h"

class Config {
public:
    Config() noexcept;
    void load(size_t, bool incremental) noexcept;
    void load(const char8_t* name, bool incremental) noexcept;
    void save(size_t) const noexcept;
    void add(const char*) noexcept;
    void remove(size_t) noexcept;
    void rename(size_t, const char*) noexcept;
    void reset() noexcept;
    void listConfigs() noexcept;
    void createConfigDir() const noexcept;
    void openConfigDir() const noexcept;

    constexpr auto& getConfigs() noexcept
    {
        return configs;
    }

    struct Fakelag {
        bool enabled = false;
        int mode = 0;
        int limit = 1;
    } fakelag;

    struct AntiAim {
        bool enabled = false;
        int pitch = 0; //Off, Down, Zero, Up
        Yaw yawBase = Yaw::off;
        KeyBind manualForward{ std::string("manual forward"), KeyMode::Off },
            manualBackward{ std::string("manual backward"), KeyMode::Off },
            manualRight{ std::string("manual right"), KeyMode::Off },
            manualLeft{ std::string("manual left"), KeyMode::Off };
        int yawModifier = 0; //Off, Jitter
        int yawAdd = 0; //-180/180
        int spinBase = 0; //-180/180
        int jitterRange = 0;
        bool atTargets = false;
    } antiAim;

    struct Aimbot {
        struct Hitscan {
            //I hate this, but otherwise it wont compile
            constexpr auto operator!=(const Hitscan& h) const noexcept
            {
                return enabled != h.enabled || aimlock != h.aimlock || silent != h.silent
                    || friendlyFire != h.friendlyFire || ignore != h.ignore
                    || scopedOnly != h.scopedOnly || targetBacktrack != h.targetBacktrack || autoShoot != h.autoShoot
                    || autoScope != h.autoScope || waitForHeadshot != h.waitForHeadshot
                    || waitForCharge != h.waitForCharge || sortMethod != h.sortMethod
                    || hitboxes != h.hitboxes || fov != h.fov || smooth != h.smooth;
            }
            bool enabled{ false };
            bool aimlock{ false };
            bool silent{ false };
            bool friendlyFire{ false };
            bool scopedOnly{ true };
            bool targetBacktrack{ true };
            bool autoShoot{ false };
            bool autoScope{ false };
            bool waitForHeadshot{ false };
            bool waitForCharge{ false };
            int ignore{ 0 };
            int sortMethod{ 0 };
            int hitboxes{ 0 };
            float fov{ 255.0f };
            float smooth{ 1.0f };
        } hitscan;
        struct Projectile {
            constexpr auto operator!=(const Projectile& p) const noexcept
            {
                return enabled != p.enabled || aimlock != p.aimlock || silent != p.silent || friendlyFire != p.friendlyFire ||
                    ignore != p.ignore || autoShoot != p.autoShoot ||
                    sortMethod != p.sortMethod || fov != p.fov
                    || maxTime != p.maxTime;
            }
            bool enabled{ false };
            bool aimlock{ false };
            bool silent{ false };
            bool friendlyFire{ false };
            bool autoShoot{ false };
            int ignore{ 0 };
            int sortMethod{ 0 };
            float fov{ 255.0f };
            float maxTime{ 1.0f };
        } projectile;
        struct Melee {
            constexpr auto operator!=(const Melee& m) const noexcept
            {
                return enabled != m.enabled || aimlock != m.aimlock || silent != m.silent 
                    || friendlyFire != m.friendlyFire || ignore != m.ignore
                    || targetBacktrack != m.targetBacktrack|| autoHit != m.autoHit
                    || autoBackstab != m.autoBackstab || sortMethod != m.sortMethod
                    || fov != m.fov;
            }
            bool enabled{ false };
            bool aimlock{ false };
            bool silent{ false };
            bool friendlyFire{ false };
            bool targetBacktrack{ true };
            bool autoHit{ false };
            bool autoBackstab{ true };
            int ignore{ 0 };
            int sortMethod{ 0 };
            float fov{ 255.0f };
        } melee;
    } aimbot;
    KeyBind aimbotKey{ std::string("aimbot") };
    ColorToggleOutline aimbotFov{ 1.0f, 1.0f, 1.0f, 0.25f };

    struct HitscanTriggerbot {
        bool enabled{ false };
        bool magnet{ false };
        bool friendlyFire{ false };
        bool targetBacktrack{ true };
        bool scopedOnly{ true };
        int ignore{ 0 };
        int hitboxes{ 0 };
        int shotDelay{ 0 };
    } hitscanTriggerbot;

    struct ProjectileTriggerbot {
        bool enabled{ false };

        struct AutoDetonate
        {
            bool enabled{ false };
            bool silent{ true };
            bool friendlyFire{ false };
            int ignore{ 0 };
        } autoDetonate;

   } projectileTriggerbot;

    struct MeleeTriggerbot {
        bool enabled{ false };
        bool friendlyFire{ false };
        bool targetBacktrack{ true };
        bool autoBackstab{ true };
        int ignore{ 0 };
        int shotDelay{ 0 };
    } meleeTriggerbot;

    KeyBind triggerbotKey{ std::string("triggerbot") };

    struct Backtrack {
        bool enabled = false;
        int timeLimit = 200;
        bool fakeLatency = false;
        int fakeLatencyAmount = 800;
    } backtrack;

    struct Chams {
        struct Material : Color4 {
            bool enabled = false;
            bool healthBased = false;
            bool blinking = false;
            bool wireframe = false;
            bool cover = false;
            bool ignorez = false;
            int material = 0;
        };
        std::array<Material, 7> materials;
    };

    struct BuildingChams {
        Chams all, enemies, allies;
    } buildingChams;

    struct WorldChams {
        Chams all, ammoPacks, healthPacks, other;
    } worldChams;

    std::unordered_map<std::string, Chams> chams;
    KeyBind chamsKey{ std::string("chams") };

    struct GlowItem : Color4 {
        bool enabled = false;
        bool healthBased = false;
    };

    struct BuildingGlow {
        GlowItem all, enemies, allies;
    };

    struct WorldGlow {
        GlowItem all, ammoPacks, healthPacks, other;
    };

    std::unordered_map<std::string, GlowItem> glow;
    std::unordered_map<std::string, BuildingGlow> buildingGlow;
    std::unordered_map<std::string, WorldGlow> worldGlow;
    KeyBind glowKey{ std::string("glow") };


    struct StreamProofESP {
        KeyBind key{ std::string("esp") };

        std::unordered_map<std::string, Player> allies;
        std::unordered_map<std::string, Player> enemies;
        std::unordered_map<std::string, Buildings> buildings;
        std::unordered_map<std::string, NPCs> npcs;
        std::unordered_map<std::string, World> world; 
    } streamProofESP;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    struct Visuals {
        bool disablePostProcessing{ false };
        bool disableCustomDecals{ false };
        bool noFog{ false };
        bool noScopeOverlay{ false };
        bool thirdperson{ false };
        KeyBind thirdpersonKey{ std::string("thirdperson") };
        bool freeCam{ false };
        KeyBind freeCamKey{ std::string("freecam") };
        int freeCamSpeed{ 2 };
        int fov{ 0 };
        struct ProjectileTrajectory
        {
            bool enabled{ false };
            Color4 trailColor{ 0.67f, 0.85f, 0.9f, 0.8f };
            Color4 bboxColor{ 0.67f, 0.85f, 0.9f, 0.6f };
        } projectileTrajectory;
        ColorToggle bulletImpacts{ 0.0f, 0.0f, 1.f, 0.5f };
        float bulletImpactsTime{ 4.f };
        struct BulletTracers
        {
            bool enabled{ false };
            int type { 0 };
        } bulletTracers;
        struct Viewmodel
        {
            bool enabled { false };
            float x { 0.0f };
            float y { 0.0f };
            float z { 0.0f };
            float roll { 0.0f };
        } viewModel;
        struct OnHitHitbox
        {
            ColorToggle color{ 1.f, 1.f, 1.f, 1.f };
            float duration = 2.f;
        } onHitHitbox;
    } visuals;

    std::vector<itemSetting> skinChanger;

    struct Misc {
        Misc() { menuKey.keyMode = KeyMode::Toggle; }

        KeyBind menuKey = KeyBind::INSERT;
        bool antiAfkKick{ false };
        bool autoStrafe{ false };
        bool bunnyHop{ false };

        struct Crithack
        {
            bool enabled{ false };
            bool skipRandomCrits{ false };
            ImVec2 pos;
        } critHack;

        struct Tickbase {
            bool enabled{ false };

            KeyBind rechargeKey{ std::string("recharge"), KeyMode::Off };
            KeyBind doubleTapKey{ std::string("doubletap"), KeyMode::Off };
            KeyBind warpKey{ std::string("shift"), KeyMode::Off };

            bool antiWarp{ false };

            int ticksToShift{ 21 };

            bool autoRecharge{ true };
            float timeTillRecharge{ 1.0f };
            bool shiftOnDisable{ true };
        } tickBase;

        KeyBind forceCritKey{ std::string("crithack") };
        bool edgeJump{ false };
        KeyBind edgeJumpKey{ std::string("edgejump") };
        bool autoAccept{ false };
        bool revealVotes{ false };
        bool fastStop{ false };
        bool disableCustomDecals{ false };
        bool svPureBypass{ true };
        bool unhideConvars{ false };
        bool backpackExpander{ false };

        struct SpectatorList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        SpectatorList spectatorList;

        struct KeyBindList {
            bool enabled = false;
            bool noTitleBar = false;
            ImVec2 pos;
        };

        KeyBindList keybindList;

        struct Logger {
            int modes{ 0 };
            int events{ 0 };
        };

        Logger loggerOptions;

        ColorToggle3 logger;

        struct Watermark {
            bool enabled = false;
            ImVec2 pos;
        };
        Watermark watermark;

        struct PlayerList {
            bool enabled = false;
            bool steamID = false;
            bool className = false;
            bool health = true;

            ImVec2 pos;
        };

        PlayerList playerList;
        OffscreenEnemies offscreenEnemies;
    } misc;

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;
    const auto& getSystemFonts() noexcept { return systemFonts; }
    const auto& getFonts() noexcept { return fonts; }
private:
    std::vector<std::string> scheduledFonts{ "Default" };
    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;
    std::filesystem::path path;
    std::vector<std::string> configs;
};

inline std::unique_ptr<Config> config;
