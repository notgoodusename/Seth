#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <string>

#include "SDK/Platform.h"

class ClientMode;
class ClientState;
class Entity;
class GameEventDescriptor;
class GameEventManager;
class Input;
class ItemSystem;
class KeyValues;
class MemAlloc;
class MoveHelper;
class MoveData;
class PlantedC4;
class PlayerResource;
class ViewRender;
class ViewRenderBeams;
class WeaponSystem;
template <typename Key, typename Value>
struct UtlMap;
template <typename T>
class UtlVector;

struct ActiveChannels;
struct Channel;
struct CStudioHdr;
struct GlobalVars;
struct GlowObjectManager;
struct PanoramaEventRegistration;
struct Trace;
struct Vector;

class Memory {
public:
    Memory() noexcept;

    std::uintptr_t present;
    std::uintptr_t reset;

    ClientMode* clientMode;
    GlobalVars* globalVars;
private:
};

inline std::unique_ptr<const Memory> memory;
