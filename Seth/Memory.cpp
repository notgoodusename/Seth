#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string_view>
#include <utility>
#include <Windows.h>
#include <Psapi.h>

#include "SDK/LocalPlayer.h"

#include "Interfaces.h"
#include "Memory.h"


template <typename T>
static constexpr auto relativeToAbsolute(uintptr_t address) noexcept
{
    return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
}

static std::pair<void*, std::size_t> getModuleInformation(const char* name) noexcept
{
    if (HMODULE handle = GetModuleHandleA(name)) {
        if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
            return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
    }
    return {};
}

[[nodiscard]] static auto generateBadCharTable(std::string_view pattern) noexcept
{
    assert(!pattern.empty());

    std::array<std::size_t, (std::numeric_limits<std::uint8_t>::max)() + 1> table;

    auto lastWildcard = pattern.rfind('?');
    if (lastWildcard == std::string_view::npos)
        lastWildcard = 0;

    const auto defaultShift = (std::max)(std::size_t(1), pattern.length() - 1 - lastWildcard);
    table.fill(defaultShift);

    for (auto i = lastWildcard; i < pattern.length() - 1; ++i)
        table[static_cast<std::uint8_t>(pattern[i])] = pattern.length() - 1 - i;

    return table;
}

static std::uintptr_t findPattern(const char* moduleName, std::string_view pattern, bool reportNotFound = true) noexcept
{
    static auto id = 0;
    ++id;

    const auto [moduleBase, moduleSize] = getModuleInformation(moduleName);

    if (moduleBase && moduleSize) {
        const auto lastIdx = pattern.length() - 1;
        const auto badCharTable = generateBadCharTable(pattern);

        auto start = static_cast<const char*>(moduleBase);
        const auto end = start + moduleSize - pattern.length();

        while (start <= end) {
            int i = lastIdx;
            while (i >= 0 && (pattern[i] == '?' || start[i] == pattern[i]))
                --i;

            if (i < 0)
                return reinterpret_cast<std::uintptr_t>(start);

            start += badCharTable[static_cast<std::uint8_t>(start[lastIdx])];
        }
    }

    if(reportNotFound)
        MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "Seth", MB_OK | MB_ICONWARNING);
    return 0;
}

Memory::Memory() noexcept
{
    present = findPattern("gameoverlayrenderer", "\xFF\x15????\x8B\xF0\x85\xFF") + 2;
    reset = findPattern("gameoverlayrenderer", "\xC7\x45?????\xFF\x15????\x8B\xD8") + 9;

    clientMode = **reinterpret_cast<ClientMode***>(findPattern(CLIENT_DLL, "\x8B\x0D????\x8B\x02\xD9\x05") + 2);//**reinterpret_cast<ClientMode***>((*reinterpret_cast<uintptr_t**>(interfaces->client))[10] + 5);
    //input = *reinterpret_cast<Input**>((*reinterpret_cast<uintptr_t**>(interfaces->client))[16] + 1);
    globalVars = *reinterpret_cast<GlobalVars**>(findPattern(ENGINE_DLL, "\xA1????\x8B\x11\x68") + 0x8);// **reinterpret_cast<GlobalVars***>((*reinterpret_cast<uintptr_t**>(interfaces->client))[11] + 10);


    keyValuesInitialize = reinterpret_cast<decltype(keyValuesInitialize)>(findPattern(ENGINE_DLL, "\xFF\x15????\x83\xC4\x08\x89\x06\x8B\xC6") - 0x42);
    keyValuesFindKey = reinterpret_cast<decltype(keyValuesFindKey)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x81\xEC????\x56\x8B\x75\x08\x57\x8B\xF9\x85\xF6\x0F\x84????\x80\x3E\x00\x0F\x84????"));

    setAbsOrigin = reinterpret_cast<decltype(setAbsOrigin)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x57\x8B\xF1\xE8????\x8B\x7D\x08\xF3\x0F\x10\x07"));
    calcAbsoluteVelocity = relativeToAbsolute<decltype(calcAbsoluteVelocity)>(findPattern(CLIENT_DLL, "\xE8????\xD9\xE8\x8D\x45\xEC") + 1);

    estimateAbsVelocity = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC?\x56\x8B\xF1\xE8????\x3B\xF0\x75?\x8B\xCE\xE8????\x8B\x45?\xD9\x86????\xD9\x18\xD9\x86????\xD9\x58?\xD9\x86????\xD9\x58?\x5E\x8B\xE5\x5D\xC2");
    sendDatagram = findPattern(ENGINE_DLL, "\x55\x8B\xEC\xB8????\xE8????\xA1????\x53\x56\x8B\xD9");;

    localPlayer.init(*reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\xA1????\x33\xC9\x83\xC4\x04")+1));
}
