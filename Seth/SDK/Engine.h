#pragma once

#include <cstdint>
#include <functional>

#include "matrix3x4.h"
#include "Pad.h"
#include "Vector.h"
#include "VirtualMethod.h"

#define FRUSTUM_NUMPLANES 6

struct CPlane
{
public:
    Vector normal;
    float dist;
    unsigned char type;
    unsigned char signbits;
    PAD(2)
};

struct Frustum
{
public:
    CPlane plane[FRUSTUM_NUMPLANES];
    Vector absNormal[FRUSTUM_NUMPLANES];
};

struct Matrix4x4
{
private:
    Vector m[4][4];

public:
    const matrix3x4& as3x4() const
    {
        return *((const matrix3x4*)this);
    }
};

struct PlayerInfo {
    char name[32];
    int userId;
    char guid[33];
    std::uint32_t friendsId;
    char friendsName[32];
    bool fakeplayer;
    bool hltv;
    unsigned long customfiles[4];
    unsigned char filesDownloaded;
};

struct DemoPlaybackParameters {
    PAD(16)
    bool anonymousPlayerIdentity;
    PAD(23)
};

class NetworkChannel;

class Engine {
public:
    VIRTUAL_METHOD(void, getScreenSize, 5, (int& w, int& h), (this, std::ref(w), std::ref(h)))
    VIRTUAL_METHOD(bool, getPlayerInfo, 8, (int entityIndex, PlayerInfo& playerInfo), (this, entityIndex, std::ref(playerInfo)))
    VIRTUAL_METHOD(int, getPlayerForUserID, 9, (int userId), (this, userId))
    VIRTUAL_METHOD(void, getViewAngles, 19, (Vector& angles), (this, std::ref(angles)))
    VIRTUAL_METHOD(void, setViewAngles, 20, (const Vector& angles), (this, std::cref(angles)))
    VIRTUAL_METHOD(int, getMaxClients, 21, (), (this))
    VIRTUAL_METHOD(bool, isInGame, 26, (), (this))
    VIRTUAL_METHOD(bool, isConnected, 27, (), (this))
    VIRTUAL_METHOD(bool, isDrawingLoadingImage, 28, (), (this))
    VIRTUAL_METHOD(bool, cullBox, 33, (const Vector& mins, const Vector& maxs), (this, std::cref(mins), std::cref(maxs)))
    VIRTUAL_METHOD(const char*, getGameDirectory, 35, (), (this))
    VIRTUAL_METHOD(const Matrix4x4&, worldToScreenMatrix, 36, (), (this))
    VIRTUAL_METHOD(void*, getBSPTreeQuery, 42, (), (this))
    VIRTUAL_METHOD(const char*, getLevelName, 51, (), (this))
    VIRTUAL_METHOD(void, fireEvents, 56, (), (this))
    VIRTUAL_METHOD(NetworkChannel*, getNetworkChannel, 72, (), (this))
    VIRTUAL_METHOD(bool, isPaused, 84, (), (this))
    VIRTUAL_METHOD(bool, isHLTV, 86, (), (this))
    VIRTUAL_METHOD(void, clientCmdUnrestricted, 106, (const char* cmd), (this, cmd))

    auto getViewAngles() noexcept
    {
        Vector ang;
        getViewAngles(ang);
        return ang;
    }
};
