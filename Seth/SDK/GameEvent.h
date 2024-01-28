#pragma once

#include "VirtualMethod.h"

class GameEvent 
{
public:
    VIRTUAL_METHOD(const char*, getName, 1, (), (this))
    VIRTUAL_METHOD(int, getBool, 5, (const char* keyName, bool defaultValue = false), (this, keyName, defaultValue))
    VIRTUAL_METHOD(int, getInt, 6, (const char* keyName, int defaultValue = 0), (this, keyName, defaultValue))
    VIRTUAL_METHOD(float, getFloat, 7, (const char* keyName, float defaultValue = 0.0f), (this, keyName, defaultValue))
    VIRTUAL_METHOD(const char*, getString, 8, (const char* keyName, const char* defaultValue = ""), (this, keyName, defaultValue))
    VIRTUAL_METHOD(void, setInt, 10, (const char* keyName, int value), (this, keyName, value))
    VIRTUAL_METHOD(void, setString, 12, (const char* keyName, const char* value), (this, keyName, value))
};