#pragma once

#include <array>

enum class FrameStage;
class GameEvent;
struct ImDrawList;
class ViewSetup;
struct UserCmd;

namespace Visuals
{
    void drawAimbotFov(ImDrawList* drawList) noexcept;
    void thirdperson() noexcept;
    void disablePostProcessing(FrameStage stage) noexcept;

    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
