#include "SteamInterfaces.h"

#include "Interfaces.h"

SteamInterfaces::SteamInterfaces() noexcept
{
	const auto newPipe = interfaces->steamClient->createSteamPipe();
	const auto newUser = interfaces->steamClient->connectToGlobalUser(newPipe);

	friends = interfaces->steamClient->getSteamFriends(newUser, newPipe, "SteamFriends015");
	utils = interfaces->steamClient->getSteamUtils(newPipe, "SteamUtils007");
}