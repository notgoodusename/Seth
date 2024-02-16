#pragma once

#include <cstddef>

#include "../Interfaces.h"

#include "ConVar.h"
#include "Cvar.h"
#include "Vector.h"
#include "VirtualMethod.h"

// lower bits are stronger, and will eat weaker brushes completely
#define CONTENTS_EMPTY 0 // No contents

#define CONTENTS_SOLID 0x1  // an eye is never valid in a solid
#define CONTENTS_WINDOW 0x2 // translucent, but not watery (glass)
#define CONTENTS_AUX 0x4
#define CONTENTS_GRATE 0x8 // alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't
#define CONTENTS_SLIME 0x10
#define CONTENTS_WATER 0x20
#define CONTENTS_BLOCKLOS 0x40 // block AI line of sight
#define CONTENTS_OPAQUE 0x80   // things that cannot be seen through (may be non-solid though)
#define LAST_VISIBLE_CONTENTS 0x80

#define ALL_VISIBLE_CONTENTS ( LAST_VISIBLE_CONTENTS | ( LAST_VISIBLE_CONTENTS - 1 ) )

#define CONTENTS_TESTFOGVOLUME 0x100
#define CONTENTS_UNUSED 0x200

// unused
// NOTE: If it's visible, grab from the top + update LAST_VISIBLE_CONTENTS
// if not visible, then grab from the bottom.
#define CONTENTS_UNUSED6 0x400

#define CONTENTS_TEAM1 0x800  // per team contents used to differentiate collisions
#define CONTENTS_TEAM2 0x1000 // between players and objects on different teams

// ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW
#define CONTENTS_IGNORE_NODRAW_OPAQUE 0x2000

// hits entities which are MOVETYPE_PUSH (doors, plats, etc.)
#define CONTENTS_MOVEABLE 0x4000

// remaining contents are non-visible, and don't eat brushes
#define CONTENTS_AREAPORTAL 0x8000

#define CONTENTS_PLAYERCLIP 0x10000
#define CONTENTS_MONSTERCLIP 0x20000

// currents can be added to any other contents, and may be mixed
#define CONTENTS_CURRENT_0 0x40000
#define CONTENTS_CURRENT_90 0x80000
#define CONTENTS_CURRENT_180 0x100000
#define CONTENTS_CURRENT_270 0x200000
#define CONTENTS_CURRENT_UP 0x400000
#define CONTENTS_CURRENT_DOWN 0x800000

#define CONTENTS_ORIGIN 0x1000000 // removed before bsping an entity

#define CONTENTS_MONSTER 0x2000000 // should never be on a brush, only in game
#define CONTENTS_DEBRIS 0x4000000
#define CONTENTS_DETAIL 0x8000000		// brushes to be added after vis leafs
#define CONTENTS_TRANSLUCENT 0x10000000 // auto set if any surface has trans
#define CONTENTS_LADDER 0x20000000
#define CONTENTS_HITBOX 0x40000000 // use accurate hitboxes on trace

#define MASK_ALL ( 0xFFFFFFFF )
// everything that is normally solid
#define MASK_SOLID ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE )
// everything that blocks player movement
#define MASK_PLAYERSOLID ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE )
// blocks npc movement
#define MASK_NPCSOLID ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_MONSTERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE )
// water physics in these contents
#define MASK_WATER ( CONTENTS_WATER | CONTENTS_MOVEABLE | CONTENTS_SLIME )
// everything that blocks lighting
#define MASK_OPAQUE ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_OPAQUE )
// everything that blocks lighting, but with monsters added.
#define MASK_OPAQUE_AND_NPCS ( MASK_OPAQUE | CONTENTS_MONSTER )
// everything that blocks line of sight for AI
#define MASK_BLOCKLOS ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_BLOCKLOS )
// everything that blocks line of sight for AI plus NPCs
#define MASK_BLOCKLOS_AND_NPCS ( MASK_BLOCKLOS | CONTENTS_MONSTER )
// everything that blocks line of sight for players
#define MASK_VISIBLE ( MASK_OPAQUE | CONTENTS_IGNORE_NODRAW_OPAQUE )
// everything that blocks line of sight for players, but with monsters added.
#define MASK_VISIBLE_AND_NPCS ( MASK_OPAQUE_AND_NPCS | CONTENTS_IGNORE_NODRAW_OPAQUE )
// bullets see these as solid
#define MASK_SHOT ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_MONSTER | CONTENTS_WINDOW | CONTENTS_DEBRIS | CONTENTS_HITBOX )
// non-raycasted weapons see this as solid (includes grates)
#define MASK_SHOT_HULL ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_MONSTER | CONTENTS_WINDOW | CONTENTS_DEBRIS | CONTENTS_GRATE )
// hits solids (not grates) and passes through everything else
#define MASK_SHOT_PORTAL ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER )
// everything normally solid, except monsters (world+brush only)
#define MASK_SOLID_BRUSHONLY ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_GRATE )
// everything normally solid for player movement, except monsters (world+brush only)
#define MASK_PLAYERSOLID_BRUSHONLY ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_PLAYERCLIP | CONTENTS_GRATE )
// everything normally solid for npc movement, except monsters (world+brush only)
#define MASK_NPCSOLID_BRUSHONLY ( CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTERCLIP | CONTENTS_GRATE )
// just the world, used for route rebuilding
#define MASK_NPCWORLDSTATIC ( CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_MONSTERCLIP | CONTENTS_GRATE )
// These are things that can split areaportals
#define MASK_SPLITAREAPORTAL ( CONTENTS_WATER | CONTENTS_SLIME )

// UNDONE: This is untested, any moving water
#define MASK_CURRENT ( CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN )

// everything that blocks corpse movement
// UNDONE: Not used yet / may be deleted
#define MASK_DEADSOLID ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_GRATE )


class matrix3x4;

struct Ray {
    Ray(const Vector& src, const Vector& dest)
        : start(src), delta(dest - src), extents(Vector{ }), startOffset(Vector{ }), isRay(true) { isSwept = delta.x || delta.y || delta.z; }

    Ray(const Vector& src, const Vector& dest, const Vector& mins, const Vector& maxs)
        : delta(dest - src), extents(maxs - mins), startOffset(maxs + mins)
    {
        isSwept = delta.x || delta.y || delta.z;
        extents *= 0.5f;
        isRay = (extents.squareLength() < 1e-6);

        startOffset *= 0.5f;
        start = src + startOffset;
        startOffset *= -1.0f;
    }
    Vector start{ };
    float pad{ };
    Vector delta{ };
    float pad1{ };
    Vector startOffset{ };
    float pad2{ };
    Vector extents{ };
    float pad3{ };
    bool isRay{ };
    bool isSwept{ };
};

class Entity;

enum TraceType
{
    TRACE_EVERYTHING = 0,
    TRACE_WORLD_ONLY,
    TRACE_ENTITIES_ONLY,
    TRACE_EVERYTHING_FILTER_PROPS,
};

class BaseTraceFilter 
{
public:
    virtual bool shouldHitEntity(Entity* entity, int) = 0;
    virtual int getTraceType() const = 0;
};

class TraceFilter : public BaseTraceFilter
{
public:
    virtual int getTraceType() const noexcept
    { 
        return TRACE_EVERYTHING; 
    }
};

class TraceFilterEntitiesOnly : public BaseTraceFilter
{
public:
    virtual int getTraceType() const noexcept
    {
        return TRACE_ENTITIES_ONLY;
    }
};

class TraceFilterWorldOnly : public BaseTraceFilter
{
public:
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept
    {
        return false;
    }

    virtual int	getTraceType() const noexcept
    {
        return TRACE_WORLD_ONLY;
    }
};

class TraceFilterWorldAndPropsOnly : public BaseTraceFilter
{
public:
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept
    {
        return false;
    }

    virtual int	getTraceType() const noexcept
    {
        return TRACE_EVERYTHING;
    }
};

class TraceFilterWorldCustom : public BaseTraceFilter
{
public:
    TraceFilterWorldCustom(Entity* entity) : passEntity{ entity } { }
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept;

    virtual int	getTraceType() const noexcept
    {
        return TRACE_EVERYTHING;
    }

    void* passEntity = nullptr;
};

class TraceFilterHitAll : public BaseTraceFilter
{
public:
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept
    {
        return true;
    }
};

class TraceFilterHitscan : public BaseTraceFilter
{
public:
    TraceFilterHitscan(Entity* entity) : passEntity{ entity } { }
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept;

    virtual int	getTraceType() const noexcept
    {
        return TRACE_EVERYTHING;
    }

    void* passEntity = nullptr;
};

class TraceFilterArc : public BaseTraceFilter
{
public:
    TraceFilterArc(Entity* entity) : passEntity{ entity } { }
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept;

    virtual int	getTraceType() const noexcept
    {
        return TRACE_EVERYTHING;
    }

    void* passEntity = nullptr;
};

class TraceFilterHitscanIgnoreTeammates : public BaseTraceFilter
{
public:
    TraceFilterHitscanIgnoreTeammates(Entity* entity) : passEntity{ entity } { }
    virtual bool shouldHitEntity(Entity* serverEntity, int contentsMask) noexcept;

    virtual int	getTraceType() const noexcept
    {
        return TRACE_EVERYTHING;
    }

    void* passEntity = nullptr;
};

typedef bool (*ShouldHitFunction)(Entity* handleEntity, int contentsMask);

class TraceFilterSimple : public TraceFilter
{
public:
    TraceFilterSimple(Entity* entity, int collisionGroup, ShouldHitFunction shouldHit = NULL) : passEntity{ entity }, collisionGroup{ collisionGroup }, extraShouldHitCheckFunction{ shouldHit } { }
    virtual bool shouldHitEntity(Entity* handleEntity, int contentsMask);
    virtual void setPassEntity(Entity* passEntity) { this->passEntity = passEntity; }
    virtual void setCollisionGroup(int collisionGroup) { this->collisionGroup = collisionGroup; }

    const void* getPassEntity() { return passEntity; }

private:
    Entity* passEntity;
	int collisionGroup;
    ShouldHitFunction extraShouldHitCheckFunction;
};

class TraceFilterIgnorePlayers : public TraceFilterSimple
{
public:
    TraceFilterIgnorePlayers(Entity* entity, int collisionGroup) : TraceFilterSimple(entity, collisionGroup) { }
    virtual bool shouldHitEntity(Entity* entity, int contentsMask);
};

class TraceFilterIgnoreTeammates : public TraceFilterSimple
{
public:
    TraceFilterIgnoreTeammates(Entity* entity, int collisionGroup, int ignoreTeam) : TraceFilterSimple(entity, collisionGroup), ignoreTeam(ignoreTeam) { }
    virtual bool shouldHitEntity(Entity* entity, int contentsMask);

    int ignoreTeam;
};

namespace HitGroup {
    enum {
        Invalid = -1,
        Generic,
        Head,
        Chest,
        Stomach,
        LeftArm,
        RightArm,
        LeftLeg,
        RightLeg,
        Gear = 10
    };
}

struct Trace {
    Vector startpos;
    Vector endpos;
    struct Plane {
        Vector normal{ };
        float dist{ };
        std::byte type{ };
        std::byte signBits{ };
        std::byte pad[2]{ };
    } plane;
    float fraction;
    int contents;
    unsigned short dispFlags;
    bool allSolid;
    bool startSolid;
    float fractionLeftSolid;
    struct Surface {
        const char* name;
        short surfaceProps;
        unsigned short flags;
    } surface;
    int hitgroup;
    short physicsBone;
    Entity* entity;
    int hitbox;

    bool didHit() { return (fraction < 1.0f || allSolid || startSolid); }
};

// #define TRACE_STATS // - enable to see how many rays are cast per frame

#ifdef TRACE_STATS
#include "../Memory.h"
#include "GlobalVars.h"
#endif

class EngineTrace {
public:
    VIRTUAL_METHOD(int, getPointContents, 0, (const Vector& absPosition, int contentsMask), (this, std::cref(absPosition), contentsMask))
    VIRTUAL_METHOD(void, _traceRay, 4, (const Ray& ray, unsigned int mask, const BaseTraceFilter& filter, Trace& trace), (this, std::cref(ray), mask, std::cref(filter), std::ref(trace)))

    void traceRay(const Ray& ray, unsigned int mask, const BaseTraceFilter& filter, Trace& trace) noexcept
    {
#ifdef TRACE_STATS
        static int tracesThisFrame, lastFrame;

        if (lastFrame != memory->globalVars->framecount) {
            std::string msg = "traces: frame - " + std::to_string(lastFrame) + " | count - " + std::to_string(tracesThisFrame) + "\n";
            memory->logDirect(msg.c_str());
            tracesThisFrame = 0;
            lastFrame = memory->globalVars->framecount;
        }

        ++tracesThisFrame;
#endif
        _traceRay(ray, mask, filter, trace);
    }
};
