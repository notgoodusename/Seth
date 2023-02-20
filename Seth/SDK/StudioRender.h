#pragma once

#include <cstddef>
#include <string_view>

#include "VirtualMethod.h"

enum class OverrideType {
    Normal = 0,
    BuildShadows,
    DepthWrite,
    CustomMaterial, // weapon skins
    SsaoDepthWrite
};