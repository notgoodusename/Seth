#pragma once

#include <array>

enum class FrameStage;
struct ImDrawList;
class ViewSetup;
struct UserCmd;

namespace Visuals
{
    void thirdperson() noexcept;
    void disablePostProcessing(FrameStage stage) noexcept;

    void updateInput() noexcept;
    void reset(int resetType) noexcept;
}
