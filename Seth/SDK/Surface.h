#pragma once

#include <functional>
#include <tuple>

#include "Utils.h"
#include "VirtualMethod.h"

class Surface {
public:
    static constexpr unsigned font{ 0x1d }; // builtin font from vgui_spew_fonts

    VIRTUAL_METHOD(void, setDrawColor, 11, (int r, int g, int b, int a = 255), (this, r, g, b, a))
    VIRTUAL_METHOD(void, setDrawColor, 11, (const std::array<float, 3>& color, int a = 255), (this, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a))
    VIRTUAL_METHOD(void, setDrawColor, 11, (std::tuple<float, float, float> color, int a = 255), (this, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a))

    template <typename T>
    VIRTUAL_METHOD(void, drawFilledRect, 12, (T x0, T y0, T x1, T y1), (this, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1)))

    template <typename T>
    VIRTUAL_METHOD(void, drawOutlinedRect, 14, (T x0, T y0, T x1, T y1), (this, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1)))

    template <typename T>
    VIRTUAL_METHOD(void, drawLine, 15, (T x0, T y0, T x1, T y1), (this, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1)))

    VIRTUAL_METHOD(void, drawPolyLine, 16, (int* xs, int* ys, int pointCount), (this, xs, ys, pointCount))
    VIRTUAL_METHOD(void, setTextFont, 17, (unsigned font), (this, font))
    VIRTUAL_METHOD(void, setTextColor, 19, (int r, int g, int b, int a = 255), (this, r, g, b, a))
    VIRTUAL_METHOD(void, setTextColor, 19, (const std::array<float, 3> color, int a = 255), (this, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a))
    VIRTUAL_METHOD(void, setTextColor, 19, (std::tuple<float, float, float> color, int a = 255), (this, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a))

    template <typename T>
    VIRTUAL_METHOD(void, setTextPosition, 20, (T x, T y), (this, static_cast<int>(x), static_cast<int>(y)))
    
    VIRTUAL_METHOD(void, getScreenSize, 38, (int& w, int& h), (this, std::ref(w), std::ref(h)))

    VIRTUAL_METHOD(void, setCursorsAlwaysVisible, 52, (bool enable), (this, enable))

    VIRTUAL_METHOD(void, unlockCursor, 61, (), (this))
    VIRTUAL_METHOD(unsigned, createFont, 66, (), (this))
    VIRTUAL_METHOD(bool, setFontGlyphSet, 67, (unsigned font, const char* fontName, int tall, int weight, int blur, int scanlines, int flags, int rangeMin = 0, int rangeMax = 0), (this, font, fontName, tall, weight, blur, scanlines, flags, rangeMin, rangeMax))

    auto getTextSize(unsigned font, const wchar_t* text) noexcept
    {
        int width, height;
        VirtualMethod::call<void, 75>(this, font, text, std::ref(width), std::ref(height));
        return std::make_pair(width, height);
    }

    template <typename T>
    VIRTUAL_METHOD(void, drawOutlinedCircle, 99, (T x, T y, int r, int seg), (this, static_cast<int>(x), static_cast<int>(y), r, seg))

    template <typename T>
    void drawCircle(T x, T y, int startRadius, int radius) noexcept
    {
        int xs[12];
        int ys[12];

        for (int i = startRadius; i <= radius; ++i) {
            for (int j = 0; j < 12; ++j) {
                xs[j] = static_cast<int>(std::cos(Helpers::deg2rad(static_cast<float>(j * 30))) * i + x);
                ys[j] = static_cast<int>(std::sin(Helpers::deg2rad(static_cast<float>(j * 30))) * i + y);
            }
            interfaces->surface->drawPolyLine(xs, ys, 12);
        }
    }
};
