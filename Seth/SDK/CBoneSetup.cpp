#include "CBoneSetup.h"
#include "IKContext.h"

CBoneSetup::CBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters) noexcept
    : studioHdr(studioHdr)
    , boneMask(boneMask)
    , poseParameter(poseParameters)
    , poseDebugger(nullptr)
{
}


IBoneSetup::IBoneSetup(const CStudioHdr* studioHdr, int boneMask, float* poseParameters) noexcept
{
    boneSetup = new CBoneSetup(studioHdr, boneMask, poseParameters);
}

IBoneSetup::~IBoneSetup() noexcept
{
    if (boneSetup)
        delete boneSetup;
}

void IBoneSetup::initPose(Vector pos[], Quaternion q[]) noexcept
{
   memory->boneSetupInitPose(this, pos, q);
}

void IBoneSetup::accumulatePose(Vector pos[], Quaternion q[], int sequence, float cycle, float weight, float time, IKContext* ikContext) noexcept
{
	memory->boneSetupAccumulatePose(this, pos, q, sequence, cycle, weight, time, ikContext);
}

void IBoneSetup::calcAutoplaySequences(Vector pos[], Quaternion q[], float realTime, void* IKContext) noexcept
{
    memory->boneSetupCalcAutoplaySequences(this, pos, q, realTime, IKContext); //TODO Fix
}

void IBoneSetup::calcBoneAdj(Vector pos[], Quaternion q[], const float controllers[]) noexcept
{
    memory->boneSetupCalcBoneAdj(this, pos, q, controllers);
}