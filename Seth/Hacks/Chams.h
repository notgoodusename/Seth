#pragma once

#include <vector>

#include "../Config.h"

class Entity;
struct ModelRenderInfo;
class matrix3x4;
class Material;

class Chams {
public:
    bool render(void*, const ModelRenderInfo&, matrix3x4*) noexcept;
    static void updateInput() noexcept;
private:
    void renderBuilding(Entity* building) noexcept;
    void renderWorld(Entity* worldEntity) noexcept;
    void renderNPCs(Entity* npc) noexcept;
    void renderPlayer(Entity* player) noexcept;

    bool appliedChams;
    void* state;
    const ModelRenderInfo* info;
    matrix3x4* customBoneToWorld;

    void applyChams(const std::array<Config::Chams::Material, 7>& chams, int health = 0, int maxHealth = 100, const matrix3x4* customMatrix = nullptr) noexcept;
};
