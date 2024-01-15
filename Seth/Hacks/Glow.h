#pragma once

namespace Glow
{
    void render() noexcept;

    bool hasDrawn(int handle) noexcept;
    bool isRenderingGlow() noexcept;
    
    void updateInput() noexcept;
}