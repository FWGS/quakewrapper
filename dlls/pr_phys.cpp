/*
pr_phys.cpp - Quake virtual machine wrapper impmenentation
Copyright (C) 2017 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll.h"
#include "util.h"
#include "saverestore.h"
#include "client.h"
#include "game.h"
#include "progs.h"

#define IS_NAN( x )		(((*(int *)&x) & (255<<23)) == (255<<23))
#define STOP_EPSILON	0.1f
#define MAX_CLIP_PLANES	5

/*
=============
SV_PlayerRunThink

Runs thinking code if player time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
BOOL SV_PlayerRunThink( edict_t *ent, float frametime, double time )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(ent);
	float thinktime;

	thinktime = pev->nextthink;

	if( thinktime <= 0.0f || thinktime > (time + frametime))
	{
		// update fields before pmove
		ED_UpdateEdictFields( ent );
		return true;
          }

	if( thinktime < time )
		thinktime = time;	// don't let things stay in the past.
				// it is possible to start that way
				// by a trigger with a local time.

	pev->nextthink = 0;
	pr.global_struct->time = gpGlobals->time = thinktime;	// ouch!!!
	pr.global_struct->self = ENTINDEX( ent );
	pr.global_struct->other = ENTINDEX( 0 );

	PR_ExecuteProgram( pev->think );

	if( FBitSet( ent->v.flags, FL_KILLME ))
	{
		ALERT( at_error, "client can't be deleted!\n" );
		ClearBits( ent->v.flags, FL_KILLME );
          }

	// update fields before pmove
	ED_UpdateEdictFields( ent );

	return !ent->free;
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
BOOL SV_RunThink( edict_t *pEdict )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	float thinktime;

	thinktime = pev->nextthink;

	if (thinktime <= 0 || thinktime > PHYSICS_TIME() + gpGlobals->frametime)
		return true;
		
	if (thinktime < PHYSICS_TIME())
		thinktime = PHYSICS_TIME();	// don't let things stay in the past.
					// it is possible to start that way
					// by a trigger with a local time.
	pev->nextthink = 0;
	pr.global_struct->time = gpGlobals->time = thinktime;	// ouch!!!
	pr.global_struct->self = ENTINDEX( pEdict );
	pr.global_struct->other = ENTINDEX( 0 );

	PR_ExecuteProgram( pev->think );

	return !pEdict->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact( edict_t *pEdict1, edict_t *pEdict2, TraceResult *trace )
{
	pr_entvars_t *pev1 = (pr_entvars_t *)GET_PRIVATE(pEdict1);
	pr_entvars_t *pev2 = (pr_entvars_t *)GET_PRIVATE(pEdict2);
	int old_self = pr.global_struct->self;
	int old_other = pr.global_struct->other;

	pr.global_struct->time = gpGlobals->time = PHYSICS_TIME();
	SV_CopyTraceToGlobal( trace );

	if( pev1 && pev1->touch && pev1->solid != SOLID_NOT )
	{
		pr.global_struct->self = ENTINDEX(pEdict1);
		pr.global_struct->other = ENTINDEX(pEdict2);
		PR_ExecuteProgram (pev1->touch);
	}

	if( pev2 && pev2->touch && pev2->solid != SOLID_NOT )
	{
		pr.global_struct->self = ENTINDEX(pEdict2);
		pr.global_struct->other = ENTINDEX(pEdict1);
		PR_ExecuteProgram (pev2->touch);
	}

	pr.global_struct->self = old_self;
	pr.global_struct->other = old_other;
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (edict_t *pEdict)
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	float maxVelocity;

	maxVelocity = CVAR_GET_FLOAT( "sv_maxvelocity" );

	// bound velocity
	for( int i = 0; i < 3; i++ )
	{
		if (IS_NAN(pev->velocity[i]))
		{
			ALERT( at_console, "Got a NaN velocity on %s\n", STRING( pev->classname ));
			pev->velocity[i] = 0;
		}
		if (IS_NAN(pev->origin[i]))
		{
			ALERT( at_console, "Got a NaN origin on %s\n", STRING( pev->classname ));
			pev->origin[i] = 0;
		}
		if (pev->velocity[i] > maxVelocity)
			pev->velocity[i] = maxVelocity;
		else if (pev->velocity[i] < -maxVelocity)
			pev->velocity[i] = -maxVelocity;
	}
}

/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
int SV_ClipVelocity( Vector in, Vector normal, Vector &out, float overbounce )
{
	float	backoff;
	float	change;
	int	blocked = 0;
	
	if (normal.z > 0) blocked |= 1;	// floor
	if (!normal.z) blocked |= 2;		// step
	
	backoff = DotProduct( in, normal ) * overbounce;

	for( int i = 0; i < 3; i++ )
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	
	return blocked;
}

/*
============
SV_AddGravity

============
*/
void SV_AddGravity (edict_t *pEdict)
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	float		ent_gravity, gravity;

	gravity = CVAR_GET_FLOAT ("sv_gravity");

	eval_t *val = GETEDICTFIELDVALUE( pEdict, pr.eval_gravity );

	if( val && val->value )
		ent_gravity = val->value;
	else ent_gravity = 1.0f;

	pev->velocity[2] -= ent_gravity * gravity * gpGlobals->frametime;
}

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
TraceResult SV_PushEntity (edict_t *pEdict, const Vector &push)
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	TraceResult trace;
	Vector end;

	end = pev->origin + push;		

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes

	if (pev->movetype == MOVETYPE_FLYMISSILE)
		TRACE_MONSTER_HULL( pEdict, pev->origin, end, 2, pEdict, &trace ); 
	else if (pev->solid == SOLID_TRIGGER || pev->solid == SOLID_NOT)
		// only clip against bmodels
		TRACE_MONSTER_HULL( pEdict, pev->origin, end, 1, pEdict, &trace ); 
	else
		TRACE_MONSTER_HULL( pEdict, pev->origin, end, 0, pEdict, &trace ); 
	
	pev->origin = trace.vecEndPos;
	SV_LinkEdict( pEdict, true );

	if (trace.pHit)
		SV_Impact (pEdict, trace.pHit, &trace);		

	return trace;
}

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition( edict_t *pEdict )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	int cont = POINT_CONTENTS(pev->origin);

	if (!pev->watertype)
	{
		// just spawned here
		pev->watertype = cont;
		pev->waterlevel = 1;
		return;
	}
	
	if (cont <= CONTENTS_WATER)
	{
		if (pev->watertype == CONTENTS_EMPTY)
		{	
			// just crossed into water
			EMIT_SOUND( pEdict, CHAN_AUTO, "misc/h2ohit1.wav", 1, ATTN_NORM );
		}		
		pev->watertype = cont;
		pev->waterlevel = 1;
	}
	else
	{
		if (pev->watertype != CONTENTS_EMPTY)
		{	
			// just crossed into water
			EMIT_SOUND( pEdict, CHAN_AUTO, "misc/h2ohit1.wav", 1, ATTN_NORM );
		}		
		pev->watertype = CONTENTS_EMPTY;
		pev->waterlevel = cont;
	}
}

/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
int SV_FlyMove( edict_t *pEdict, float time, TraceResult *steptrace )
{
	int	numplanes;
	Vector	planes[MAX_CLIP_PLANES];
	Vector	primal_velocity, original_velocity, new_velocity;
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	TraceResult trace;
	float	time_left;
	int	blocked;
	int	i, j;

	blocked = 0;
	numplanes = 0;
	original_velocity = primal_velocity = pev->velocity;
	
	time_left = time;

	for( int bumpcount = 0; bumpcount < 4; bumpcount++ )
	{
		if (pev->velocity == g_vecZero)
			break;

		Vector end = pev->origin + time_left * pev->velocity;

		TRACE_MONSTER_HULL( pEdict, pev->origin, end, FALSE, pEdict, &trace );

		if (trace.fAllSolid)
		{	
			// entity is trapped in another solid
			pev->velocity = g_vecZero;
			return 3;
		}

		if (trace.flFraction > 0.0f)
		{	
			// actually covered some distance
			pev->origin = trace.vecEndPos;
			original_velocity = pev->velocity;
			numplanes = 0;
		}

		if (trace.flFraction == 1.0f)
			 break; // moved the entire distance

		if( !trace.pHit )
		{
			ALERT( at_error, "SV_FlyMove: trace.pHit == NULL\n" );
			pev->velocity = g_vecZero;
			return 3;
		}

		if (trace.vecPlaneNormal.z > 0.7f)
		{
			pr_entvars_t *pevHit = (pr_entvars_t *)GET_PRIVATE(trace.pHit);

			blocked |= 1; // floor
			if (pevHit->solid == SOLID_BSP)
			{
				SetBits( pev->flags, FL_ONGROUND );
				pev->groundentity = ENTINDEX( trace.pHit );
			}
		}

		if (!trace.vecPlaneNormal.z)
		{
			blocked |= 2; // step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

		// run the impact function
		SV_Impact (pEdict, trace.pHit, &trace);

		// break if removed by the impact function
		if( pEdict->free || FBitSet( pev->flags, FL_KILLME ))
			break;
		
		time_left -= time_left * trace.flFraction;
		
		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			pev->velocity = g_vecZero;
			return 3;
		}

		planes[numplanes] = trace.vecPlaneNormal;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++)
		{
			SV_ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
			for (j = 0; j < numplanes; j++)
			{
				if( j != i )
				{
					if( DotProduct( new_velocity, planes[j] ) < 0.0f )
						break; // not ok
				}
			}

			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	
			// go along this plane
			pev->velocity = new_velocity;
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				pev->velocity = g_vecZero;
				return 7;
			}

			Vector dir = CrossProduct (planes[0], planes[1]);
			float d = DotProduct (dir, pev->velocity);
			pev->velocity = dir * d;
		}

		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct (pev->velocity, primal_velocity) <= 0.0f)
		{
			pev->velocity = g_vecZero;
			return blocked;
		}
	}
	return blocked;
}

/*
============
SV_PushMove

============
*/
void SV_PushMove( edict_t *pusher, float movetime )
{
	pr_entvars_t	*pevPusher = (pr_entvars_t *)GET_PRIVATE( pusher );
	vec3_t		mins, maxs, move;
	vec3_t		entorig, pushorig;

	if( pevPusher->velocity == g_vecZero )
	{
		pevPusher->ltime += movetime;
		return;
	}

	move = pevPusher->velocity * movetime;
	mins = pevPusher->absmin + move;
	maxs = pevPusher->absmax + move;

	pushorig = pevPusher->origin;

	// move the pusher to it's final position
	pevPusher->origin += move;
	pevPusher->ltime += movetime;
	SV_LinkEdict( pusher, false );
	pr.num_moved = 0;

	// see if any solid entities are inside the final position
	for( int e = 1; e < gpGlobals->maxEntities; e++ )
	{
		edict_t	*check;

		if(( check = INDEXENT( e )) == NULL )
			break;

		pr_entvars_t *pevCheck = (pr_entvars_t *)GET_PRIVATE( check );

		if( check->free || !pevCheck )
			continue;
		if( pevCheck->movetype == MOVETYPE_PUSH || pevCheck->movetype == MOVETYPE_NONE || pevCheck->movetype == MOVETYPE_NOCLIP )
			continue;

		// if the entity is standing on the pusher, it will definately be moved
		if( !( FBitSet( pevCheck->flags, FL_ONGROUND ) && INDEXENT( pevCheck->groundentity ) == pusher ) )
		{
			if( pevCheck->absmin[0] >= maxs[0]
			 || pevCheck->absmin[1] >= maxs[1]
			 || pevCheck->absmin[2] >= maxs[2]
			 || pevCheck->absmax[0] <= mins[0]
			 || pevCheck->absmax[1] <= mins[1]
			 || pevCheck->absmax[2] <= mins[2] )
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if( !SV_TestEntityPosition( check ))
				continue;
		}

		// remove the onground flag for non-players
		if( pevCheck->movetype != MOVETYPE_WALK )
			ClearBits( pevCheck->flags, FL_ONGROUND );
		
		entorig = pevCheck->origin;
		pr.moved_from[pr.num_moved] = pevCheck->origin;
		pr.moved_edict[pr.num_moved] = check;
		pr.num_moved++;

		// try moving the contacted entity 
		pevPusher->solid = SOLID_NOT;
		SV_PushEntity( check, move );
		pevPusher->solid = SOLID_BSP;

		// if it is still inside the pusher, block
		if( SV_TestEntityPosition( check ))
		{
			// fail the move
			if( pevCheck->mins[0] == pevCheck->maxs[0] )
				continue;

			if( pevCheck->solid == SOLID_NOT || pevCheck->solid == SOLID_TRIGGER )
			{	
				// corpse
				pevCheck->mins[0] = pevCheck->mins[1] = 0;
				pevCheck->maxs = pevCheck->mins;
				continue;
			}
			
			pevCheck->origin = entorig;
			SV_LinkEdict( check, true );

			pevPusher->origin = pushorig;
			SV_LinkEdict( pusher, false );
			pevPusher->ltime -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if( pevPusher->blocked )
			{
				pr.global_struct->self = ENTINDEX( pusher );
				pr.global_struct->other = ENTINDEX( check );
				PR_ExecuteProgram( pevPusher->blocked );
			}
			
			// move back any entities we already moved
			for( int i = 0; i < pr.num_moved; i++ )
			{
				pr_entvars_t *pevMoved = (pr_entvars_t *)GET_PRIVATE( pr.moved_edict[i] );
				pevMoved->origin = pr.moved_from[i];
				SV_LinkEdict( pr.moved_edict[i], false );
			}
			return;
		}	
	}

	
}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher( edict_t *ent )
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE( ent );
	float		thinktime, oldltime, movetime;

	oldltime = pev->ltime;
	thinktime = pev->nextthink;

	if( thinktime < pev->ltime + gpGlobals->frametime )
	{
		movetime = thinktime - pev->ltime;
		if( movetime < 0 ) movetime = 0;
	}
	else movetime = gpGlobals->frametime;

	if( movetime )
		SV_PushMove( ent, movetime );	// advances pev->ltime if not blocked
		
	if( thinktime > oldltime && thinktime <= pev->ltime )
	{
		pev->nextthink = 0;
		pr.global_struct->time = gpGlobals->time;
		pr.global_struct->self = ENTINDEX(ent);
		pr.global_struct->other = ENTINDEX( 0 );
		PR_ExecuteProgram( pev->think );
	}
}

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None( edict_t *ent )
{
	// regular thinking
	SV_RunThink( ent );
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip( edict_t *ent )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );

	// regular thinking
	if( !SV_RunThink( ent ))
		return;
	
	pev->angles += pev->avelocity * gpGlobals->frametime;
	pev->origin += pev->velocity * gpGlobals->frametime;

	SV_LinkEdict( ent, FALSE );
}

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step( edict_t *pEdict )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	BOOL hitsound;

	// freefall if not onground
	if ( !FBitSet( pev->flags, ( FL_ONGROUND|FL_FLY|FL_SWIM )))
	{
		float gravity = CVAR_GET_FLOAT ("sv_gravity");
		if (pev->velocity.z < gravity * -0.1f)
			hitsound = true;
		else
			hitsound = false;

		SV_AddGravity( pEdict );
		SV_CheckVelocity( pEdict );
		SV_FlyMove( pEdict, gpGlobals->frametime, NULL );
		if (pEdict->free) return; // removed by impact function

		SV_LinkEdict( pEdict, true );

		if (FBitSet( pev->flags, FL_ONGROUND ))	// just hit ground
		{
			if( hitsound )
				EMIT_SOUND( pEdict, CHAN_AUTO, "demon/dland2.wav", 1, ATTN_NORM);
		}
	}

	// regular thinking
	if( !SV_RunThink( pEdict ))
		return;
	
	SV_CheckWaterTransition( pEdict );
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
g-cont. Override physics toss with quake code
=============
*/
void SV_Physics_Toss( edict_t *pEdict )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);

	// regular thinking
	if (!SV_RunThink( pEdict ))
		return;

	// if onground, return without moving
	if (FBitSet( pev->flags, FL_ONGROUND ))
		return;

	SV_CheckVelocity (pEdict);

	// add gravity
	if (pev->movetype != MOVETYPE_FLY && pev->movetype != MOVETYPE_FLYMISSILE)
		SV_AddGravity (pEdict);

	// move angles
	pev->angles += pev->avelocity * gpGlobals->frametime;

	TraceResult trace;
	Vector move;

	move = pev->velocity * gpGlobals->frametime;
	trace = SV_PushEntity (pEdict, move);

	if (trace.flFraction == 1)
		return;
	if (pEdict->free)
		return;

	float backoff;
	
	if (pev->movetype == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else
		backoff = 1;

	SV_ClipVelocity(pev->velocity, trace.vecPlaneNormal, pev->velocity, backoff);

	// stop if on ground
	if (trace.vecPlaneNormal[2] > 0.7)
	{		
		if (pev->velocity[2] < 60 || pev->movetype != MOVETYPE_BOUNCE)
		{
			SetBits(pev->flags, FL_ONGROUND);
			pev->groundentity = ENTINDEX(trace.pHit);
			pev->velocity = g_vecZero;
			pev->avelocity = g_vecZero;
		}
	}
	
	// check for in water
	SV_CheckWaterTransition (pEdict);
}

//
// assume pEntity is valid
//
int RunPhysicsFrame( edict_t *pEdict )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);

	if( pr.global_struct->force_retouch != 0.0f )
	{
		// force retouch even for stationary
		SV_LinkEdict( pEdict, true );
	}

	// hack to save termporary entities
	if( pev->classname == iStringNull )
		pev->classname = MAKE_STRING( "temp_entity" );

	// NOTE: at this point pEntity assume to be valid
	switch( (int)pev->movetype )
	{
	case MOVETYPE_NONE:
		SV_Physics_None (pEdict);
		break;
	case MOVETYPE_NOCLIP:
		SV_Physics_Noclip (pEdict);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
		SV_Physics_Toss (pEdict);
		break;
	case MOVETYPE_STEP:
		SV_Physics_Step (pEdict);
		break;
	case MOVETYPE_PUSH:
		SV_Physics_Pusher (pEdict);
		break;
	default:
		HOST_ERROR( "SV_Physics: bad movetype %i\n", (int)pev->movetype );
		break;
	}

	return 1;
}
