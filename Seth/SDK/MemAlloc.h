#pragma once

#include "VirtualMethod.h"

class MemAlloc
{
public:
    VIRTUAL_METHOD(void*, alloc, 1, (size_t size), (this, size))
    VIRTUAL_METHOD(void*, realloc, 3, (size_t size, void* memory), (this, size, memory))
    VIRTUAL_METHOD(void*, free, 5, (void* memory), (this, memory))
};