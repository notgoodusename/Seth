#pragma once
#include <array>
#include <tuple>

#include "VirtualMethod.h"
#include "ViewSetup.h"

struct Matrix4x4;

class RenderView
{
public:
	VIRTUAL_METHOD(void, setBlend, 4, (float blend), (this, blend))
	VIRTUAL_METHOD(float, getBlend, 5, (), (this))
	VIRTUAL_METHOD(void, setColorModulation, 6, (const float* col), (this, col))
	VIRTUAL_METHOD(void, getColorModulation, 7, (float* col), (this, col))
	VIRTUAL_METHOD(void, getMatricesForView, 50, (const ViewSetup& viewSetup, Matrix4x4* worldToView, Matrix4x4* viewToProjection, Matrix4x4* worldToProjection, Matrix4x4* worldToPixels), (this, &viewSetup, worldToView, viewToProjection, worldToProjection, worldToPixels))

	void setColorModulation(float r, float g, float b) noexcept
	{
		float clr[3] = { r, g, b };
		return setColorModulation(clr);
	}

	void setColorModulation(const std::array<float, 3>& color) noexcept
	{
		float clr[3] = { color[0], color[1], color[2] };
		return setColorModulation(clr);
	}

	void setColorModulation(const std::tuple<float, float, float>& color) noexcept
	{
		float clr[3] = { std::get<0>(color), std::get<1>(color), std::get<2>(color) };
		return setColorModulation(clr);
	}
};
