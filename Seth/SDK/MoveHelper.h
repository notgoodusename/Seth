#pragma once

#include "VirtualMethod.h"

class Entity;

class MoveHelper {
public:
    VIRTUAL_METHOD(void, setHost, 0, (Entity* host), (this, host))
};
