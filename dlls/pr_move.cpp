/*
pr_move.cpp - Quake virtual machine wrapper impmenentation
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

#define DI_NODIR	-1

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
BOOL SV_CheckBottom( edict_t *ent )
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE(ent);
	vec3_t		mins, maxs, start, stop;
	float		mid, bottom;
	TraceResult	trace;
	int		x, y;

	mins = pev->origin + pev->mins;
	maxs = pev->origin + pev->maxs;

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1.0f;

	for( x = 0; x <= 1; x++ )
	{
		for( y = 0; y <= 1; y++ )
		{
			start.x = x ? maxs.x : mins.x;
			start.y = y ? maxs.y : mins.y;

			if( POINT_CONTENTS( start ) != CONTENTS_SOLID )
				goto realcheck;
		}
	}
	return true; // we got out easy
realcheck:
	// check it for real...
	start.z = mins.z;

	float stepSize = CVAR_GET_FLOAT( "sv_stepsize" );

	// the midpoint must be within 16 of the bottom
	start.x = stop.x = (mins.x + maxs.x) * 0.5f;
	start.y = stop.y = (mins.y + maxs.y) * 0.5f;
	stop.z = start.z - 2.0f * stepSize;

	TRACE_LINE( start, stop, TRUE, ent, &trace );

	if( trace.flFraction == 1.0f )
		return false;

	mid = bottom = trace.vecEndPos.z;

	// the corners must be within 16 of the midpoint
	for( x = 0; x <= 1; x++ )
	{
		for( y = 0; y <= 1; y++ )
		{
			start.x = stop.x = x ? maxs.x : mins.x;
			start.y = stop.y = y ? maxs.y : mins.y;

			TRACE_LINE( start, stop, TRUE, ent, &trace );

			if( trace.flFraction != 1.0f && trace.vecEndPos.z > bottom )
				bottom = trace.vecEndPos.z;
			if( trace.flFraction == 1.0f || mid - trace.vecEndPos.z > stepSize )
				return false;
		}
	}
	return true;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
BOOL SV_MoveStep( edict_t *pEdict, const vec3_t &vecMove, BOOL relink )
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	TraceResult	trace;
	float		dz;

	// try the move	
	Vector oldorg = pev->origin;
	Vector neworg = pev->origin + vecMove;

	pEdict->v.mins = pev->mins;
	pEdict->v.maxs = pev->maxs;

	// flying monsters don't step up
	if( FBitSet( pev->flags, FL_SWIM|FL_FLY ))
	{
		// try one move with vertical motion, then one without
		for( int i = 0; i < 2; i++ )
		{
			edict_t *pEnemy = INDEXENT( pev->enemy );
			neworg = pev->origin + vecMove;

			if( i == 0 && !FNullEnt( pEnemy ))
			{
				pr_entvars_t *pevEnemy = (pr_entvars_t *)GET_PRIVATE(pEnemy);
				dz = pev->origin.z - pevEnemy->origin.z;

				if( dz > 40 )
					neworg.z -= 8;
				if( dz < 30 )
					neworg.z += 8;
			}

			TRACE_MONSTER_HULL( pEdict, pev->origin, neworg, FALSE, pEdict, &trace ); 
	
			if( trace.flFraction == 1.0f )
			{
				if( FBitSet( pev->flags, FL_SWIM ) && POINT_CONTENTS( trace.vecEndPos ) == CONTENTS_EMPTY )
					return FALSE; // swim monster left water
	
				pev->origin = trace.vecEndPos;

				if( relink )
					SV_LinkEdict( pEdict, TRUE );
				return TRUE;
			}
			
			if( FNullEnt( pEnemy ))
				break;
		}

		return FALSE;
	}

	float stepSize = CVAR_GET_FLOAT( "sv_stepsize" );

	// push down from a step height above the wished position
	neworg.z += stepSize;
	Vector end = neworg;
	end.z -= stepSize * 2;

	TRACE_MONSTER_HULL( pEdict, neworg, end, FALSE, pEdict, &trace ); 

	if( trace.fAllSolid )
		return FALSE;

	if( trace.fStartSolid )
	{
		neworg.z -= stepSize;
		TRACE_MONSTER_HULL( pEdict, neworg, end, FALSE, pEdict, &trace ); 

		if( trace.fAllSolid || trace.fStartSolid )
			return FALSE;
	}

	if( trace.flFraction == 1.0f )
	{
		// if monster had the ground pulled out, go ahead and fall
		if( FBitSet( pev->flags, FL_PARTIALGROUND ))
		{
			pev->origin += vecMove;
			ClearBits( pev->flags, FL_ONGROUND );

			if( relink )
				SV_LinkEdict( pEdict, TRUE );
			return TRUE;
		}
	
		return FALSE; // walked off an edge
	}

	// check point traces down for dangling corners
	pev->origin = trace.vecEndPos;

	if( !SV_CheckBottom( pEdict ))	
	{
		if( FBitSet( pev->flags, FL_PARTIALGROUND ))
		{	
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if( relink )
				SV_LinkEdict( pEdict, TRUE );
			return TRUE;
		}

		pev->origin = oldorg;
		return FALSE;
	}

	if( FBitSet( pev->flags, FL_PARTIALGROUND ))
		ClearBits( pev->flags, FL_PARTIALGROUND );

	pev->groundentity = ENTINDEX( trace.pHit );

	// the move is ok
	if( relink )
		SV_LinkEdict( pEdict, TRUE );
	return TRUE;
}

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
BOOL SV_StepDirection( edict_t *pEdict, float flYaw, float flDist )
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE(pEdict);
	vec3_t		move, oldorigin;
	float		delta;
	
	pev->ideal_yaw = flYaw;
	pev->angles.y = UTIL_AngleMod( pev->ideal_yaw, pev->angles.y, pev->yaw_speed );

	flYaw = flYaw * M_PI * 2 / 360;
	move.x = cos( flYaw ) * flDist;
	move.y = sin( flYaw ) * flDist;
	move.z = 0.0f;

	oldorigin = pev->origin;

	if( SV_MoveStep( pEdict, move, false ))
	{
		delta = pev->angles.y - pev->ideal_yaw;

		if( delta > 45 && delta < 315 )
		{	
			// not turned far enough, so don't take the step
			pev->origin = oldorigin;
		}
                    
		SV_LinkEdict( pEdict, TRUE );
		return TRUE;
	}

	SV_LinkEdict( pEdict, TRUE );

	return FALSE;
}

/*
================
SV_NewChaseDir

================
*/
void SV_NewChaseDir( edict_t *pActor, edict_t *pEnemy, float flDist )
{
	pr_entvars_t	*pevActor = (pr_entvars_t *)GET_PRIVATE(pActor);
	pr_entvars_t	*pevEnemy = (pr_entvars_t *)GET_PRIVATE(pEnemy);
	float		tdir, olddir, turnaround;
	float		deltax, deltay;
	vec3_t		d;

	olddir = UTIL_AngleMod( (int)(pevActor->ideal_yaw / 45 ) * 45 );
	turnaround = UTIL_AngleMod( olddir - 180 );

	deltax = pevEnemy->origin.x - pevActor->origin.x;
	deltay = pevEnemy->origin.y - pevActor->origin.y;

	if( deltax > 10 )
		d[1] = 0;
	else if( deltax < -10 )
		d[1] = 180;
	else d[1] = DI_NODIR;

	if( deltay < -10 )
		d[2] = 270;
	else if( deltay > 10 )
		d[2] = 90;
	else d[2] = DI_NODIR;

	// try direct route
	if( d[1] != DI_NODIR && d[2] != DI_NODIR )
	{
		if( d[1] == 0 )
			tdir = d[2] == 90 ? 45 : 315;
		else tdir = d[2] == 90 ? 135 : 215;
			
		if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
			return;
	}

	// try other directions
	if((( rand() & 3 ) & 1 ) || abs( deltay ) > abs( deltax ))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if( d[1] != DI_NODIR && d[1] != turnaround && SV_StepDirection( pActor, d[1], flDist ))
		return;

	if( d[2] != DI_NODIR && d[2] != turnaround && SV_StepDirection( pActor, d[2], flDist ))
		return;

	// there is no direct path to the player, so pick another direction
	if( olddir != -1 && SV_StepDirection( pActor, olddir, flDist ))
		return;

	if( rand() & 1 ) // randomly determine direction of search
	{
		for( tdir = 0; tdir <= 315; tdir += 45 )
		{
			if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
				return;
		}
	}
	else
	{
		for( tdir = 315; tdir >= 0; tdir -= 45 )
		{
			if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
				return;
		}
	}

	if( turnaround != DI_NODIR && SV_StepDirection( pActor, turnaround, flDist ))
		return;

	pevActor->ideal_yaw = olddir; // can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all
	if( !SV_CheckBottom( pActor ))
		SetBits( pevActor->flags, FL_PARTIALGROUND );
}

/*
======================
SV_CloseEnough

======================
*/
BOOL SV_CloseEnough( edict_t *pEdict, edict_t *pGoal, float flDist )
{
	pr_entvars_t *pevEdict = (pr_entvars_t *)GET_PRIVATE(pEdict);
	pr_entvars_t *pevGoal = (pr_entvars_t *)GET_PRIVATE(pGoal);

	for( int i = 0; i < 3; i++ )
	{
		if( pevGoal->absmin[i] > pevEdict->absmax[i] + flDist )
			return FALSE;
		if( pevGoal->absmax[i] < pevEdict->absmin[i] - flDist )
			return FALSE;
	}
	return TRUE;
}

/*
======================
SV_MoveToGoal

======================
*/
int SV_MoveToGoal( edict_t *pEdict, edict_t *pGoal, float flDist )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pEdict);

	if( !FBitSet( pev->flags, FL_ONGROUND|FL_FLY|FL_SWIM ))
		return 0;

	if( pev->enemy && SV_CloseEnough( pEdict, pGoal, flDist ))
		return 0;

	// bump around...
	if(( rand() & 3 ) == 1 || !SV_StepDirection( pEdict, pev->ideal_yaw, flDist ))
	{
		SV_NewChaseDir( pEdict, pGoal, flDist );
	}

	return 1;
}

/*
======================
SV_WalkMove

======================
*/
int SV_WalkMove( edict_t *pEdict, float flYaw, float flDist )
{
	pr_entvars_t *pevEdict = (pr_entvars_t *)GET_PRIVATE(pEdict);
	vec3_t move;

	if( !FBitSet( pevEdict->flags, FL_ONGROUND|FL_FLY|FL_SWIM ))
		return 0;

	flYaw = flYaw * M_PI * 2 / 360;
	move.x = cos( flYaw ) * flDist;
	move.y = sin( flYaw ) * flDist;
	move.z = 0.0f;

	return SV_MoveStep( pEdict, move, true );
}