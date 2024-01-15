#pragma once

#include "VirtualMethod.h"

class Texture;
class Material;

class MaterialRenderContext
{
public:
	VIRTUAL_METHOD(void, setRenderTarget, 6, (Texture* texture), (this, texture))
	VIRTUAL_METHOD(void, depthRange, 11, (float zNear, float zFar), (this, zNear, zFar))
	VIRTUAL_METHOD(void, clearBuffers, 12, (bool clearColor, bool clearDepth, bool clearStencil = false), (this, clearColor, clearDepth, clearStencil))
	VIRTUAL_METHOD(void, viewport, 38, (int x, int y, int width, int height), (this, x, y, width, height))
	VIRTUAL_METHOD(void, clearColor4ub, 73, (unsigned char r, unsigned char g, unsigned char b, unsigned char a), (this, r, g, b, a))
	VIRTUAL_METHOD(void, drawScreenSpaceRectangle, 103, (Material* material, int destX, int destY, int width, int height, float srcTextureX0, float srcTextureY0, float srcTextureX1, float srcTextureY1, int srcTextureWidth, int srcTextureHeight, void* clientRenderable = NULL, int nXDice = 1, int nYDice = 1), (this, material, destX, destY, width, height, srcTextureX0, srcTextureY0, srcTextureX1, srcTextureY1, srcTextureWidth, srcTextureHeight, clientRenderable, nXDice, nYDice))
	VIRTUAL_METHOD(void, pushRenderTargetAndViewport, 108, (), (this))
	VIRTUAL_METHOD(void, popRenderTargetAndViewport, 109, (), (this))
	VIRTUAL_METHOD(void, setStencilEnable, 117, (bool enabled), (this, enabled))
    VIRTUAL_METHOD(void, setStencilFailOperation, 118, (int operation), (this, operation))
	VIRTUAL_METHOD(void, setStencilZFailOperation, 119, (int operation), (this, operation))
	VIRTUAL_METHOD(void, setStencilPassOperation, 120, (int operation), (this, operation))
	VIRTUAL_METHOD(void, setStencilCompareFunction, 121, (int comparisonOperation), (this, comparisonOperation))
	VIRTUAL_METHOD(void, setStencilReferenceValue, 122, (int reference), (this, reference))
	VIRTUAL_METHOD(void, setStencilTestMask, 123, (unsigned int mask), (this, mask))
	VIRTUAL_METHOD(void, setStencilWriteMask, 124, (unsigned int mask), (this, mask))
};