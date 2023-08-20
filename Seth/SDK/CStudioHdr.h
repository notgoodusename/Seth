#pragma once

#include "ModelInfo.h"
#include "UtlVector.h"

struct StudioSeqdesc
{
public:
	inline char* const label() const noexcept { return ((char*)this) + labelIndex; }
	inline char* const activityName() const noexcept { return ((char*)this) + activityNameIndex; }

	void* basePtr;

	int labelIndex;

	int activityNameIndex;

	int flags;

	int activity;
	int actWeight;

	int numEvents;
	int eventIndex;

	Vector bbMin;
	Vector bbMax;

	int numBlends;

	int animIndexIndex;

	int movementIndex;
	int groupSize[2];
	int paramIndex[2];
	float paramStart[2];
	float paramEnd[2];
	int paramParent;

	float fadeInTime;
	float fadeOutTime;

	int localEntryNode;
	int localExitNode;
	int nodeFlags;

	float entryPhase;
	float exitPhase;

	float lastFrame;

	int nextSeq;
	int pose;

	int numIkRules;

	int numAutoLayers;
	int autoLayerIndex;

	int weightListIndex;

	int poseKeyIndex;

	int numIkLocks;
	int ikLockIndex;


	int keyValueIndex;
	int keyValueSize;

	int cyclePoseIndex;

	int activityModifierIndex;
	int numActivityModifiers;
	PAD(20)
};

struct CStudioHdr
{
public:
	StudioHdr* hdr;
	void* virtualModel;
	UtlVector<const StudioHdr*> hdrCache;
	int	numFrameUnlockCounter;
	int* frameUnlockCounter;
	PAD(8);
	UtlVector<int> boneFlags;
	UtlVector<int> boneParent;
	void* activityToSequence;

	StudioSeqdesc seqdesc(int sequence) noexcept
	{
		return *reinterpret_cast<StudioSeqdesc*>(memory->seqdesc(this, sequence));
	}
};