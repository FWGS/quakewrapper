/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Misc utility code
//
#ifndef ENGINECALLBACK_H
#include "enginecallback.h"
#endif

#ifndef PHYSCALLBACK_H
#include "physcallback.h"
#endif

inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent );  // implementation later in this file

extern globalvars_t			*gpGlobals;

// Use this instead of ALLOC_STRING on constant strings
#define STRING(offset)		(const char *)(gpGlobals->pStringBase + (int)offset)
#define MAKE_STRING(str)	((int)str - (int)STRING(0))

// Testing the three types of "entity" for nullity
#define eoNullEntity 0

// Keeps clutter down a bit, when using a float as a bit-vector
#define SetBits(flBitVector, bits)		((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits)	((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit)		((int)(flBitVector) & (bit))

// Makes these more explicit, and easier to find
#define DLL_GLOBAL

// More explicit than "int"
typedef int EOFFSET;

// In case it's not alread defined
typedef int BOOL;

// In case this ever changes
#define M_PI			3.14159265358979323846

class CBaseEntity;

//
// Conversion among the three types of "entity", including identity-conversions.
//
inline edict_t *ENT(edict_t *pent)		{ return pent; }
inline edict_t *ENT(EOFFSET eoffset)			{ return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(EOFFSET eoffset)			{ return eoffset; }
inline EOFFSET OFFSET(const edict_t *pent)	
{ 
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent); 
}

inline int ENTINDEX(edict_t *pEdict)			{ return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
inline edict_t* INDEXENT( int iEdictNum )		{ return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }
inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, edict_t *ent ) {
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ent );
}

inline BOOL FNullEnt(EOFFSET eoffset) { return eoffset == 0; }
inline BOOL FNullEnt(const edict_t* pent) { return pent == NULL || FNullEnt(OFFSET(pent)); }

// Testing strings for nullity
#define iStringNull 0
inline BOOL FStringNull(int iString)			{ return iString == iStringNull; }

// Misc useful
inline BOOL FStrEq(const char*sz1, const char*sz2)
	{ return (strcmp(sz1, sz2) == 0); }
inline BOOL FClassnameIs(edict_t* pent, const char* szClassname)
	{ return FStrEq(STRING( pent->v.classname ), szClassname); }

extern float UTIL_AngleMod( float a );
extern float UTIL_AngleMod( float ideal, float current, float speed );

extern DLL_GLOBAL const Vector g_vecZero;
extern DLL_GLOBAL int g_iXashEngineBuildNumber;
extern DLL_GLOBAL BOOL g_fTouchSemaphore;
extern DLL_GLOBAL BOOL g_registered;

//
// Constants that were used only by QC (maybe not used at all now)
//
// Un-comment only as needed
//

#define SND_SPAWNING		(1<<8)		// duplicated in protocol.h we're spawing, used in some cases for ambients 
#define SND_STOP			(1<<5)		// duplicated in protocol.h stop sound
#define SND_CHANGE_VOL		(1<<6)		// duplicated in protocol.h change sound vol
#define SND_CHANGE_PITCH		(1<<7)		// duplicated in protocol.h change sound pitch

// quake skill flags
#define SF_NOT_EASY			256
#define SF_NOT_MEDIUM		512
#define SF_NOT_HARD			1024
#define SF_NOT_DEATHMATCH		2048

#define SVC_TEMPENTITY		23
#define SVC_INTERMISSION		30
#define SVC_FINALE			31
#define SVC_CDTRACK			32
#define SVC_WEAPONANIM		35
#define SVC_ROOMTYPE		37
#define SVC_DIRECTOR		51

// MoveToOrigin stuff
#define MOVE_NORMAL			0	// normal move in the direction monster is facing
#define MOVE_STRAFE			1	// moves in direction specified, no matter which way monster is facing

#define VEC_HULL_MIN		Vector(-16, -16, -24)
#define VEC_HULL_MAX		Vector( 16,  16,  32)

#define VEC_LARGE_HULL_MIN		Vector(-32, -32, -24)
#define VEC_LARGE_HULL_MAX		Vector( 32,  32,  64)

#define VEC_VIEW			Vector( 0, 0, 22 )

// Sound Utilities

// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
// is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
// down to 1 is a lower pitch.   150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
// fast as EMIT_SOUND (the pitchshift mixer is not native coded).

inline void EMIT_SOUND(edict_t *entity, int channel, const char *sample, float volume, float attenuation)
{
	EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t *entity, int channel, const char *sample)
{
	EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

float anglemod( float a );

int SV_WalkMove( edict_t *pEdict, float flYaw, float flDist );
int SV_MoveToGoal( edict_t *pEdict, edict_t *pGoal, float flDist );

extern void UTIL_StringToVector( float *pVector, const char *pString );
extern void UTIL_StringToIntArray( int *pVector, int count, const char *pString );
extern void UTIL_StringToFloatArray( float *pVector, int count, const char *pString );
extern void UTIL_LogPrintf( char *fmt, ... );

void CRC_Init( word *crcvalue );
void CRC_ProcessByte( word *crcvalue, byte data );
word CRC_Value( word crcvalue );
	 