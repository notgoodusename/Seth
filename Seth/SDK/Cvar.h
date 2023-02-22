#pragma once

#include "ConVar.h"
#include "VirtualMethod.h"

class ConVar;

class Cvar {
public:
    VIRTUAL_METHOD(ConVar*, findVar, 13, (const char* name), (this, name))
    VIRTUAL_METHOD(ConCommandBase*, getCommands, 17, (), (this))
};