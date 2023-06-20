#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>

#include "SDK/Platform.h"

class BaseFileSystem;
class Client;
class Cvar;
class DebugOverlay;
class Engine;
class EngineSound;
class EngineTrace;
class EntityList;
class FileSystem;
class GameEventManager;
class GameMovement;
class GameUI;
class InputSystem;
class Localize;
class MaterialSystem;
class MDLCache;
class ModelInfo;
class ModelRender;
class NetworkStringTableContainer;
class Panel;
class PhysicsSurfaceProps;
class Prediction;
class RenderView;
class Surface;
class Server;
class SteamClient;
class SoundEmitter;
class StudioRender;

class Interfaces {
public:
#define GAME_INTERFACE(type, name, moduleName, version) \
type* name = reinterpret_cast<type*>(find(moduleName, version));

    GAME_INTERFACE(BaseFileSystem, baseFileSystem, FILESYSTEM_DLL, "VBaseFileSystem011")
    GAME_INTERFACE(Client, client, CLIENT_DLL, "VClient017")
    GAME_INTERFACE(Cvar, cvar, VSTDLIB_DLL, "VEngineCvar004")
    GAME_INTERFACE(DebugOverlay, debugOverlay, ENGINE_DLL, "VDebugOverlay003")
    GAME_INTERFACE(Engine, engine, ENGINE_DLL, "VEngineClient013")
    GAME_INTERFACE(EngineTrace, engineTrace, ENGINE_DLL, "EngineTraceClient003")
    GAME_INTERFACE(EntityList, entityList, CLIENT_DLL, "VClientEntityList003")
    GAME_INTERFACE(FileSystem, fileSystem, FILESYSTEM_DLL, "VFileSystem022")
    GAME_INTERFACE(GameEventManager, gameEventManager, ENGINE_DLL, "GAMEEVENTSMANAGER002")
    GAME_INTERFACE(GameMovement, gameMovement, CLIENT_DLL, "GameMovement001")
    GAME_INTERFACE(GameUI, gameUI, GAMEUI_DLL, "GameUI011")
    GAME_INTERFACE(InputSystem, inputSystem, INPUTSYSTEM_DLL, "InputSystemVersion001")
    GAME_INTERFACE(Localize, localize, VGUI2_DLL, "VGUI_Localize005")
    GAME_INTERFACE(MaterialSystem, materialSystem, MATERIALSYSTEM_DLL, "VMaterialSystem081")
    GAME_INTERFACE(MDLCache, mdlCache, DATACACHE_DLL, "MDLCache004")
    GAME_INTERFACE(ModelInfo, modelInfo, ENGINE_DLL, "VModelInfoClient006")
    GAME_INTERFACE(ModelRender, modelRender, ENGINE_DLL, "VEngineModel016")
    GAME_INTERFACE(NetworkStringTableContainer, networkStringTableContainer, ENGINE_DLL, "VEngineClientStringTable001")
    GAME_INTERFACE(Panel, panel, VGUI2_DLL, "VGUI_Panel009")
    GAME_INTERFACE(PhysicsSurfaceProps, physicsSurfaceProps, VPHYSICS_DLL, "VPhysicsSurfaceProps001")
    GAME_INTERFACE(Prediction, prediction, CLIENT_DLL, "VClientPrediction001")
    GAME_INTERFACE(RenderView, renderView, ENGINE_DLL, "VEngineRenderView014")
    GAME_INTERFACE(Surface, surface, VGUIMATSURFACE_DLL, "VGUI_Surface030")
    GAME_INTERFACE(EngineSound, sound, ENGINE_DLL, "IEngineSoundClient003")
    GAME_INTERFACE(Server, server, SERVER_DLL, "ServerGameDLL012")
    GAME_INTERFACE(SteamClient, steamClient, STEAMCLIENT, "SteamClient017")
    GAME_INTERFACE(StudioRender, studioRender, STUDIORENDER_DLL, "VStudioRender025")

#undef GAME_INTERFACE
private:
    static void* find(const char* moduleName, const char* name) noexcept
    {
        if (const auto createInterface = reinterpret_cast<std::add_pointer_t<void* __cdecl(const char* name, int* returnCode)>>(
            GetProcAddress(GetModuleHandleA(moduleName), "CreateInterface")
            )) {
            if (void* foundInterface = createInterface(name, nullptr))
                return foundInterface;
        }

        MessageBoxA(nullptr, ("Failed to find " + std::string{ name } + " interface!").c_str(), "Seth", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
};

inline std::unique_ptr<const Interfaces> interfaces;
