#pragma once

#include <forward_list>
#include <list>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "SDK/matrix3x4.h"
#include "SDK/Vector.h"

#include "CustomTexture.h"

struct LocalPlayerData;

struct PlayerData;
struct BuildingsData;
struct EntityData;
struct NPCData;
struct WorldData;

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

    void clearTextures() noexcept;
    void clearUnusedAvatars() noexcept;

    //Lock-free
    int getNetOutgoingLatency() noexcept;

    // You have to acquire Lock before using these getters
    const Matrix4x4& toScreenMatrix() noexcept;
    const LocalPlayerData& local() noexcept;
    const std::vector<PlayerData>& players() noexcept;
    const PlayerData* playerByHandle(int handle) noexcept;
    const std::vector<BuildingsData>& buildings() noexcept;
    const std::vector<NPCData>& npcs() noexcept;
    const std::vector<WorldData>& world() noexcept;
}

enum class Team;
enum class TFClass;

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
    ImTextureID getAvatarTexture() const noexcept;
    float fadingAlpha() const noexcept;

    bool dormant;
    bool enemy = false;
    bool alive;
    bool inViewFrustum;
    float lastContactTime = 0.0f;
    int health;
    int maxHealth;
    int userId;
    int handle;
    bool isCloaked;
    Team team;
    std::uint64_t steamID;
    const char* name;
    TFClass classID;
    Vector origin;
    std::string activeWeapon;
    std::vector<std::pair<Vector, Vector>> bones;

    class CustomTexture {
        ImTextureID texture = nullptr;
    public:
        CustomTexture() = default;
        ~CustomTexture();
        CustomTexture(const CustomTexture&) = delete;
        CustomTexture& operator=(const CustomTexture&) = delete;
        CustomTexture(CustomTexture&& other) noexcept : texture{ other.texture } { other.texture = nullptr; }
        CustomTexture& operator=(CustomTexture&& other) noexcept { clear(); texture = other.texture; other.texture = nullptr; return *this; }

        void init(int width, int height, const std::uint8_t* data) noexcept;
        void clear() noexcept;
        ImTextureID get() const noexcept { return texture; }
    };
};

struct NPCData : BaseData {
    NPCData(Entity* npc) noexcept;

    std::string name;
};

struct BuildingsData : BaseData {
    BuildingsData(Entity* building) noexcept;

    bool enemy = false;
    bool alive;
    std::string name;
    std::string owner;
    int health;
    int maxHealth;
};

struct WorldData : BaseData {
    WorldData(Entity* worldEntity) noexcept;

    std::string name;
};