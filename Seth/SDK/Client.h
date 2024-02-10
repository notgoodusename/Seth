#pragma once

#include "BitBuffer.h"
#include "VirtualMethod.h"
#include "ViewSetup.h"

struct ClientClass;

class Client {
public:
    VIRTUAL_METHOD(ClientClass*, getAllClasses, 8, (), (this))
    VIRTUAL_METHOD(bool, writeUsercmdDeltaToBuffer, 23, (bufferWrite* buf, int from, int to, bool isnewcommand), (this, buf, from, to, isnewcommand))
    VIRTUAL_METHOD(bool, getPlayerView, 59, (ViewSetup& viewSetup), (this, std::cref(viewSetup)))
};
