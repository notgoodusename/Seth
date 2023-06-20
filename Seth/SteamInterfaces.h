#pragma once

#include <memory>
#include <type_traits>
#include <d3d9.h>
#include <Windows.h>

#include "SDK/Platform.h"
#include "SDK/Steam.h"

class SteamInterfaces {
public:
    SteamInterfaces() noexcept;

    SteamFriends* friends;
    SteamUtils* utils;
};

inline std::unique_ptr<const SteamInterfaces> steamInterfaces;
