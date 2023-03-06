#pragma once

#include "VirtualMethod.h"
class Material;
class MaterialRenderContext;
class Texture;
class KeyValues;

class MaterialSystem {
public:
    VIRTUAL_METHOD(Material*, createMaterial, 72, (const char* materialName, KeyValues* keyValues), (this, materialName, keyValues))
    VIRTUAL_METHOD(Material*, findMaterial, 73, (const char* materialName, const char* textureGroupName = nullptr, bool complain = true, const char* complainPrefix = nullptr), (this, materialName, textureGroupName, complain, complainPrefix))
    VIRTUAL_METHOD(short, firstMaterial, 75, (), (this))
    VIRTUAL_METHOD(short, nextMaterial, 76, (short handle), (this, handle))
    VIRTUAL_METHOD(short, invalidMaterial, 77, (), (this))
    VIRTUAL_METHOD(Material*, getMaterial, 78, (short handle), (this, handle))
    VIRTUAL_METHOD(Texture*, findTexture, 81, (char const* textureName, const char* textureGroupName, bool complain = true, int additionalCreationFlags = 0), (this, textureName, textureGroupName, complain, additionalCreationFlags))
    VIRTUAL_METHOD(Texture*, createNamedRenderTargetTextureEx, 87, (const char* name, int w, int h, int sizeMode, int format, int depth = 0, unsigned int textureFlags = 4 | 8, unsigned int renderTargetFlags = 0),  (this, name, w, h, sizeMode, format, depth, textureFlags, renderTargetFlags))
    VIRTUAL_METHOD(MaterialRenderContext*, getRenderContext, 100, (), (this))
};