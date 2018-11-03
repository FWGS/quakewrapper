/*
pr_world.cpp - Quake virtual machine wrapper impmenentation
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
#include "client.h"
#include "physcallback.h"
#include "com_model.h"
#include "enginefeatures.h"

/*
==================
SV_CopyTraceToGlobal

share local trace with global variables
==================
*/
void SV_CopyTraceToGlobal( TraceResult *trace )
{
	gpGlobals->trace_allsolid = trace->fAllSolid;
	gpGlobals->trace_startsolid = trace->fStartSolid;
	gpGlobals->trace_fraction = trace->flFraction;
	gpGlobals->trace_plane_dist = trace->flPlaneDist;
	gpGlobals->trace_flags = 0;
	gpGlobals->trace_inopen = trace->fInOpen;
	gpGlobals->trace_inwater = trace->fInWater;
	gpGlobals->trace_endpos = trace->vecEndPos;
	gpGlobals->trace_plane_normal = trace->vecPlaneNormal;
	gpGlobals->trace_hitgroup = trace->iHitgroup;

	if( !FNullEnt( trace->pHit ))
		gpGlobals->trace_ent = trace->pHit;
	else gpGlobals->trace_ent = INDEXENT(0); // world

	PR_SetTraceGlobals();
}

/*
============
SV_TestEntityPosition

This could be a lot more efficient...
============
*/
BOOL SV_TestEntityPosition( edict_t *ent )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );
	TraceResult trace;

	TRACE_MONSTER_HULL( ent, pev->origin, pev->origin, FALSE, ent, &trace ); 

	return trace.fStartSolid;
}


/*
====================
SV_TouchLinks
====================
*/
void SV_TouchLinks( edict_t *ent, areanode_t *node )
{
	link_t		*l, *next;
	int		old_self, old_other;
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE( ent );
	pr_entvars_t	*pevTouch;
	edict_t		*touch;

	// touch linked edicts
	for( l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next )
	{
		next = l->next;
		touch = EDICT_FROM_AREA( l );
		pevTouch = (pr_entvars_t *)GET_PRIVATE( touch );

		if( touch == ent || pevTouch == NULL )
			continue;

		if( !pevTouch->touch || pevTouch->solid != SOLID_TRIGGER )
			continue;

		if( pev->absmin[0] > pevTouch->absmax[0]
		 || pev->absmin[1] > pevTouch->absmax[1]
		 || pev->absmin[2] > pevTouch->absmax[2]
		 || pev->absmax[0] < pevTouch->absmin[0]
		 || pev->absmax[1] < pevTouch->absmin[1]
		 || pev->absmax[2] < pevTouch->absmin[2] )
			continue;

		old_self = pr.global_struct->self;
		old_other = pr.global_struct->other;

		pr.global_struct->self = ENTINDEX( touch );
		pr.global_struct->other = ENTINDEX( ent );
		pr.global_struct->time = gpGlobals->time;

		PR_ExecuteProgram( pevTouch->touch );

		pr.global_struct->self = old_self;
		pr.global_struct->other = old_other;
	}
	
	// recurse down both sides
	if( node->axis == -1 )
		return;
	
	if ( pev->absmax[node->axis] > node->dist )
		SV_TouchLinks ( ent, node->children[0] );
	if ( pev->absmin[node->axis] < node->dist )
		SV_TouchLinks( ent, node->children[1] );
}


void DispatchObjectCollisionBox( edict_t *pent )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pent );

	if( !pent || pent->free || !pent->pvPrivateData )
		HOST_ERROR( "SV_LinkEdict: invalid entity\n" );

	pev->absmin = pev->origin + pev->mins;
	pev->absmax = pev->origin + pev->maxs;

	if (FBitSet( pev->flags, FL_ITEM ))
	{
		// to make items easier to pick up and allow them to be grabbed off
		// of shelves, the abs sizes are expanded
		pev->absmin.x -= 15;
		pev->absmin.y -= 15;
		pev->absmax.x += 15;
		pev->absmax.y += 15;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		pev->absmin.x -= 1;
		pev->absmin.y -= 1;
		pev->absmin.z -= 1;
		pev->absmax.x += 1;
		pev->absmax.y += 1;
		pev->absmax.z += 1;
	}

	// what we needs to correct relink the entity?
	if( pev->modelindex == 0 )//FIXME
		pev->modelindex = pent->v.modelindex;

	if( pev->owner != 0 )
		pent->v.owner = INDEXENT( pev->owner );
	else pent->v.owner = NULL;

	if( pev->groundentity != 0 )
		pent->v.groundentity = INDEXENT( pev->groundentity );
	else pent->v.groundentity = NULL;

	pent->v.absmin = pev->absmin;
	pent->v.absmax = pev->absmax;
	pent->v.movetype = pev->movetype;
	pent->v.solid = pev->solid;
	pent->v.flags = pev->flags;
}

/*
===============
SV_LinkEdict
===============
*/
void SV_LinkEdict( edict_t *pent, BOOL touch_triggers )
{
	if( pent == INDEXENT( 0 ))
		return; // don't add the world

	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pent );

	if( !pent || pent->free || !pent->pvPrivateData )
		HOST_ERROR( "SV_LinkEdict: invalid entity\n" );

	LINK_ENTITY( pent, FALSE );

	// ignore non-solid bodies
	if( pev->solid == SOLID_NOT )
		return;

	// if touch_triggers, touch all entities at this node and decend for more
	if( touch_triggers && !g_fTouchSemaphore )
	{
		g_fTouchSemaphore = TRUE;
		SV_TouchLinks( pent, GET_AREANODE( ));
		g_fTouchSemaphore = FALSE;
	}
}
