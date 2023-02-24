#pragma once

#include "Utils.h"
#include "Vector.h"
#include "VirtualMethod.h"

class matrix3x4;
class Material;

struct Model;

struct ModelRenderInfo {
    Vector origin;
    Vector angles;
    void* renderable;
    const Model* model;
    const matrix3x4* modelToWorld;
    const matrix3x4* lightingOffset;
    const Vector* lightingOrigin;
    int flags;
    int entityIndex;
    int skin;
    int body;
    int hitboxSet;
    unsigned short mdlInstance;
};

enum class OverrideType {
    Normal = 0,
    BuildShadows,
    DepthWrite,
    SsaoDepthWrite
};

class ModelRender {
public:
    VIRTUAL_METHOD(void, forcedMaterialOverride, 1, (Material* material, OverrideType type = OverrideType::Normal), (this, material, type))
};