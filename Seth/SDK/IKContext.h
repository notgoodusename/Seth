#pragma once

#include "CStudioHdr.h"
#include "Entity.h"
#include "matrix3x4.h"

class IKContext
{
private:
	PAD(4192)
public:
	void* operator new(size_t size) noexcept;
	void operator delete(void* ptr) noexcept;

	static void construct(IKContext* ik) noexcept;
	
	void init(const CStudioHdr* hdr, const Vector& localAngles, const Vector& localOrigin, float currentTime, int frameCount, int boneMask) noexcept;
	void addDependencies(StudioSeqdesc& seqdesc, int sequence, float cycle, const float poseParameters[], float weight = 1.0f) noexcept;
	
	void updateTargets(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept;
	void solveDependencies(Vector* pos, Quaternion* q, matrix3x4* boneCache, void* computed) noexcept;

	void addSequenceLocks(StudioSeqdesc& seqdesc, Vector pos[], Quaternion q[]) noexcept;
	void solveSequenceLocks(StudioSeqdesc& seqdesc, Vector pos[], Quaternion q[]) noexcept;
};
