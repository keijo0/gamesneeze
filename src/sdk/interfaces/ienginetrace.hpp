#pragma once
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "../sdk.hpp"
#include "interfaces.hpp"
#include "../classes/entity.hpp"

// Contents flags
#define CONTENTS_SOLID                0x1
#define CONTENTS_MOVEABLE             0x8
#define CONTENTS_WINDOW               0x20
#define CONTENTS_MONSTER              0x2000
#define CONTENTS_GRATE                0x2000
#define CONTENTS_PLAYERCLIP           0x20000
#define CONTENTS_MONSTERCLIP          0x10000
#define CONTENTS_OPAQUE               0x80
#define CONTENTS_BLOCKLOS             0x2
#define CONTENTS_DEBRIS               0x4000000
#define CONTENTS_HITBOX               0x4000000

// Mask definitions
#define MASK_SOLID                    (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define MASK_PLAYERSOLID              (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define MASK_NPCSOLID                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE)
#define MASK_NPCFLUID                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_OPAQUE                   (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE)
#define MASK_BLOCKLOS                 (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_BLOCKLOS)
#define MASK_SHOT                     (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
#define MASK_SHOT_BRUSHONLY           (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_DEBRIS)
#define MASK_SHOT_HULL                (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE)
#define MASK_SHOT_PORTAL              (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_SOLID_BRUSHONLY          (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE)
#define MASK_PLAYERSOLID_BRUSHONLY    (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE)
#define MASK_NPCSOLID_BRUSHONLY       (CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
#define MASK_NPCWORLDSTATIC           (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE)
#define MASK_NPCWORLDSTATIC_FLUID     (CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP)
#define MASK_DEADSOLID                (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_GRATE)

struct Plane
{
	Vector normal;
	float dist;
	unsigned char type;
	unsigned char signbits;
	unsigned char pad[2];
};

struct Surface
{
	const char *name;
	short surfaceProps;
	unsigned short flags;
};

struct Trace
{
	Vector startpos;
	Vector endpos;
	Plane plane;

	float fraction;

	int contents;
	unsigned int dispFlags;

	bool allsolid;
	bool startsolid;

	float fractionleftsolid;

	Surface surface;

	HitGroups hitgroup;
	short physicsbone;

	unsigned short worldSurfaceIndex;
	Player* m_pEntityHit;
	int hitbox;
};

struct Ray
{
	VectorAligned m_Start;
	VectorAligned m_Delta;
	VectorAligned m_StartOffset;
	VectorAligned m_Extents;

	const matrix3x4_t *m_pWorldAxisTransform;

	bool m_IsRay;
	bool m_IsSwept;

	Ray() : m_pWorldAxisTransform(nullptr)
	{}

	void Init(Vector vecStart, Vector vecEnd)
	{
		m_Delta = vecEnd - vecStart;
		m_IsSwept = (m_Delta.LengthSqr() != 0);
		m_Extents.x = m_Extents.y = m_Extents.z = 0.0f;
		m_pWorldAxisTransform = nullptr;
		m_IsRay = true;
		m_StartOffset.x = m_StartOffset.y = m_StartOffset.z = 0.0f;
		m_Start = vecStart;
	}

	void Init(Vector const &start, Vector const &end, Vector const &mins, Vector const &maxs)
	{
		m_Delta = end - start;

		m_pWorldAxisTransform = NULL;
		m_IsSwept = (m_Delta.LengthSqr() != 0);

		m_Extents = maxs - mins;
		m_Extents *= 0.5f;
		m_IsRay = (m_Extents.LengthSqr() < 1e-6);

		// Offset m_Start to be in the center of the box...
		m_StartOffset = maxs + mins;
		m_StartOffset *= 0.5f;
		m_Start = start + m_StartOffset;
		m_StartOffset *= -1.0f;
	}
};

class ITraceFilter
{
public:
	virtual bool ShouldHitEntity(Player* pEntity, int contentsMask) = 0;

	virtual int GetTraceType() const = 0;
};

class TraceFilter : public ITraceFilter
{
public:
	bool ShouldHitEntity(Player* pEntityHandle, int contentsMask)
	{
		return !(pEntityHandle == pSkip);
	}

	virtual int GetTraceType() const
	{
		return 0;
	}

	void *pSkip;
};

class CTraceFilterWorldOnly : public ITraceFilter
{
public:
	bool ShouldHitEntity(Player* pEntityHandle, int contentsMask)
	{
		return false;
	}

	virtual int GetTraceType() const
	{
		return 1; // TRACE_WORLD_ONLY
	}
};

class IEngineTrace {
public:
	const char* TraceRay(Ray &ray, unsigned int fMask, ITraceFilter *pTraceFilter,  Trace  *pTrace) {
		typedef const char* (*Fn)(void*, Ray&, unsigned int, ITraceFilter*, Trace*);
		return getVirtualFunc<Fn>(this, 5)(this, ray, fMask, pTraceFilter, pTrace);
	}
};

