/*
pr_save.cpp - Quake virtual machine wrapper impmenentation
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
#include "skill.h"
#include "game.h"
#include "progs.h"

// model precache table stashed between RestoreGlobalState and DispatchCreateEntitiesInRestoreList, see comments in SaveGlobalState
#define SAVE_MAX_MODELS	4096	// engine MAX_MODELS
static char *g_savedModelNames[SAVE_MAX_MODELS];
static int g_savedModelCount = 0;

typedef struct
{
	int	modelCount;
} modeltableheader_t;

typedef struct
{
	char	name[64];	// MAX_QPATH
} modeltableentry_t;

static TYPEDESCRIPTION gModelTableHeader[] =
{
	DEFINE_FIELD( modeltableheader_t, modelCount, FIELD_INTEGER ),
};

static TYPEDESCRIPTION gModelTableEntry[] =
{
	DEFINE_ARRAY( modeltableentry_t, name, FIELD_CHARACTER, 64 ),
};

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pent);
	
	if ( pev && pSaveData )
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[ pSaveData->currentIndex ];

		if( FClassnameIs( pent, "bodyque" ))
			return;	// never saving the bodyques!

		if( FClassnameIs( pent, "particle" ))
			return;	// never saving the particles!

		eval_t *val = GETEDICTFIELDVALUE( pent, pr.eval_classtype );
		if( val && val->value == 42.0f ) return;

		if ( pTable->pent != pent )
			ALERT( at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n" );

		pTable->location = pSaveData->size;		// Remember entity position for file I/O
		pTable->classname = pev->classname;		// Remember entity class for respawn

		CSave saveHelper( pSaveData );
		saveHelper.WriteProgFields( "PROGS", pev, pr.fielddefs, pr.progs->numfielddefs );

		pTable->size = pSaveData->size - pTable->location;	// Size of entity block is data size written to block
	}
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(pent);

	if ( pev && pSaveData )
	{
		CRestore restoreHelper( pSaveData );
		// overwrite the fields from saved state
		restoreHelper.ReadProgFields( "PROGS", pev, pr.fielddefs, pr.progs->numfielddefs );
		pent->v.classname = pev->classname; // Share this out
	
		// restore viewangles for client
		if( FBitSet( pev->flags, FL_CLIENT ))
		{
			pev->v_angle.z = 0;	// Clear out roll
			pev->angles = pev->v_angle;
			pev->fixangle = TRUE;           // turn this way immediately
		}

		// spawn entity to precache sounds, etc
		ED_PrecacheEdict( pent, pev );

		SV_LinkEdict( pent, FALSE );
		ED_UpdateEdictFields( pent );

		if( FNullEnt( pent ))
		{
			// NOTE: restore packed fog as integer
			pent->v.impulse = *(int *)&pev->impulse;

			if( pent->v.impulse != 0 )
				UPDATE_PACKED_FOG( pent->v.impulse );
		}
	}

	return 0;
}

void DispatchCreateEntitiesInRestoreList( SAVERESTOREDATA *pSaveData, int levelMask, qboolean create_world )
{
	ENTITYTABLE *pTable;
	edict_t *pent;

	// rebuild the model precache table in the saved order, so raw model
	// indices stored in progs fields stay valid after loading
	if( create_world && g_savedModelCount > 0 )
	{
		for( int i = 0; i < g_savedModelCount; i++ )
		{
			const char *name = g_savedModelNames[i];

			// inline models are precached with the world and
			// already occupy their original slots
			if( !name[0] || name[0] == '*' )
				continue;

			int idx = PRECACHE_MODEL( STRING( ALLOC_STRING( name )));

			if( idx != i + 1 )
				ALERT( at_warning, "%s: model %s expected index %i, got %i\n", __func__, name, i + 1, idx );
		}

		for( int i = 0; i < g_savedModelCount; i++ )
			free( g_savedModelNames[i] );
		g_savedModelCount = 0;
	}

	// create entity list
	for( int i = 0; i < pSaveData->tableCount; i++ )
	{
		pTable = &pSaveData->pTable[i];
		pent = NULL;

		if( pTable->classname != iStringNull && pTable->size && ( !FBitSet( pTable->flags, FENTTABLE_REMOVED ) || !create_world ))
		{
			int	active = FBitSet( pTable->flags, levelMask ) ? 1 : 0;

			if( create_world )
				active = 1;

			if( pTable->id == 0 && create_world ) // worldspawn
			{
				pent = INDEXENT( pTable->id );

				pr_entvars_t *pev = (pr_entvars_t *)ALLOC_PRIVATE( pent, pr.progs->entityfields * 4 );
				pev->classname = pTable->classname;

				// SV_InitEdict
				memset( &pent->v, 0, sizeof( entvars_t ));
				pent->v.pContainingEntity = pent;
				pent->free = false;
				pr.loadgame = true;
			}
			else if(( pTable->id > 0 ) && ( pTable->id <= gpGlobals->maxClients ))
			{
				if( !FBitSet( pTable->flags, FENTTABLE_PLAYER ))
					ALERT( at_warning, "ENTITY IS NOT A PLAYER: %d\n", i );

				edict_t *ed = INDEXENT( pTable->id );

				// create the player
				if( active && ed != NULL )
				{
					// create the player
					pr_entvars_t *pev = (pr_entvars_t *)ALLOC_PRIVATE( ed, pr.progs->entityfields * 4 );
					pev->classname = pTable->classname;
					pent = ed;
				}
			}
			else if( active )
			{
				pent = CREATE_ENTITY();

				// progs have constant class size for each entity
				pr_entvars_t *pev = (pr_entvars_t *)ALLOC_PRIVATE( pent, pr.progs->entityfields * 4 );
				pev->classname = pTable->classname;
			}
		}

		pTable->pent = pent;
	}
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pFields, fieldCount );
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pFields, fieldCount );
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteGlobalFields( "GLOBALS", pr.globals, pr.globaldefs, pr.progs->numglobaldefs );

	// progs can store raw model indices in custom entity fields,
	// for example, rerelease infected monsters keep one in modelindex_transformed
	// so the model precache table must be rebuilt in exactly the same order
	// on restore for such indices to stay valid
	modeltableheader_t hdr = { 0 };

	for( int i = 1; i < SAVE_MAX_MODELS; i++ )
	{
		const char *name = MODEL_NAME( i );

		if( !name || !name[0] )
			break;

		hdr.modelCount = i;
	}

	saveHelper.WriteFields( "MODELTABLE", &hdr, gModelTableHeader, ARRAYSIZE( gModelTableHeader ));

	for( int i = 1; i <= hdr.modelCount; i++ )
	{
		modeltableentry_t entry;

		Q_strncpy( entry.name, MODEL_NAME( i ), sizeof( entry.name ));
		saveHelper.WriteFields( "MODELNAME", &entry, gModelTableEntry, ARRAYSIZE( gModelTableEntry ));
	}
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
	pr.global_struct->total_secrets = 0;
	pr.global_struct->found_secrets = 0;
	pr.global_struct->total_monsters = 0;
	pr.global_struct->killed_monsters = 0;

	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadGlobalFields( "GLOBALS", pr.globals, pr.globaldefs, pr.progs->numglobaldefs );

	// QUOTH issues. Reset the framecount to properly precache
	eval_t *val = GETEGLOBALVALUE( pr.glob_framecount );
	if( val ) val->value = 0.0f;

	// stash the saved model table. it can't be precached right here,
	// the server is not spawned yet: this is done later in
	// DispatchCreateEntitiesInRestoreList
	for( int i = 0; i < g_savedModelCount; i++ )
		free( g_savedModelNames[i] );
	g_savedModelCount = 0;

	modeltableheader_t hdr = { 0 };

	if( !restoreHelper.Empty() && restoreHelper.ReadFields( "MODELTABLE", &hdr, gModelTableHeader, ARRAYSIZE( gModelTableHeader )))
	{
		hdr.modelCount = bound( 0, hdr.modelCount, SAVE_MAX_MODELS - 1 );

		for( int i = 0; i < hdr.modelCount; i++ )
		{
			modeltableentry_t entry = { 0 };

			restoreHelper.ReadFields( "MODELNAME", &entry, gModelTableEntry, ARRAYSIZE( gModelTableEntry ));
			entry.name[sizeof( entry.name ) - 1] = '\0';
			g_savedModelNames[g_savedModelCount++] = strdup( entry.name );
		}
	}
}

void ResetGlobalState( void )
{
	PR_ResetGlobalState();
}