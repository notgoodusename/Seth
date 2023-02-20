#pragma once

#include "VirtualMethod.h"

class InputSystem {
public:
	virtual void AttachToWindow(void* hWnd) = 0;
	virtual void DetachFromWindow() = 0;
	virtual void enableInput(bool bEnable) = 0;
    VIRTUAL_METHOD(void, resetInputState, 25, (), (this))
};
