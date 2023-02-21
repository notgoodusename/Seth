#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"

#include "Entity.h"
#include "GlobalVars.h"

std::string Entity::getPlayerName() noexcept
{
    std::string playerName = "unknown";

    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
        return playerName;

    playerName = playerInfo.name;

    if (wchar_t wide[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, 128, wide, 128)) {
        if (wchar_t wideNormalized[128]; NormalizeString(NormalizationKC, wide, -1, wideNormalized, 128)) {
            if (char nameNormalized[128]; WideCharToMultiByte(CP_UTF8, 0, wideNormalized, -1, nameNormalized, 128, nullptr, nullptr))
                playerName = nameNormalized;
        }
    }

    playerName.erase(std::remove(playerName.begin(), playerName.end(), '\n'), playerName.cend());
    return playerName;
}