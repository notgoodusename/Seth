#pragma once
#include <vector>

struct UserCmd;

namespace TargetSystem
{
    void updateTargets(UserCmd*) noexcept;

    struct Enemy
    {
        int id;
        float distance;
        float fov;

        Enemy(int id, float distance, float fov) noexcept : id(id), distance(distance), fov(fov) { }
    };

    const std::vector<Enemy>& getTargets(int sortType) noexcept;

    void reset() noexcept;
};

enum SortType
{
    Distance,
    Fov
};