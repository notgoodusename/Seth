#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../StrayElements.h"

#include "Entity.h"
#include "GlobalVars.h"

const char* Entity::getPlayerName() noexcept
{
    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo) || !StrayElements::getPlayerResource())
        return "unknown";

    const auto name = StrayElements::getPlayerResource()->getPlayerName(index());
    if(!name)
        return "unknown";

    return name;
}

//i was just browsing around ida looking for sm else, and i found that getSteamId is a virtual method, kill me
std::uint64_t Entity::getSteamId() noexcept
{
    std::uint64_t steamId = 0;

    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
        return steamId;

    if (playerInfo.fakeplayer)
        return steamId;

    //guid gives us SteamId3, we want steamId64
    //you might think, oh why not just 76561193665298432 + steamdid3?? and the reason is because valve loves bit manipulation, 
    //so we have to add every bit to an unsigned long long, lets go
    //https://developer.valvesoftware.com/wiki/SteamID for more info
    std::string steamId3 = playerInfo.guid;
    steamId3 = steamId3.substr(5, steamId3.size() - 6); //remove the "[U:1:" and "]" part, we want numbers
    steamId = stoi(steamId3);

    bool addBit = false;
    if (steamId % 2 != 0)
    {
        addBit = true;
        steamId--;
    }
    steamId /= 2;

    std::string binaryText = "0001000100000000000000000001"; //This is basically the header of an steamid
    for (int i = 0; i <= 31; i++)
    {
        if (addBit && i == 31)
        {
            binaryText += "1";
            break;
        }
        binaryText += getBit(steamId, 31 - 1 - i) ? "1" : "0";
    }

    uint64_t base = 1; //pow is not what we need, its good yes, but it uses cpu so lets get this slightly faster approach a try
    steamId = 0;
    for (int i = binaryText.size() - 1; i >= 0; i--)
    {
        if (binaryText.at(i) == '1')
            steamId += base;
        base *= 2;
    }

    return steamId;
}