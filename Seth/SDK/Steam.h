#pragma once

#include <cstdint>

#include "Pad.h"
#include "VirtualMethod.h"

class SteamFriends {
public:
    VIRTUAL_METHOD(int, getSmallFriendAvatar, 34, (std::uint64_t steamID), (this, steamID))
};

class SteamUtils {
public:
    VIRTUAL_METHOD(bool, getImageRGBA, 6, (int image, std::uint8_t* buff, int buffSize), (this, image, buff, buffSize))
};

class SteamClient
{
public:
    VIRTUAL_METHOD(int, createSteamPipe, 0, (), (this))
    VIRTUAL_METHOD(int, connectToGlobalUser, 2, (int steamPipe), (this, steamPipe))
    VIRTUAL_METHOD(SteamFriends*, getSteamFriends, 8, (int steamUser, int steamPipe, const char* version), (this, steamUser, steamPipe, version))
    VIRTUAL_METHOD(SteamUtils*, getSteamUtils, 9, (int steamPipe, const char* version), (this, steamPipe, version))
};