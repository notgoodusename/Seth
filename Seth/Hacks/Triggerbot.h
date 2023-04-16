#pragma once

struct UserCmd;

namespace Triggerbot
{
    void run(UserCmd* cmd) noexcept;
    void updateInput() noexcept;
    void reset() noexcept;
}