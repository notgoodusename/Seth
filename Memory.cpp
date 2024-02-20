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

    clientMode = **reinterpret_cast<ClientMode***>(findPattern(CLIENT_DLL, "\x8B\x0D????\x8B\x02\xD9\x05") + 2);
    clientState = *reinterpret_cast<ClientState**>(findPattern(ENGINE_DLL, "\x68????\xE8????\x83\xC4\x08\x5F\x5E\x5B\x5D\xC3") + 1);
    gameRules = *reinterpret_cast<GameRules**>(findPattern(CLIENT_DLL, "\x8B\x0D????\x8D\x78\x30\x8B\x11\xFF\x52\x7C\x83\xC0\x24\xEB\x14\xFF\x50\x7C\x8B\x0D????\x8D\x78\x18\x8B\x11\xFF\x52\x7C\x83\xC0\x0C\x57\x50\x8D\x8B????\xE8????\x53") + 2);
    globalVars = *reinterpret_cast<GlobalVars**>(findPattern(ENGINE_DLL, "\xA1????\x8B\x11\x68") + 0x8);
    input = **reinterpret_cast<Input***>(findPattern(CLIENT_DLL, "\x8B\x0D????\x56\x8B\x01\xFF\x50\x24\x8B\x45\xFC") + 2);
    itemSchema = relativeToAbsolute<decltype(itemSchema)>(findPattern(CLIENT_DLL, "\xE8????\x8A\x4F\x15") + 1);
    moveHelper = **reinterpret_cast<MoveHelper***>(findPattern(CLIENT_DLL, "\x8B\x0D????\x8B\x01\xFF\x50\x28\x56\x8B\xC8") + 2);
    memAlloc = *reinterpret_cast<MemAlloc**>(GetProcAddress(GetModuleHandleA(TIER0_DLL), "g_pMemAlloc"));

    keyValuesInitialize = reinterpret_cast<decltype(keyValuesInitialize)>(findPattern(ENGINE_DLL, "\xFF\x15????\x83\xC4\x08\x89\x06\x8B\xC6") - 0x42);
    keyValuesFindKey = reinterpret_cast<decltype(keyValuesFindKey)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x81\xEC????\x56\x8B\x75\x08\x57\x8B\xF9\x85\xF6\x0F\x84????\x80\x3E\x00\x0F\x84????"));
    attributeHookValue = relativeToAbsolute<decltype(attributeHookValue)>(findPattern(CLIENT_DLL, "\xE8????\xDA\x4D\xFC") + 1);
   
    boneSetupInitPose = relativeToAbsolute<decltype(boneSetupInitPose)>(findPattern(CLIENT_DLL, "\xE8????\xD9\x47\x1C") + 1);
    boneSetupAccumulatePose = relativeToAbsolute<decltype(boneSetupAccumulatePose)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x07\x33\xFF") + 1);
    boneSetupCalcAutoplaySequences = relativeToAbsolute<decltype(boneSetupCalcAutoplaySequences)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x03\x83\xB8?????\x74\x1E") + 1);
    boneSetupCalcBoneAdj = relativeToAbsolute<decltype(boneSetupCalcBoneAdj)>(findPattern(CLIENT_DLL, "\xE8????\xFF\x75\x18\xD9\x45\x14\x8B\x07") + 1);

    calcAbsoluteVelocity = relativeToAbsolute<decltype(calcAbsoluteVelocity)>(findPattern(CLIENT_DLL, "\xE8????\xD9\xE8\x8D\x45\xEC") + 1);
    conColorMsg = reinterpret_cast<decltype(conColorMsg)>(GetProcAddress(GetModuleHandleA(TIER0_DLL), "?ConColorMsg@@YAXABVColor@@PBDZZ"));
    conMsg = reinterpret_cast<decltype(conMsg)>(GetProcAddress(GetModuleHandleA(TIER0_DLL), "?ConMsg@@YAXPBDZZ"));
    cullBox = relativeToAbsolute<decltype(cullBox)>(findPattern(ENGINE_DLL, "\xE8????\x33\xC9\x83\xC4\x10\x84\xC0") + 1);
    getAmmoCount = reinterpret_cast<decltype(getAmmoCount)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\x75\x08\x57\x8B\xF9\x83\xFE\xFF\x75\x08\x5F\x33\xC0\x5E\x5D\xC2\x04\x00"));
    getItemDefinition = relativeToAbsolute<decltype(getItemDefinition)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB6\x40\x15") + 1);
    getNextThinkTick = reinterpret_cast<decltype(getNextThinkTick)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x45\x08\x56\x8B\xF1\x85\xC0\x75\x13"));
    getWeaponData = reinterpret_cast<decltype(getWeaponData)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x66\x8B??\x66\x3B\x05????\x73"));
    generatePerspectiveFrustum = relativeToAbsolute<decltype(generatePerspectiveFrustum)>(findPattern(ENGINE_DLL, "\xE8????\x8B\x45\x08\x83\xC4\x24\xF3\x0F\x10\x05????") + 1);
    
    IKContextConstruct = relativeToAbsolute<decltype(IKContextConstruct)>(findPattern(CLIENT_DLL, "\xE8????\x89\x83????\xEB\x38") + 1);
    IKContextDeconstruct = relativeToAbsolute<decltype(IKContextDeconstruct)>(findPattern(CLIENT_DLL, "\xE8????\x68????\xFF\x75\xE4") + 1);
    IKContextInit = relativeToAbsolute<decltype(IKContextInit)>(findPattern(CLIENT_DLL, "\xE8????\xFF\x73\x0C\x8D\x8D????") + 1);
    IKContextUpdateTargets = relativeToAbsolute<decltype(IKContextUpdateTargets)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x07\xD9\x45\x14") + 1);
    IKContextSolveDependencies = relativeToAbsolute<decltype(IKContextSolveDependencies)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x07\x8D\x4D\xD0\x51") + 1);

    passServerEntityFilter = reinterpret_cast<decltype(passServerEntityFilter)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\x75?\x85\xF6\x75?\xB0?"));
    physicsRunThink = reinterpret_cast<decltype(physicsRunThink)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x53\x8B\xD9\x56\x57\x8B\x83????\xC1"));
    randomSeed = reinterpret_cast<decltype(randomSeed)>(GetProcAddress(GetModuleHandleA(VSTDLIB_DLL), "RandomSeed"));
    randomInt = reinterpret_cast<decltype(randomInt)>(GetProcAddress(GetModuleHandleA(VSTDLIB_DLL), "RandomInt"));
    randomFloat = reinterpret_cast<decltype(randomFloat)>(GetProcAddress(GetModuleHandleA(VSTDLIB_DLL), "RandomFloat"));
    setAbsOrigin = reinterpret_cast<decltype(setAbsOrigin)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x57\x8B\xF1\xE8????\x8B\x7D\x08\xF3\x0F\x10\x07"));
    setAbsAngle = reinterpret_cast<decltype(setAbsAngle)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x60\x56\x57\x8B\xF1"));
    setCollisionBounds = relativeToAbsolute<decltype(setCollisionBounds)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB6\x4E\x40") + 1);
    setNextThink = reinterpret_cast<decltype(setNextThink)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\xF3\x0F\x10\x45?\x0F\x2E\x05????\x53"));
    shouldCollide = reinterpret_cast<decltype(shouldCollide)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x55?\x56\x8B\x75?\x3B\xF2"));
    simulatePlayerSimulatedEntities = relativeToAbsolute<decltype(simulatePlayerSimulatedEntities)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x06\x8B\xCE\x5E\x5B") + 1);
    seqdesc = relativeToAbsolute<decltype(seqdesc)>(findPattern(CLIENT_DLL, "\xE8????\x39\x78\x10") + 1);
    standardFilterRules = reinterpret_cast<decltype(standardFilterRules)>(findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x4D?\x56\x8B\x01\xFF\x50?\x8B\xF0\x85\xF6\x75?\xB0?\x5E\x5D\xC3"));

    predictionRandomSeed = *reinterpret_cast<int**>(findPattern(CLIENT_DLL, "\xC7\x05????????\x5D\xC3\x8B\x40\x34") + 2);

    addToCritBucket = findPattern(CLIENT_DLL, "\x55\x8B\xEC\xA1????\xF3\x0F\x10\x81????\xF3\x0F\x10\x48?\x0F\x2F\xC8\x76\x1D\xF3\x0F\x58\x45?\x0F\x2F\xC8\xF3\x0F\x11\x81????\x77\x03\x0F\x28\xC1\xF3\x0F\x11\x81????\x5D\xC2\x04\x00");
    baseInterpolatePart1 = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x53\x8B\x5D\x18\x56\x8B\xF1");
    calcIsAttackCritical = findPattern(CLIENT_DLL, "\x53\x57\x6A?\x68????\x68????\x6A?\x8B\xF9\xE8????\x50\xE8????\x8B\xD8\x83\xC4?\x85\xDB\x0F\x84");
    calculateChargeCap = relativeToAbsolute<decltype(calculateChargeCap)>(findPattern(CLIENT_DLL, "\xE8????\xF3\x0F\x10\x1D????\xD9\x55\x08") + 1);
    calcViewModelView = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x70\x8B\x55\x0C\x53\x8B\x5D\x08\x89\x4D\xFC\x8B\x02\x89\x45\xE8\x8B\x42\x04\x89\x45\xEC\x8B\x42\x08\x89\x45\xF0\x56\x57");
    canFireRandomCriticalShot = findPattern(CLIENT_DLL, "\x55\x8B\xEC\xF3\x0F\x10\x4D?\xF3\x0F\x58\x0D");
    checkForSequenceChange = relativeToAbsolute<decltype(checkForSequenceChange)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x87????\x83\xEC\x0C") + 1);
    clLoadWhitelist = relativeToAbsolute<decltype(clLoadWhitelist)>(findPattern(ENGINE_DLL, "\xE8????\x83\xC4\x08\x8B\xF0\x56") + 1);
    clMove = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x83\xEC?\x83\x3D?????\x0F\x8C????\xE8");
    clSendMove = findPattern(ENGINE_DLL, "\x55\x8B\xEC\x81\xEC????\xA1????\x8D");
    customTextureOnItemProxyOnBindInternal = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x57\x8B\xF9\x8B\x4F\x04\x85\xC9\x0F\x84????");
    doEnginePostProcessing = relativeToAbsolute<decltype(doEnginePostProcessing)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x75\xF4\x83\xC4\x18") + 1);
    estimateAbsVelocity = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC?\x56\x8B\xF1\xE8????\x3B\xF0\x75?\x8B\xCE\xE8????\x8B\x45?\xD9\x86????\xD9\x18\xD9\x86????\xD9\x58?\xD9\x86????\xD9\x58?\x5E\x8B\xE5\x5D\xC2");
    enableWorldFog = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x0D????\x83\xEC\x0C\x8B\x01\x53\x56\xFF\x90????\x8B\xF0\x85\xF6\x74\x07\x8B\x06\x8B\xCE\xFF\x50\x08");
    fireBullet = relativeToAbsolute<decltype(fireBullet)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x45\x20\x47") + 1);
    frameAdvance = relativeToAbsolute<decltype(frameAdvance)>(findPattern(CLIENT_DLL, "\xE8????\x80\xBF?????\xD9\x55\x08") + 1);
    getTraceType = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x53\x56\x57\x8B\xF9\xE8????\x6A\x00");
    interpolateServerEntities = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x30\x8B\x0D????\x53");
    interpolateViewModel = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x08\x53\x56\x8B\xF1\x57\x80\xBE?????");
    isAllowedToWithdrawFromCritBucket = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x56\x8B\xF1\x0F\xB7\x86????\xFF\x86????\x50\xE8????\x83\xC4\x04\x80\xB8?????\x74\x0A\xF3\x0F\x10\x15");
    newMatchFoundDashboardStateOnUpdate = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x83\xEC\x0C\x56\x8B\xF1\xE8????\x8B\x86????");
    tfPlayerInventoryGetMaxItemCount = findPattern(CLIENT_DLL, "\x8B\x49\x68\x56");
    randomSeedReturnAddress1 = findPattern(CLIENT_DLL, "\x83\xC4?\x0F\x57?\x80\x7D\xFF");
    randomSeedReturnAddress2 = findPattern(CLIENT_DLL, "\x83\xC4?\x68????\x6A?\xFF\x15????\xF3\x0F???\x83\xC4?\x0F\x28");
    randomSeedReturnAddress3 = findPattern(CLIENT_DLL, "\x83\xC4?\x68????\x6A?\xFF\x15????\xF3\x0F???\x83\xC4?\xF3\x0F");
    runSimulation = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x51\x53\x56\x8B\x75?\x57\xFF\x75?\x8B\xF9\x8D\x8E");
    sendDatagram = findPattern(ENGINE_DLL, "\x55\x8B\xEC\xB8????\xE8????\xA1????\x53\x56\x8B\xD9");
    updateClientSideAnimation = findPattern(CLIENT_DLL, "\x56\x8B\xF1\x80\xBE?????\x74\x27");
    updateTFAnimState = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x81\xEC????\x53\x57\x8B\xF9\x8B\x9F????");


    localPlayer.init(*reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\xA1????\x33\xC9\x83\xC4\x04")+1));
}