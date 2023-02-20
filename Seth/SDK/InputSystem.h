#pragma once

#include "VirtualMethod.h"

class InputSystem {
public:
	VIRTUAL_METHOD(void, enableInput, 2, (bool enable), (this, enable))
    VIRTUAL_METHOD(void, resetInputState, 25, (), (this))
};