#pragma once

#include <cstdint>

#include "Platform.h"

class HudChat {
public:
    template <typename... Args>
    void printf(const char* fmt, Args... args) noexcept
    {
        (*reinterpret_cast<void(__cdecl***)(void*, int, const char*, ...)>(this))[18](this, 0, fmt, args...);
    }
};

class ClientMode {
public:
    auto getHudChat() noexcept
    {
        return *reinterpret_cast<HudChat**>(std::uintptr_t(this) + 28);
    }
};
