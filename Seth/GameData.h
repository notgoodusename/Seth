#pragma once

#include <forward_list>
#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "SDK/matrix3x4.h"
#include "SDK/Vector.h"

#include "Texture.h"

struct LocalPlayerData;

struct PlayerData;
struct ObserverData;
struct WeaponData;
struct EntityData;
struct LootCrateData;
struct ProjectileData;
struct BombData;
struct InfernoData;
struct SmokeData;

struct Matrix4x4;

namespace GameData
{
    void update() noexcept;

    class Lock {
    public:
        Lock() noexcept : lock{ mutex } {};
    private:
        std::scoped_lock<std::mutex> lock;
        static inline std::mutex mutex;
    };

    // You have to acquire Lock before using these getters
    const Matrix4x4& toScreenMatrix() noexcept;
    const LocalPlayerData& local() noexcept;
    const std::vector<PlayerData>& players() noexcept;
    const PlayerData* playerByHandle(int handle) noexcept;
    const std::vector<ObserverData>& observers() noexcept;
    const std::vector<WeaponData>& weapons() noexcept;
    const std::vector<EntityData>& entities() noexcept;
    const std::vector<LootCrateData>& lootCrates() noexcept;
    const std::forward_list<ProjectileData>& projectiles() noexcept;
    const BombData& plantedC4() noexcept;
    const std::string& gameMode() noexcept;
    const std::vector<InfernoData>& infernos() noexcept;
    const std::vector<SmokeData>& smokes() noexcept;
}

enum class Team;

struct LocalPlayerData {
    void update() noexcept;

    bool exists = false;
    bool alive = false;
    bool shooting = false;
    bool noScope = false;
    float nextWeaponAttack = 0.0f;
    int fov;
    int handle;
    Vector origin;
    Team team;
};

class Entity;

struct BaseData {
    BaseData(Entity* entity) noexcept;

    constexpr auto operator<(const BaseData& other) const
    {
        return distanceToLocal > other.distanceToLocal;
    }

    float distanceToLocal;
    Vector obbMins, obbMaxs;
    matrix3x4 coordinateFrame;
};

struct PlayerData : BaseData {
    PlayerData(Entity* entity) noexcept;
    PlayerData(const PlayerData&) = delete;
    PlayerData& operator=(const PlayerData&) = delete;
    PlayerData(PlayerData&&) = default;
    PlayerData& operator=(PlayerData&&) = default;

    void update(Entity* entity) noexcept;
    const std::string getRankName() const noexcept;
    ImTextureID getAvatarTexture() const noexcept;
    ImTextureID getRankTexture() const noexcept;
    float fadingAlpha() const noexcept;

    bool dormant;
    bool enemy = false;
    bool inViewFrustum;
    bool alive;
    bool immune = false;
    float lastContactTime = 0.0f;
    int health;
    int maxHealth;
    int handle;
    bool isCloaked;
    Team team;
    std::string name;
    Vector origin;
    std::string activeWeapon;
    std::vector<std::pair<Vector, Vector>> bones;

    class Texture {
        ImTextureID texture = nullptr;
    public:
        Texture() = default;
        ~Texture();
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept : texture{ other.texture } { other.texture = nullptr; }
        Texture& operator=(Texture&& other) noexcept { clear(); texture = other.texture; other.texture = nullptr; return *this; }

        void init(int width, int height, const std::uint8_t* data) noexcept;
        void clear() noexcept;
        ImTextureID get() const noexcept { return texture; }
    };
};