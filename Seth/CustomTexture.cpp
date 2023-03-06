#include "CustomTexture.h"

#include "imgui/imgui_impl_dx9.h"

void CustomTexture::init(int width, int height, const std::uint8_t* data) noexcept
{
    texture = ImGui_CreateTextureRGBA(width, height, data);
}

void CustomTexture::clear() noexcept
{
    if (texture)
        ImGui_DestroyTexture(texture);
    texture = nullptr;
}
