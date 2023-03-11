#pragma once

#include "Pad.h"
#include "VirtualMethod.h"

struct SurfacePhysicsParameters
{
	// vphysics physical properties
	float			friction;
	float			elasticity;				// collision elasticity - used to compute coefficient of restitution
	float			density;				// physical density (in kg / m^3)
	float			thickness;				// material thickness if not solid (sheet materials) in inches
	float			dampening;
};

struct SurfaceAudioParameters
{
	// sounds / audio data
	float			reflectivity;		// like elasticity, but how much sound should be reflected by this surface
	float			hardnessFactor;	// like elasticity, but only affects impact sound choices
	float			roughnessFactor;	// like friction, but only affects scrape sound choices

// audio thresholds
	float			roughThreshold;	// surface roughness > this causes "rough" scrapes, < this causes "smooth" scrapes
	float			hardThreshold;	// surface hardness > this causes "hard" impacts, < this causes "soft" impacts
	float			hardVelocityThreshold;	// collision velocity > this causes "hard" impacts, < this causes "soft" impacts
									// NOTE: Hard impacts must meet both hardnessFactor AND velocity thresholds
};

struct SurfaceSoundNames
{
	unsigned short	stepleft;
	unsigned short	stepright;

	unsigned short	impactSoft;
	unsigned short	impactHard;

	unsigned short	scrapeSmooth;
	unsigned short	scrapeRough;

	unsigned short	bulletImpact;
	unsigned short	rolling;

	unsigned short	breakSound;
	unsigned short	strainSound;
};

struct SurfaceSoundHandles
{
	short	stepleft;
	short	stepright;

	short	impactSoft;
	short	impactHard;

	short	scrapeSmooth;
	short	scrapeRough;

	short	bulletImpact;
	short	rolling;

	short	breakSound;
	short	strainSound;
};

struct SurfaceGameProps
{
	// game movement data
	float			maxSpeedFactor;			// Modulates player max speed when walking on this surface
	float			jumpFactor;				// Indicates how much higher the player should jump when on the surface
	// Game-specific data
	unsigned short	material;
	// Indicates whether or not the player is on a ladder.
	unsigned char	climbable;
	unsigned char	pad;
};

struct SurfaceData
{
	SurfacePhysicsParameters physics;	// physics parameters
	SurfaceAudioParameters audio;		// audio parameters
	SurfaceSoundNames sounds;		// names of linked sounds
	SurfaceGameProps game;		// Game data / properties

	SurfaceSoundHandles soundhandles;
};

class PhysicsSurfaceProps {
public:
    VIRTUAL_METHOD(void, getPhysicsProperties, 4, (int surfaceDataIndex, float* density, float* thickness, float* friction, float* elasticity), (this, surfaceDataIndex, density, thickness, friction, elasticity))
    VIRTUAL_METHOD(SurfaceData*, getSurfaceData, 5, (int index), (this, index))
};
