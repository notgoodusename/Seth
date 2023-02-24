#pragma once
#include <array>
#include <tuple>

#include "VirtualMethod.h"

class RenderView
{
public:
	VIRTUAL_METHOD(void, setBlend, 4, (float blend), (this, blend))
	VIRTUAL_METHOD(void, setColorModulation, 6, (const float* col), (this, col))

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
