#include "IKContext.h"
#include "MemAlloc.h"
#include "Quaternion.h"

void* IKContext::operator new(size_t size) noexcept
{
	IKContext* ptr = reinterpret_cast<IKContext*>(memory->memAlloc->alloc(size));
	construct(ptr);

	return ptr;
}

void IKContext::operator delete(void* ptr) noexcept
{
	memory->IKContextDeconstruct(ptr);
	memory->memAlloc->free(ptr);
}

void IKContext::construct(IKContext* ik) noexcept
{
    memory->IKContextConstruct(ik);
}

void IKContext::init(const CStudioHdr* hdr, const Vector& localAngles, const Vector& localOrigin, float currentTime, int frameCount, int boneMask) noexcept
{
    memory->IKContextInit(this, hdr, localAngles, localOrigin, currentTime, frameCount, boneMask);
}

void IKContext::updateTargets(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept
{
    memory->IKContextUpdateTargets(this, pos, q, boneCache, computed);
}

void IKContext::solveDependencies(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept
{
    memory->IKContextSolveDependencies(this, pos, q, boneCache, computed);
}