#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "imgui/imgui.h"

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

    struct Tickbase {
        KeyBind doubletap{ std::string("doubletap"), KeyMode::Off };
        KeyBind hideshots{ std::string("hideshots"), KeyMode::Off };
        bool teleport{ false };
    } tickbase;

    struct Aimbot {
        struct Hitscan {
            //I hate this, but otherwise it wont compile
            constexpr auto operator!=(const Hitscan& h) const noexcept
            {
                return enabled != h.enabled || aimlock != h.aimlock || silent != h.silent
                    || friendlyFire != h.friendlyFire || ignoreCloaked != h.ignoreCloaked
                    || scopedOnly != h.scopedOnly || targetBacktrack != h.targetBacktrack || autoShoot != h.autoShoot
                    || autoScope != h.autoScope || autoRev != h.autoRev 
                    || autoExtinguishTeam != h.autoExtinguishTeam || waitForHeadshot != h.waitForHeadshot
                    || waitForCharge != h.waitForCharge || sortMethod != h.sortMethod
                    || hitboxes != h.hitboxes || fov != h.fov || smooth != h.smooth;
            }
            bool enabled{ false };
            bool aimlock{ false };
            bool silent{ false };
            bool friendlyFire{ false };
            bool ignoreCloaked{ true };
            bool scopedOnly{ true };
            bool targetBacktrack{ true };
            bool autoShoot{ false };
            bool autoScope{ false };
            bool autoRev{ false };
            bool autoExtinguishTeam{ false };
            bool waitForHeadshot{ false };
            bool waitForCharge{ false };
            int sortMethod{ 0 };
            int hitboxes{ 0 };
            float fov{ 0.0f };
            float smooth{ 1.0f };
        } hitscan;
        struct Projectile {
            constexpr auto operator!=(const Projectile& p) const noexcept
            {
                return enabled != p.enabled;
            }
            bool enabled{ false };
        } projectile;
        struct Melee {
            constexpr auto operator!=(const Melee& m) const noexcept
            {
                return enabled != m.enabled || aimlock != m.aimlock || silent != m.silent 
                    || friendlyFire != m.friendlyFire || ignoreCloaked != m.ignoreCloaked
                    || targetBacktrack != m.targetBacktrack|| autoHit != m.autoHit
                    || autoBackstab != m.autoBackstab || sortMethod != m.sortMethod
                    || fov != m.fov;
            }
            bool enabled{ false };
            bool aimlock{ false };
            bool silent{ false };
            bool friendlyFire{ false };
            bool ignoreCloaked{ true };
            bool targetBacktrack{ true };
            bool autoHit{ false };
            bool autoBackstab{ true };
            int sortMethod{ 0 };
            float fov{ 0.0f };
        } melee;
    } aimbot;
    KeyBind aimbotKey{ std::string("aimbot") };
    ColorToggleOutline aimbotFov{ 1.0f, 1.0f, 1.0f, 0.25f };

    struct Triggerbot {
        bool enabled = false;
        bool friendlyFire = false;
        bool scopedOnly = true;
        int hitboxes = 0;
        int shotDelay = 0;
    } triggerbot;
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

    std::unordered_map<std::string, Chams> chams;
    KeyBind chamsKey{ std::string("chams") };

    struct GlowItem : Color4 {
        bool enabled = false;
        bool healthBased = false;
        int style = 0;
    };

    struct PlayerGlow {
        GlowItem all, visible, occluded;
    };

    std::unordered_map<std::string, PlayerGlow> playerGlow;
    std::unordered_map<std::string, GlowItem> glow;
    KeyBind glowKey{ std::string("glow") };


    struct StreamProofESP {
        KeyBind key{ std::string("esp") };

        std::unordered_map<std::string, Player> allies;
        std::unordered_map<std::string, Player> enemies;
        std::unordered_map<std::string, Buildings> buildings;
        std::unordered_map<std::string, World> world; 
    } streamProofESP;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    struct Visuals {
        bool disablePostProcessing{ false };
        bool inverseRagdollGravity{ false };
        bool noFog{ false };
        bool noScopeOverlay{ false };
        bool thirdperson{ false };
        KeyBind thirdpersonKey{ std::string("thirdperson") };
        bool freeCam{ false };
        KeyBind freeCamKey{ std::string("freecam") };
        int freeCamSpeed{ 2 };
        int fov{ 0 };
        ColorToggle bulletImpacts{ 0.0f, 0.0f, 1.f, 0.5f };
        float bulletImpactsTime{ 4.f };
        BulletTracers bulletTracers;
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


    struct Misc {
        Misc() { menuKey.keyMode = KeyMode::Toggle; }

        KeyBind menuKey = KeyBind::INSERT;
        bool antiAfkKick{ false };
        bool autoStrafe{ false };
        bool bunnyHop{ false };
        bool edgeJump{ false };
        KeyBind edgeJumpKey{ std::string("edgejump") };
        bool autoAccept{ false };
        bool revealVotes{ false };
        bool fastStop{ false };
        bool svPureBypass{ true };
        bool unhideConvars{ false };

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
