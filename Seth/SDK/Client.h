#pragma once

#include "VirtualMethod.h"
#include "BitBuffer.h"

struct ClientClass;

class Client {
public:
    VIRTUAL_METHOD(ClientClass*, getAllClasses, 8, (), (this))
    VIRTUAL_METHOD(bool, writeUsercmdDeltaToBuffer, 23, (int slot, bufferWrite* buf, int from, int to, bool isnewcommand), (this, slot, buf, from, to, isnewcommand))
};
