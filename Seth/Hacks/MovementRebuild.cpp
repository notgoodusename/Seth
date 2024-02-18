#include "MovementRebuild.h"
#include "../SDK/PhysicsSurfaceProps.h"

//https://github.com/emilyinure/haven-tf2

MovementRebuild::info mv;
MovementRebuild::Cvars cvars;

Vector MovementRebuild::getPlayerMins() noexcept
{
	return mv.obbMins;
}

Vector MovementRebuild::getPlayerMaxs() noexcept
{
	return mv.obbMaxs;
}

void MovementRebuild::tracePlayerBBox(const Vector& start, const Vector& end, unsigned int mask, int collisionGroup, Trace& pm) noexcept
{
	interfaces->engineTrace->traceRay({ start, end, getPlayerMins(), getPlayerMaxs() }, mask, TraceFilterIgnorePlayers{ mv.player, collisionGroup }, pm);
}

void MovementRebuild::tryTouchGround(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, int collisionGroup, Trace& pm) noexcept
{
	interfaces->engineTrace->traceRay({ start, end, mins, maxs }, mask, TraceFilterIgnorePlayers{ mv.player, collisionGroup }, pm);
}

unsigned int MovementRebuild::playerSolidMask(bool brushOnly) noexcept
{
	return (brushOnly) ? MASK_PLAYERSOLID_BRUSHONLY : MASK_PLAYERSOLID;
}

void MovementRebuild::init() noexcept
{
	cvars.accelerate = interfaces->cvar->findVar("sv_accelerate");
	cvars.airAccelerate = interfaces->cvar->findVar("sv_airaccelerate");
	cvars.bounce = interfaces->cvar->findVar("sv_bounce");
	cvars.friction = interfaces->cvar->findVar("sv_friction");
	cvars.gravity = interfaces->cvar->findVar("sv_gravity");
	cvars.stopSpeed = interfaces->cvar->findVar("sv_stopspeed");
	cvars.maxVelocity = interfaces->cvar->findVar("sv_maxvelocity");
	cvars.optimizedMovement = interfaces->cvar->findVar("sv_optimizedmovement");
	cvars.parachuteMaxSpeedZ = interfaces->cvar->findVar("tf_parachute_maxspeed_z");
}

void MovementRebuild::setEntity(Entity* player) noexcept
{
	mv.player = player;
	mv.groundEntity = player->calculateGroundEntity();
	mv.velocity = player->velocity();
	mv.baseVelocity = Vector{ };
	mv.position = player->origin();
	mv.eyeAngles = player->eyeAngles();
	mv.surfaceFriction = 1.0f;
	mv.waterLevel = player->waterLevel();
	mv.maxSpeed = player->getMaxSpeed();

	mv.obbMins = player->obbMins() * player->modelScale();
	mv.obbMaxs = player->obbMaxs() * player->modelScale();
	//THE BIG ONE
	//This is the most important part of movement rebuild
	//Assigning the correct forwardmove, sidemove and upmove values
	//TODO: improve this
	Vector direction = mv.velocity.toAngle();
	direction.y = mv.eyeAngles.y - direction.y;
	const auto finalDirection = Vector::fromAngle(direction) * mv.velocity.length2D();
	mv.forwardMove = finalDirection.x * (450.0f / mv.maxSpeed);
	mv.sideMove = finalDirection.y * (450.0f / mv.maxSpeed);
	mv.upMove = finalDirection.z;
}

Vector MovementRebuild::runPlayerMove() noexcept
{
	if (!cvars.accelerate || !cvars.airAccelerate || !cvars.bounce || !cvars.friction || !cvars.gravity 
		|| !cvars.stopSpeed || !cvars.maxVelocity || !cvars.optimizedMovement || !cvars.parachuteMaxSpeedZ)
		return Vector{ };
	
	if (!cvars.optimizedMovement->getInt())
		categorizePosition();
	else if (mv.velocity.z > 250.0f)
		setGroundEntity(NULL);

	fullWalkMove();
	return mv.position;
}

void MovementRebuild::waterMove() noexcept
{
	Vector	wishvel;
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	Trace	pm;
	float speed, newspeed, addspeed, accelspeed;
	Vector forward, right, up;

	Vector::fromAngleAll(mv.eyeAngles, &forward, &right, &up);  // Determine movement angles

	//
	// user intentions
	//
	for (int i = 0; i < 3; i++)
	{
		wishvel[i] = forward[i] * mv.forwardMove + right[i] * mv.sideMove;
	}

	// Sinking after no other movement occurs
	if (!mv.forwardMove && !mv.sideMove && !mv.upMove)
	{
		wishvel[2] -= 60;		// drift towards bottom
	}
	else  // Go straight up by upmove amount.
	{
		// exaggerate upward movement along forward as well
		float upwardMovememnt = mv.forwardMove * forward.z * 2;
		upwardMovememnt = std::clamp(upwardMovememnt, 0.f, mv.maxSpeed);
		wishvel[2] += mv.upMove + upwardMovememnt;
	}

	// Copy it over and determine speed
	wishdir = wishvel;
	wishspeed = wishdir.normalizeInPlace();

	// Cap speed.
	if (wishspeed > mv.maxSpeed)
	{
		wishvel *= (mv.maxSpeed / wishspeed);
		wishspeed = mv.maxSpeed;
	}

	// Slow us down a bit.
	wishspeed *= 0.8f;

	// Water friction
	temp = mv.velocity;
	speed = temp.normalizeInPlace();
	if (speed)
	{
		newspeed = speed - memory->globalVars->intervalPerTick * speed * cvars.friction->getFloat() * mv.surfaceFriction;
		if (newspeed < 0.1f)
		{
			newspeed = 0;
		}
		mv.velocity *= (newspeed / speed);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if (wishspeed >= 0.1f)  // old !
	{
		addspeed = wishspeed - newspeed;
		if (addspeed > 0)
		{
			wishvel.normalizeInPlace();
			accelspeed = cvars.accelerate->getFloat() * wishspeed * memory->globalVars->intervalPerTick * mv.surfaceFriction;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}

			for (int i = 0; i < 3; i++)
			{
				float deltaSpeed = accelspeed * wishvel[i];
				mv.velocity[i] += deltaSpeed;
			}
		}
	}

	//mv.velocity += mv.baseVelocity;

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	dest.x = mv.position.x + mv.velocity.x * memory->globalVars->intervalPerTick;
	dest.y = mv.position.y + mv.velocity.y * memory->globalVars->intervalPerTick;
	dest.z = mv.position.z + mv.velocity.z * memory->globalVars->intervalPerTick;

	tracePlayerBBox(mv.position, dest, playerSolidMask(), 8, pm);
	if (pm.fraction == 1.0f)
	{
		start = dest;
		start[2] += 18.f + 1.f;

		tracePlayerBBox(start, dest, playerSolidMask(), 8, pm);

		if (!pm.startSolid && !pm.allSolid)
		{
			// walked up the step, so just keep result and exit
			mv.position = pm.endpos;
			//mv.velocity -= mv.baseVelocity;
			return;
		}

		// Try moving straight along out normal path.
		tryPlayerMove();
	}
	else
	{
		if (!mv.groundEntity)
		{
			tryPlayerMove();
			//mv.velocity -= mv.baseVelocity;
			return;
		}

		stepMove(dest, pm);
	}
	
	//mv.velocity -= mv.baseVelocity;
}

void MovementRebuild::friction() noexcept
{
	if (mv.groundEntity == NULL)
		return;

	float speed, newspeed, control;
	float friction;
	float drop;

	// Calculate speed
	speed = mv.velocity.length();

	// If too slow, return
	if (speed < 0.1f)
		return;

	drop = 0;

	// apply ground friction
	friction = cvars.friction->getFloat() * mv.surfaceFriction;

	control = (speed < cvars.stopSpeed->getFloat()) ? cvars.stopSpeed->getFloat() : speed;

	// Add the amount to the drop amount.
	drop += control * friction * memory->globalVars->intervalPerTick;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if (newspeed != speed)
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		mv.velocity *= newspeed;
	}
}

void MovementRebuild::airAccelerate(Vector& wishdir, float wishspeed, float accel) noexcept
{
	float wishspd = wishspeed;

	// Cap speed
	if (wishspd > 30.0f)
		wishspd = 30.0f;

	// Determine veer amount
	const float currentspeed = mv.velocity.dotProduct(wishdir);

	// See how much to add
	const float addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	float accelspeed = accel * wishspeed * memory->globalVars->intervalPerTick * mv.surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust pmove vel.
	for (int i = 0; i < 3; i++)
	{
		mv.velocity[i] += accelspeed * wishdir[i];
	}
}

void MovementRebuild::airMove() noexcept
{
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	Vector::fromAngleAll(mv.eyeAngles, &forward, &right, &up);  // Determine movement angles /from velocity

	// Copy movement amounts
	fmove = mv.forwardMove;
	smove = mv.sideMove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2] = 0;
	forward.normalizeInPlace();  // Normalize remainder of vectors
	right.normalizeInPlace();    // 

	for (int i = 0; i < 2; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i] * fmove + right[i] * smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	wishdir = wishvel;   // Determine maginitude of speed of move
	wishspeed = wishdir.normalizeInPlace();

	airAccelerate(wishdir, wishspeed, cvars.airAccelerate->getFloat());

	// Add in any base velocity to the current velocity.
	//mv.velocity += mv.baseVelocity;

	tryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	//mv.velocity -= mv.baseVelocity;
}

void MovementRebuild::accelerate(Vector& wishdir, float wishspeed, float accel) noexcept
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed = mv.velocity.dotProduct(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * memory->globalVars->intervalPerTick * wishspeed * mv.surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i = 0; i < 3; i++)
	{
		mv.velocity[i] += accelspeed * wishdir[i];
	}
}

void MovementRebuild::walkMove() noexcept
{
	Vector wishvel;
	float spd;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	Trace pm;
	Vector forward, right, up;

	Vector::fromAngleAll(mv.eyeAngles, &forward, &right, &up);  // Determine movement angles

	auto oldground = mv.groundEntity;

	// Copy movement amounts
	fmove = mv.forwardMove;
	smove = mv.sideMove;

	forward[2] = 0;
	right[2] = 0;

	forward.normalizeInPlace();  // Normalize remainder of vectors.
	right.normalizeInPlace();    // 

	for (int i = 0; i < 2; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	wishvel[2] = 0.0f;             // Zero out z part of velocity

	wishdir = wishvel;   // Determine maginitude of speed of move
	wishspeed = wishdir.normalizeInPlace();

	//
	// Clamp to server defined max speed
	//

	wishspeed = std::clamp(wishspeed, 0.0f, mv.maxSpeed);

	// Set pmove velocity
	mv.velocity[2] = 0.0f;
	accelerate(wishdir, wishspeed, cvars.accelerate->getFloat());
	mv.velocity[2] = 0.0f;

	// Add in any base velocity to the current velocity.
	//mv.velocity += mv.baseVelocity;

	spd = mv.velocity.length();

	if (spd < 1.0f)
	{
		mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		//mv.velocity -= mv.baseVelocity;
		return;
	}

	// first try just moving to the destination	
	dest[0] = mv.position[0] + mv.velocity[0] * memory->globalVars->intervalPerTick;
	dest[1] = mv.position[1] + mv.velocity[1] * memory->globalVars->intervalPerTick;
	dest[2] = mv.position[2];

	// first try moving directly to the next spot
	tracePlayerBBox(mv.position, dest, playerSolidMask(), 8, pm);

	if (pm.fraction == 1)
	{
		mv.position = pm.endpos;
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		//mv.velocity -= mv.baseVelocity;
		stayOnGround();
		return;
	}

	// Don't walk up stairs if not on ground.
	if (oldground == NULL && mv.waterLevel == 0)
	{
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		//mv.velocity -= mv.baseVelocity;
		return;
	}

	stepMove(dest, pm);

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	//mv.velocity -= mv.baseVelocity;

	stayOnGround();
}

void MovementRebuild::stayOnGround() noexcept
{
	Trace trace;
	Vector start = mv.position;
	Vector end = mv.position;
	start.z += 2;
	end.z -= 18.0f;

	// See how far up we can go without getting stuck

	tracePlayerBBox(mv.position, start, playerSolidMask(), 8, trace);
	start = trace.endpos;

	// using trace.startsolid is unreliable here, it doesn't get set when
	// tracing bounding box vs. terrain

	// Now trace down from a known safe position
	tracePlayerBBox(start, end, playerSolidMask(), 8, trace);
	if (trace.fraction > 0.0f &&			// must go somewhere
		trace.fraction < 1.0f &&			// must hit something
		!trace.startSolid &&				// can't be embedded in a solid
		trace.plane.normal[2] >= 0.7)		// can't hit a steep slope that we can't stand on anyway
	{
		float delta = fabs(mv.position.z - trace.endpos.z);
		//This is incredibly hacky. The real problem is that trace returning that strange value we can't network over.
		if (delta > 0.5f * ((1.0f/ (1<<5))))
		{
			mv.position = trace.endpos;
		}
	}
}

void MovementRebuild::fullWalkMove() noexcept
{
	if (!checkWater())
	{
		if (mv.player->isParachuting() && mv.velocity[2] < 0)
			mv.velocity[2] = max(mv.velocity[2], cvars.parachuteMaxSpeedZ->getFloat());

		startGravity();
	}

	if (mv.waterLevel >= 2 
		|| mv.player->isAGhost() || mv.player->isSwimmingNoEffects())
	{
		// Perform regular water movement
		waterMove();

		// Redetermine position vars
		categorizePosition();

		// If we are on ground, no downward velocity.
		if (mv.groundEntity != NULL)
			mv.velocity[2] = 0;
	}
	else
	{
		if (mv.groundEntity != NULL)
		{
			mv.velocity[2] = 0.0;
			friction();
		}

		// Make sure velocity is valid.
		checkVelocity();

		if (mv.groundEntity != NULL)
			walkMove();
		else
			airMove();

		// Redetermine position vars
		categorizePosition();

		checkVelocity();

		if (!checkWater())
			finishGravity();

		// If we are on ground, no downward velocity.
		if (mv.groundEntity != NULL)
			mv.velocity[2] = 0;
	}
}

int MovementRebuild::clipVelocity(Vector& in, Vector& normal, Vector& out, float overbounce) noexcept
{
	float	backoff;
	float	change;
	float angle;
	int		blocked;

	angle = normal[2];

	blocked = 0x00;         // Assume unblocked.
	if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;	// 
	if (!angle)				// If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;	// 


	// Determine how far along plane to slide based on incoming direction.
	backoff = in.dotProduct(normal) * overbounce;

	for (int i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}

	// iterate once to make sure we aren't still moving through the plane
	float adjust = out.dotProduct(normal);
	if (adjust < 0.0f)
		out -= (normal * adjust);

	return blocked;
}

int MovementRebuild::stuck() noexcept
{
	Trace trace;

	const int hitent = testPlayerPosition(mv.position, 8, trace);
	if (hitent == 0xFFFFFFFF)
		return 0;
	return 1;
}

bool MovementRebuild::checkWater() noexcept
{
	Vector	point;
	int		cont;

	Vector playerMins = getPlayerMins();
	Vector playerMaxs = getPlayerMaxs();

	// Pick a spot just above the players feet.
	point[0] = mv.position[0] + (playerMins[0] + playerMaxs[0]) * 0.5f;
	point[1] = mv.position[1] + (playerMins[1] + playerMaxs[1]) * 0.5f;
	point[2] = mv.position[2] + playerMins[2] + 1;

	// Assume that we are not in water at all.
	mv.waterLevel = 0;

	// Grab point contents.
	cont = interfaces->engineTrace->getPointContents(point, NULL);

	// Are we under water? (not solid and not empty?)
	if (cont & MASK_WATER)
	{
		// We are at least at level one
		mv.waterLevel = 1;

		// Now check a point that is at the player hull midpoint.
		point[2] = mv.position[2] + (playerMins[2] + playerMaxs[2]) * 0.5f;
		cont = interfaces->engineTrace->getPointContents(point, NULL);
		// If that point is also under water...
		if (cont & MASK_WATER)
		{
			// Set a higher water level.
			mv.waterLevel = 2;

			// Now check the eye position.  (view_ofs is relative to the origin)
			point[2] = mv.position[2] + mv.player->viewOffset()[2];
			cont = interfaces->engineTrace->getPointContents(point, NULL);
			if (cont & MASK_WATER)
				mv.waterLevel = 3;
		}
	}

	return mv.waterLevel > 1;
}

void MovementRebuild::startGravity() noexcept
{
	const float gravity = 1.0f;// (mv.player->conditionEx2() & TFCondEx2_Parachute) ? 0.224f : 1.0f;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv.velocity[2] -= (gravity * cvars.gravity->getFloat() * 0.5f * memory->globalVars->intervalPerTick);
	mv.velocity[2] += mv.baseVelocity[2] * memory->globalVars->intervalPerTick;

	mv.baseVelocity[2] = 0.0f;
	
	checkVelocity();
}

void MovementRebuild::finishGravity() noexcept
{
	const float gravity = 1.0f;//(mv.player->conditionEx2() & TFCondEx2_Parachute) ? 0.224f : 1.0f;

	// Get the correct velocity for the end of the dt 
	mv.velocity[2] -= (gravity * cvars.gravity->getFloat() * 0.5f * memory->globalVars->intervalPerTick);

	checkVelocity();
}

int MovementRebuild::tryPlayerMove(Vector* firstDest, Trace* firstTrace) noexcept
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[5];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	Trace	pm;
	Vector		end;
	float		timeLeft, allFraction;
	int			blocked;

	numbumps = 4;           // Bump up to four times

	blocked = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	original_velocity = mv.velocity;
	primal_velocity = mv.velocity;

	allFraction = 0;
	timeLeft = memory->globalVars->intervalPerTick;   // Total time for this movement operation.

	new_velocity = Vector{ 0.0f, 0.0f, 0.0f };

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (mv.velocity.length() == 0.0)
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		end.x = mv.position.x + timeLeft * mv.velocity.x;
		end.y = mv.position.y + timeLeft * mv.velocity.y;
		end.z = mv.position.z + timeLeft * mv.velocity.z;

		if (firstDest && end == *firstDest)
			pm = *firstTrace;
		else
			tracePlayerBBox(mv.position, end, playerSolidMask(), 8, pm);

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allSolid)
		{
			// entity is trapped in another solid
			mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };;
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if (pm.fraction > 0)
		{
			if (numbumps > 0 && pm.fraction == 1)
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				Trace stuck;
				tracePlayerBBox(pm.endpos, pm.endpos, playerSolidMask(), 8, stuck);
				if (stuck.startSolid || stuck.fraction != 1.0f)
				{
					mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
					break;
				}
			}
			// actually covered some distance
			mv.position = pm.endpos;
			mv.velocity = original_velocity;;
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			break;		// moved the entire distance
		}

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		timeLeft -= timeLeft * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= 5)
		{
			// this shouldn't really happen
			//  Stop our movement if so.
			mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
			break;
		}

		// Set up next clipping plane
		planes[numplanes] = pm.plane.normal;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if (numplanes == 1 &&
			mv.groundEntity == NULL)
		{
			for (i = 0; i < numplanes; i++)
			{
				if (planes[i][2] > 0.7)
				{
					// floor or slope
					clipVelocity(original_velocity, planes[i], new_velocity, 1);
					original_velocity = new_velocity;
				}
				else
				{
					clipVelocity(original_velocity, planes[i], new_velocity, 1.0f + cvars.bounce->getFloat() * (1.0f - mv.surfaceFriction));
				}
			}

			mv.velocity = new_velocity;
			original_velocity = new_velocity;
		}
		else
		{
			for (i = 0; i < numplanes; i++)
			{
				clipVelocity(
					original_velocity,
					planes[i],
					mv.velocity,
					1);

				for (j = 0; j < numplanes; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv.velocity.dotProduct(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}

			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
					break;
				}
				dir = planes[0].crossProduct(planes[1]);
				dir.normalizeInPlace();
				d = dir.dotProduct(mv.velocity);
				mv.velocity = dir * d;
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv.velocity.dotProduct(primal_velocity);
			if (d <= 0)
			{
				mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
				break;
			}
		}
	}

	if (allFraction == 0)
	{
		mv.velocity = Vector{ 0.0f, 0.0f, 0.0f };
	}

	return blocked;
}

void MovementRebuild::checkVelocity() noexcept
{
	Vector origin = mv.position;

	for (int i = 0; i < 3; i++)
	{
		if (isnan(mv.velocity[i]))
			mv.velocity[i] = 0.0f;

		if (isnan(origin[i]))
		{
			origin[i] = 0.0f;
			mv.position = origin;
		}

		mv.velocity[i] = std::clamp(mv.velocity[i], -cvars.maxVelocity->getFloat(), cvars.maxVelocity->getFloat());
	}
}

void MovementRebuild::categorizePosition() noexcept
{
	Trace pm;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	mv.surfaceFriction = 1.0f;

	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid	

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	checkWater();

	Vector point = mv.position;
	point.z -= 2.0f;

	Vector bumpOrigin = mv.position;

	// Shooting up really fast.  Definitely not on ground.
	// On ladder moving up, so not on ground either
	// NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY 140.0f

	float zvel = mv.velocity[2];
	bool movingUp = zvel > 0.0f;
	bool movingUpRapidly = zvel > NON_JUMP_VELOCITY;
	float groundEntityVelZ = 0.0f;
	if (movingUpRapidly)
	{
		// Tracker 73219, 75878:  ywb 8/2/07
		// After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
		//  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
		// We need to account for standing on a moving ground object in that case in order to determine if we really 
		//  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
		//  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
		auto ground = mv.groundEntity;
		if (ground)
		{
			groundEntityVelZ = ground->getAbsVelocity().z;
			movingUpRapidly = (zvel - groundEntityVelZ) > NON_JUMP_VELOCITY;
		}
	}

	// Was on ground, but now suddenly am not
	if (movingUpRapidly)
	{
		setGroundEntity(NULL);
	}
	else
	{
		// Try and move down.
		tryTouchGround(bumpOrigin, point, getPlayerMins(), getPlayerMaxs(), MASK_PLAYERSOLID, 8, pm);

		// Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
		if (!pm.entity || pm.plane.normal[2] < 0.7)
		{
			// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
			tryTouchGroundInQuadrants(bumpOrigin, point, MASK_PLAYERSOLID, 8, pm);

			if (!pm.entity || pm.plane.normal[2] < 0.7)
			{
				setGroundEntity(NULL);
				// probably want to add a check for a +z velocity too!
				if (mv.velocity.z > 0.0f)
				{
					mv.surfaceFriction = 0.25f;
				}
			}
			else
			{
				setGroundEntity(&pm);
			}
		}
		else
		{
			setGroundEntity(&pm);  // Otherwise, point to index of ent under us.
		}
	}
}


void MovementRebuild::categorizeGroundSurface(Trace& pm) noexcept
{
	interfaces->physicsSurfaceProps->getPhysicsProperties(pm.surface.surfaceProps, NULL, NULL, &mv.surfaceFriction, NULL);

	// HACKHACK: Scale this to fudge the relationship between vphysics friction values and player friction values.
	// A value of 0.8f feels pretty normal for vphysics, whereas 1.0f is normal for players.
	// This scaling trivially makes them equivalent.  REVISIT if this affects low friction surfaces too much.
	mv.surfaceFriction *= 1.25f;
	if (mv.surfaceFriction > 1.0f)
		mv.surfaceFriction = 1.0f;
}

int MovementRebuild::testPlayerPosition(const Vector& pos, int collisionGroup, Trace& pm) noexcept
{
	interfaces->engineTrace->traceRay({ pos, pos, getPlayerMins(), getPlayerMaxs() }, playerSolidMask(), TraceFilterSimple{ mv.player, collisionGroup }, pm);
	if ((pm.contents & playerSolidMask()) && pm.entity)
		return pm.entity->handle();
	return 0xFFFFFFFF;
}

void MovementRebuild::setGroundEntity(Trace* pm) noexcept
{
	Entity* oldGround = mv.groundEntity;
	Entity* newGround = pm ? pm->entity : NULL;

	mv.groundEntity = newGround;

	if (newGround != oldGround)
	{
		if (oldGround)
		{
			if (oldGround->getClassId() == ClassId::FuncConveyor)
			{
				Vector right{ };
				Vector::fromAngleAll(oldGround->angleRotation(), nullptr, &right, nullptr);
				right *= oldGround->conveyorSpeed();
				mv.baseVelocity -= right;
				mv.baseVelocity.z = right.z;
			}
		}
		if (newGround)
		{
			if (newGround->getClassId() == ClassId::FuncConveyor)
			{
				Vector right{ };
				Vector::fromAngleAll(newGround->angleRotation(), nullptr, &right, nullptr);
				right *= newGround->conveyorSpeed();
				mv.baseVelocity += right;
				mv.baseVelocity.z = right.z;
			}
		}
	}

	// If we are on something...

	if (newGround)
	{
		categorizeGroundSurface(*pm);

		mv.velocity.z = 0.0f;
	}
}

void MovementRebuild::stepMove(Vector& vecDestination, Trace& trace) noexcept
{
	Vector vecEndPos = vecDestination;

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	Vector vecPos = mv.position, vecVel = mv.velocity;

	// Slide move down.
	tryPlayerMove(&vecEndPos, &trace);

	// Down results.
	Vector vecDownPos = mv.position, vecDownVel = mv.velocity;

	// Reset original values.
	mv.position = vecPos;
	mv.velocity = vecVel;

	// Move up a stair height.
	vecEndPos = mv.position;
	vecEndPos.z += 18.f + 0.03125f;

	tracePlayerBBox(mv.position, vecEndPos, playerSolidMask(), 8, trace);
	if (!trace.startSolid && !trace.allSolid)
	{
		mv.position = trace.endpos;
	}

	// Slide move up.
	tryPlayerMove();

	// Move down a stair (attempt to).
	vecEndPos = mv.position;
	vecEndPos.z -= 18.f + 0.03125f;

	tracePlayerBBox(mv.position, vecEndPos, playerSolidMask(), 8, trace);

	// If we are not on the ground any more then use the original movement attempt.
	if (trace.plane.normal[2] < 0.7)
	{
		mv.position = vecDownPos;
		mv.velocity = vecDownVel;
		return;
	}

	// If the trace ended up in empty space, copy the end over to the origin.
	if (!trace.startSolid && !trace.allSolid)
	{
		mv.position = trace.endpos;
	}

	// Copy this origin to up.
	Vector vecUpPos = mv.position;

	// decide which one went farther
	float downDist = (vecDownPos.x - vecPos.x) * (vecDownPos.x - vecPos.x) + (vecDownPos.y - vecPos.y) * (vecDownPos.y - vecPos.y);
	float upDist = (vecUpPos.x - vecPos.x) * (vecUpPos.x - vecPos.x) + (vecUpPos.y - vecPos.y) * (vecUpPos.y - vecPos.y);
	if (downDist > upDist)
	{
		mv.position = vecDownPos;
		mv.velocity = vecDownVel;
	}
	else
	{
		// copy z value from slide move
		mv.velocity.z = vecDownVel.z;
	}
}

void MovementRebuild::tryTouchGroundInQuadrants(const Vector& start, const Vector& end, unsigned int mask, int collisionGroup, Trace& pm) noexcept
{
	Vector mins, maxs;
	Vector minsSrc = getPlayerMins();
	Vector maxsSrc = getPlayerMaxs();

	float fraction = pm.fraction;
	Vector endpos = pm.endpos;

	// Check the -x, -y quadrant
	mins = minsSrc;
	maxs = Vector{ min(0.0f, maxsSrc.x), min(0.0f, maxsSrc.y), maxsSrc.z };
	tryTouchGround(start, end, mins, maxs, mask, collisionGroup, pm);
	if (pm.entity && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, +y quadrant
	mins = Vector{ max(0.0f, minsSrc.x), max(0.0f, minsSrc.y), minsSrc.z };
	maxs = maxsSrc;
	tryTouchGround(start, end, mins, maxs, mask, collisionGroup, pm);
	if (pm.entity && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the -x, +y quadrant
	mins = Vector{ minsSrc.x, max(0.0f, minsSrc.y), minsSrc.z };
	maxs = Vector{ min(0.0f, maxsSrc.x), maxsSrc.y, maxsSrc.z };
	tryTouchGround(start, end, mins, maxs, mask, collisionGroup, pm);
	if (pm.entity && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, -y quadrant
	mins = Vector{ max(0.0f, minsSrc.x), minsSrc.y, minsSrc.z };
	maxs = Vector{ maxsSrc.x, min(0.0f, maxsSrc.y), maxsSrc.z };
	tryTouchGround(start, end, mins, maxs, mask, collisionGroup, pm);
	if (pm.entity && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	pm.fraction = fraction;
	pm.endpos = endpos;
}