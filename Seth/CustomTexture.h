#pragma once

#include <cstdint>

using ImTextureID = void*;

class CustomTexture {
    ImTextureID texture = nullptr;
public:
    CustomTexture() = default;
    ~CustomTexture() { clear(); }
    CustomTexture(const CustomTexture&) = delete;
    CustomTexture& operator=(const CustomTexture&) = delete;
    CustomTexture(CustomTexture&& other) noexcept : texture{ other.texture } { other.texture = nullptr; }
    CustomTexture& operator=(CustomTexture&& other) noexcept { clear(); texture = other.texture; other.texture = nullptr; return *this; }

    void init(int width, int height, const std::uint8_t* data) noexcept;
    void clear() noexcept;
    ImTextureID get() noexcept { return texture; }
};
