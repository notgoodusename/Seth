#pragma once

#include "Pad.h"
#include "Vector.h"
#include "VirtualMethod.h"

struct UserCmd;

class Input {
public:
    VIRTUAL_METHOD(UserCmd*, getUserCmd, 8, (int slot, int sequenceNumber), (this, slot, sequenceNumber))
    VIRTUAL_METHOD(int, isCameraInThirdPerson, 31, (), (this))
};