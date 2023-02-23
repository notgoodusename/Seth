#pragma once

#include <string>

#include "VirtualMethod.h"

class Localize {
public:
    VIRTUAL_METHOD(const wchar_t*, find, 2, (const char* tokenName), (this, tokenName))
    VIRTUAL_METHOD(const char*, findAsUTF8, 15, (const char* tokenName), (this, tokenName))
};
