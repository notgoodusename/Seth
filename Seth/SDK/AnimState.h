#pragma once
#include "Activity.h"
#include "Entity.h"
#include "Pad.h"
#include "UtlVector.h"
#include "Vector.h"

#define ANIM_LAYER_ACTIVE		0x0001
#define ANIM_LAYER_AUTOKILL		0x0002
#define ANIM_LAYER_KILLME		0x0004
#define ANIM_LAYER_DONTRESTORE	0x0008
#define ANIM_LAYER_CHECKACCESS	0x0010
#define ANIM_LAYER_DYING		0x0020

struct AnimationLayer
{
public:
	int	flags;
	bool sequenceFinished;
	bool kooping;
	int	sequence;
	float cycle;
	float prevCycle;
	float weight;
	float playbackRate;
	float blendIn;
	float blendOut;
	float killRate;
	float killDelay;
	float layerAnimtime;
	float layerFadeOuttime;

	Activity activity;

	int	priority;
	int order;

	bool isActive() noexcept { return ((flags & ANIM_LAYER_ACTIVE) != 0); }
	bool isAutokill() noexcept { return ((flags & ANIM_LAYER_AUTOKILL) != 0); }
	bool isKillMe() noexcept { return ((flags & ANIM_LAYER_KILLME) != 0); }
	bool isAutoramp() noexcept { return (blendIn != 0.0 || blendOut != 0.0); }
	void killMe() noexcept { flags |= ANIM_LAYER_KILLME; }
	void dying() noexcept { flags |= ANIM_LAYER_DYING; }
	bool isDying() noexcept { return ((flags & ANIM_LAYER_DYING) != 0); }
	void dead() noexcept { flags &= ~ANIM_LAYER_DYING; }

	float lastEventCheck;
	float lastAccess;

	// Network state changes get forwarded here.
	void* ownerEntity;
};

struct GestureSlot
{
	int gestureSlot;
	Activity activity;
	bool autoKill;
	bool active;
	AnimationLayer* animLayer;
};

struct MultiPlayerPoseData
{
	int moveX;
	int moveY;
	int aimYaw;
	int aimPitch;
	int bodyHeight;
	int moveYaw;
	int moveScale;

	float estimateYaw;
	float lastAimTurnTime;

	void init() noexcept
	{
		moveX = 0;
		moveY = 0;
		aimYaw = 0;
		aimPitch = 0;
		bodyHeight = 0;
		moveYaw = 0;
		moveScale = 0;
		estimateYaw = 0.0f;
		lastAimTurnTime = 0.0f;
	}
};

struct DebugPlayerAnimData
{
	float velocity;
	float aimPitch;
	float aimYaw;
	float bodyHeight;
	float moveYawX;
	float moveYawY;

	void init() noexcept
	{
		velocity = 0.0f;
		aimPitch = 0.0f;
		aimYaw = 0.0f;
		bodyHeight = 0.0f;
		moveYawX = 0.0f;
		moveYawY = 0.0f;
	}
};

struct MultiPlayerMovementData
{
	// Set speeds to -1 if they are not used.
	float walkSpeed;
	float runSpeed;
	float sprintSpeed;
	float bodyYawRate;
};

enum LegAnimType
{
	LEGANIM_9WAY,	// Legs use a 9-way blend, with "move_x" and "move_y" pose parameters.
	LEGANIM_8WAY,	// Legs use an 8-way blend with "move_yaw" pose param.
	LEGANIM_GOLDSRC	// Legs always point in the direction he's running and the torso rotates.
};

struct MultiPlayerAnimState
{
public:
	PAD(4)
	bool forceAimYaw;
	PAD(3)
	UtlVector<GestureSlot> gestureSlots;
	void* player;
	Vector angRender;

	bool poseParameterInit;
	PAD(3)

	MultiPlayerPoseData	poseParameterData;
	DebugPlayerAnimData debugAnimData;

	bool currentFeetYawInitialized;
	float lastAnimationStateClearTime;

	float eyeYaw;
	float eyePitch;
	float goalFeetYaw;
	float currentFeetYaw;
	float lastAimTurnTime;

	MultiPlayerMovementData movementData;

	bool jumping;
	float jumpStartTime;
	bool firstJumpFrame;
	bool inSwim;
	bool firstSwimFrame;
	bool dying;
	bool firstDyingFrame;

	int currentMainSequenceActivity;
	int specificMainSequence;

	void* activeWeapon;

	float lastGroundSpeedUpdateTime;
	PAD(48) //CInterpolatedVar<float> m_iv_flMaxGroundSpeed;
	float maxGroundSpeed;

	int movementSequence;
	int legAnimType;
}; //244

struct TFPlayerAnimState : public MultiPlayerAnimState
{
public:
	void* TFPlayer;
	bool inAirWalk;
	float holdDeployedPoseUntilTime;
	float tauntMoveX;
	float tauntMoveY;
	float vehicleLeanVel;
	float vehicleLeanPos;
	Vector vecSmoothedUp;
};