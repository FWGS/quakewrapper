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
/*

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "physcallback.h"
#include "com_model.h"
#include "features.h"
#include  "pm_defs.h"
#include "features.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

//
// Xash3D physics interface
//
unsigned int EngineSetFeatures( void )
{
	return ENGINE_QUAKE_COMPATIBLE;
}

//
// attempt to create custom entity when default method is failed
// 0 - attempt to create, -1 - reject to create
//
int DispatchCreateEntity( edict_t *pent, const char *szName )
{
	HOST_ERROR( "pfnCreateEdict: %s called\n", szName );
	return -1;
}

//
// run custom physics for each entity
// return 0 to use built-in engine physic
//
int DispatchPhysicsEntity( edict_t *pEdict )
{
	if( !GET_PRIVATE(pEdict))
		return 0;	// not initialized

	if( RunPhysicsFrame( pEdict ))
	{
		if( !pEdict->free && pEdict->v.flags & FL_KILLME )
			REMOVE_ENTITY( pEdict );
		else ED_UpdateEdictFields( pEdict );
		return 1;
	}

	return 0;	// other movetypes uses built-in engine physic
}

// handle player touching ents
void PM_PlayerTouch( playermove_t *pmove, edict_t *pEntity )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pEntity );

	if( !pEntity || !pmove ) return; // ???

	ED_UpdateProgsFields( pEntity );

	// touch triggers
	SV_LinkEdict( pEntity, TRUE );

	// save original velocity
	vec3_t oldvel = pev->velocity;

	// touch other objects
	for( int i = 0; i < pmove->numtouch; i++ )
	{
		pmtrace_t *pmtrace = &pmove->touchindex[i];
		edict_t *pTouch = INDEXENT( pmove->physents[pmtrace->ent].info );
		pr_entvars_t *pevTouch = (pr_entvars_t *)GET_PRIVATE( pTouch );

		// touch himself?
		if( !pevTouch || pevTouch == pev )
			continue;

		// set momentum velocity to allow player pushing boxes
		pev->velocity = pmtrace->deltavelocity;

		TraceResult tr;

		tr.fAllSolid = pmtrace->allsolid;
		tr.fStartSolid = pmtrace->startsolid;
		tr.fInOpen = pmtrace->inopen;
		tr.fInWater = pmtrace->inwater;
		tr.flFraction = pmtrace->fraction;
		tr.vecEndPos = pmtrace->endpos;
		tr.flPlaneDist = pmtrace->plane.dist;
		tr.vecPlaneNormal = pmtrace->plane.normal;
		tr.iHitgroup = pmtrace->hitgroup;
		tr.pHit = pTouch;

		// IMPORTANT: don't change order!
		SV_Impact( pTouch, pEntity, &tr );

		if( pev->teleport_time && !FBitSet( pev->flags, FL_WATERJUMP ))
			pEntity->v.teleport_time = 0.7; // teleport
	}

	// restore velocity
	pev->velocity = oldvel;

	ED_UpdateEdictFields( pEntity );
}

//
// Quake uses simple trigger touch that approached to bbox
//
int DispatchTriggerTouch( edict_t *pent, edict_t *trigger )
{
	if( trigger == pent || trigger->v.solid != SOLID_TRIGGER ) // disabled ?
		return 0;

	if( pent->v.absmin[0] > trigger->v.absmax[0]
	|| pent->v.absmin[1] > trigger->v.absmax[1]
	|| pent->v.absmin[2] > trigger->v.absmax[2]
	|| pent->v.absmax[0] < trigger->v.absmin[0]
	|| pent->v.absmax[1] < trigger->v.absmin[1]
	|| pent->v.absmax[2] < trigger->v.absmin[2] )
		return 0;

	return 1;
}

//
// Quake hull select order
//
void *DispatchHullForBsp( edict_t *ent, const float *fmins, const float *fmaxs, float *foffset )
{
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE(ent);
	Vector		mins = Vector( (float *)fmins );
	Vector		maxs = Vector( (float *)fmaxs );
	Vector		size, offset;
	model_t		*model;
	hull_t		*hull;

	// decide which clipping hull to use, based on the size
	model = (model_t *)MODEL_HANDLE( ent->v.modelindex );

	if( !model || model->type != mod_brush )
		HOST_ERROR( "Entity %i SOLID_BSP with a non bsp model %i\n", ENTINDEX( ent ), (model) ? model->type : mod_bad );

	size = maxs - mins;

	if( size[0] < 3.0f )
		hull = &model->hulls[0];
	else if( size[0] <= 32.0f )
		hull = &model->hulls[1];
	else hull = &model->hulls[2];

	offset = hull->clip_mins - mins;
	offset += pev->origin;

	offset.CopyToArray( foffset );

	return hull;
}
	
static physics_interface_t gPhysicsInterface = 
{
	SV_PHYSICS_INTERFACE_VERSION,
	DispatchCreateEntity,
	DispatchPhysicsEntity,
	ED_LoadFromFile,		// SV_LoadEntities
	NULL,			// SV_UpdatePlayerBaseVelocity
	NULL,			// SV_AllowSaveGame
	DispatchTriggerTouch,	
	EngineSetFeatures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	EndFrame,
	NULL,
	DispatchCreateEntitiesInRestoreList,
	NULL,
	NULL,
	NULL,
	NULL,
	PM_PlayerTouch,
	NULL,
	DispatchHullForBsp,
	SV_PlayerRunThink,
};

int Server_GetPhysicsInterface( int iVersion, server_physics_api_t *pfuncsFromEngine, physics_interface_t *pFunctionTable )
{
	if ( !pFunctionTable || !pfuncsFromEngine || iVersion != SV_PHYSICS_INTERFACE_VERSION )
	{
		return FALSE;
	}

	size_t iExportSize = sizeof( server_physics_api_t );
	size_t iImportSize = sizeof( physics_interface_t );

	// NOTE: the very old versions NOT have information about current build in any case
	if( g_iXashEngineBuildNumber <= 1910 )
	{
		if( g_fXashEngine )
			ALERT( at_console, "old version of Xash3D was detected. Engine features was disabled.\n" );

		// interface sizes for build 1905 and older
		iExportSize = 28;
		iImportSize = 24;
	}

	if( g_iXashEngineBuildNumber <= 3700 )
		iImportSize -= 12;

	// copy new physics interface
	memcpy( &g_physfuncs, pfuncsFromEngine, iExportSize );

	// fill engine callbacks
	memcpy( pFunctionTable, &gPhysicsInterface, iImportSize );

	return TRUE;
}