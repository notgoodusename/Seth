#pragma once

#include "VirtualMethod.h"

class KeyValues;
class Material;
class RenderContext;

class MaterialSystem {
public:
    VIRTUAL_METHOD(Material*, createMaterial, 72, (const char* materialName, KeyValues* keyValues), (this, materialName, keyValues))
    VIRTUAL_METHOD(Material*, findMaterial, 73, (const char* materialName, const char* textureGroupName = nullptr, bool complain = true, const char* complainPrefix = nullptr), (this, materialName, textureGroupName, complain, complainPrefix))
    VIRTUAL_METHOD(short, firstMaterial, 75, (), (this))
    VIRTUAL_METHOD(short, nextMaterial, 76, (short handle), (this, handle))
    VIRTUAL_METHOD(short, invalidMaterial, 77, (), (this))
    VIRTUAL_METHOD(Material*, getMaterial, 78, (short handle), (this, handle))
    VIRTUAL_METHOD(RenderContext*, getRenderContext, 100, (), (this))
};
