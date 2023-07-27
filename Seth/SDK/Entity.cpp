#include "../Memory.h"
#include "../Helpers.h"
#include "../Interfaces.h"
#include "../StrayElements.h"

#include "Entity.h"
#include "GlobalVars.h"

void Entity::getPlayerName(char(&out)[128]) noexcept
{
    if (!StrayElements::getPlayerResource()) {
        strcpy(out, "unknown");
        return;
    }

    const auto name = StrayElements::getPlayerResource()->getPlayerName(index());
    if (!name)
    {
        strcpy(out, "unknown");
        return;
    }

    strcpy(out, name + '\0');
    out[127] = '\0';
}

std::uint64_t Entity::getSteamId() noexcept
{
    PlayerInfo playerInfo;
    if (!interfaces->engine->getPlayerInfo(index(), playerInfo))
        return 0;

    if (playerInfo.fakeplayer)
        return 0;

    return 0x0110000100000000ULL + playerInfo.friendsId;
}

bool Entity::isEnemy(Entity* entity) noexcept
{
    return StrayElements::friendlyFire() ? true : entity->teamNumber() != teamNumber();
}