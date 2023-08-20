#pragma once

#include "CStudioHdr.h"
#include "Entity.h"
#include "Quaternion.h"
#include "matrix3x4.h"

struct CBoneSetup {
    CBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters) noexcept;
    const CStudioHdr* studioHdr;
    int boneMask;
    float* poseParameter;
    void* poseDebugger;
};

struct IBoneSetup
{
public:
    IBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters = 0) noexcept;
    ~IBoneSetup() noexcept;

    void initPose(Vector pos[], Quaternion q[]) noexcept;
    void accumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float weight, float time, IKContext* IKContext) noexcept;
    void calcAutoplaySequences(Vector pos[], Quaternion q[], float realTime, void* IKContext) noexcept;
    void calcBoneAdj(Vector pos[], Quaternion q[], const float controllers[]) noexcept;
    CBoneSetup* boneSetup;
};