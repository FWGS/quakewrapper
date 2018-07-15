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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to 
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "saverestore.h"
#include "client.h"
#include "game.h"
#include "customentity.h"
#include "usercmd.h"
#include "netadr.h"

int g_framecount = 0;

/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{	
	if( !pr.progs )
		return FALSE;

	int index = ENTINDEX(pEntity) - 1;

	if( !pr.changelevel[index] )
	{
		// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram (pr.global_struct->SetNewParms);
		for( int i = 0; i < NUM_SPAWN_PARMS; i++ )
			pr.spawn_parms[index][i] = (&pr.global_struct->parm1)[i];
	}

	pr.client_cache[index].server_hud_initialized = false;
	pr.changelevel[index] = false;

	return TRUE;
}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect( edict_t *pEntity )
{
	if (pEntity && pEntity->pvPrivateData)
	{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
		int saveSelf = pr.global_struct->self;
		pr.global_struct->self = ENTINDEX(pEntity);
		PR_ExecuteProgram (pr.global_struct->ClientDisconnect);
		pr.global_struct->self = saveSelf;
		ED_UpdateEdictFields( pEntity );
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEntity )
{
	if (!pr.progs) return;
	
	pr.global_struct->time = gpGlobals->time;
	pr.global_struct->self = ENTINDEX(pEntity);

	if( pEntity->free || !pEntity->pvPrivateData )
		return;

	PR_ExecuteProgram (pr.global_struct->ClientKill);
	ED_UpdateEdictFields( pEntity );
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer( edict_t *pEntity )
{
	if (!pEntity->pvPrivateData)
		ALLOC_PRIVATE( pEntity, pr.progs->entityfields * 4 );

	int index = ENTINDEX( pEntity ) - 1;

	for( int i = 0; i < NUM_SPAWN_PARMS; i++ )
		(&pr.global_struct->parm1)[i] = pr.spawn_parms[index][i];
	pr.global_struct->time = gpGlobals->time;
	pr.global_struct->self = ENTINDEX(pEntity);
	PR_ExecuteProgram (pr.global_struct->ClientConnect);

	PR_ExecuteProgram (pr.global_struct->PutClientInServer);

	ED_UpdateEdictFields( pEntity );
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEntity, int teamonly )
{
/*
	CBasePlayer *client;
	int		j;
	char	*p;
	char	text[128];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);

	// We can get a raw string now, without the "say " prepended
	if ( CMD_ARGC() == 0 )
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer* player = GetClassPtr((CBasePlayer *)pev);

	//Not yet.
	if ( player->m_flNextChatTime > gpGlobals->time )
		 return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( CMD_ARGC() >= 2 )
		{
			p = (char *)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( CMD_ARGC() >= 2 )
		{
			sprintf( szTemp, "%s %s", ( char * )pcmd, (char *)CMD_ARGS() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf( szTemp, "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// make sure the text has content
	for ( char *pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if ( !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if ( pc != NULL )
		return;  // no character found, so say nothing

// turn on color set 2  (color on,  no sound)
	if ( teamonly )
		sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
	else
		sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );


	player->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client->pev )
			continue;
		
		if ( client->edict() == pEntity )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, client->pev );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, &pEntity->v );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );

	char * temp;
	if ( teamonly )
		temp = "say_team";
	else
		temp = "say";
	
	// team match?
	if ( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", 
			STRING( pEntity->v.netname ), 
			GETPLAYERUSERID( pEntity ),
			GETPLAYERAUTHID( pEntity ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model" ),
			temp,
			p );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n", 
			STRING( pEntity->v.netname ), 
			GETPLAYERUSERID( pEntity ),
			GETPLAYERAUTHID( pEntity ),
			GETPLAYERUSERID( pEntity ),
			temp,
			p );
	}
*/
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;

// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand( edict_t *pEntity )
{
	const char *pcmd = CMD_ARGV(0);

	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	if ( FStrEq(pcmd, "fog" ) )
	{
		float fog_settings[4];
		int packed_fog[4];
		edict_t *world = INDEXENT( 0 );
		pr_entvars_t *pevWorld = (pr_entvars_t *)GET_PRIVATE( world );

		fog_settings[0] = atof( CMD_ARGV( 1 ));	// density
		fog_settings[1] = atof( CMD_ARGV( 2 ));	// red
		fog_settings[2] = atof( CMD_ARGV( 3 ));	// green
		fog_settings[3] = atof( CMD_ARGV( 4 ));	// blue

//		ALERT( at_console, "fog: %g %g %g, density %g\n", fog_settings[1], fog_settings[2], fog_settings[3], fog_settings[0] );

		for( int i = 0; i < 4; i++)
			packed_fog[i] = fog_settings[i] * 255;

		if( g_framecount > 10 )
		{
			// temporare place for store fog settings
			world->v.impulse = (packed_fog[1]<<24)|(packed_fog[2]<<16)|(packed_fog[3]<<8)|packed_fog[0];
			*(int *)&pevWorld->impulse = world->v.impulse;
			UPDATE_PACKED_FOG( world->v.impulse );
		}
	}
/*
	else if ( FStrEq(pcmd, "say" ) )
	{
		Host_Say( pEntity, 0 );
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "fly" ) )
	{
		if (pev->movetype != MOVETYPE_FLY)
		{
			pev->movetype = MOVETYPE_FLY;
			CLIENT_PRINTF( pEntity, print_console, "flymode ON\n" );
		}
		else
		{
			pev->movetype = MOVETYPE_WALK;
			CLIENT_PRINTF( pEntity, print_console, "flymode OFF\n" );
		}
	}
	else
	{
		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy( command, pcmd, 127 );
		command[127] = '\0';

		// tell the user they entered an unknown command
		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", command ) );
	}
*/
}

/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		char sName[256];
		char *pName = g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );
		strncpy( sName, pName, sizeof(sName) - 1 );
		sName[ sizeof(sName) - 1 ] = '\0';

		// First parse the name and remove any %'s
		for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
		{
			// Replace it with a space
			if ( *pApersand == '%' )
				*pApersand = ' ';
		}

		// Set the name
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX(pEntity), infobuffer, "name", sName );

		char text[256];
		sprintf( text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

		// team match?
		if ( gpGlobals->teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", 
				STRING( pEntity->v.netname ), 
				GETPLAYERUSERID( pEntity ), 
				GETPLAYERAUTHID( pEntity ),
				g_engfuncs.pfnInfoKeyValue( infobuffer, "model" ), 
				g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" changed name to \"%s\"\n", 
				STRING( pEntity->v.netname ), 
				GETPLAYERUSERID( pEntity ), 
				GETPLAYERAUTHID( pEntity ),
				GETPLAYERUSERID( pEntity ), 
				g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		}
	}

//	g_pGameRules->ClientUserInfoChanged( GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer );
}

static int g_serveractive = 0;

void ServerDeactivate( void )
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate 
	if ( g_serveractive != 1 )
	{
		return;
	}

	g_fTouchSemaphore = FALSE;
	g_serveractive = 0;
	g_framecount = 0;

	// Peform any shutdown operations here...
	GameResetFog();
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_fTouchSemaphore = FALSE;
	g_serveractive = 1;
	g_framecount = 0;

	if( pr.pfnRestoreGame && pr.loadgame )
	{
		ALERT( at_console, "Loading enhanced game - RestoreGame()\n" );
		pr.global_struct->time = PHYSICS_TIME();
		pr.global_struct->self = 1;	// probably no matter
		PR_ExecuteProgram( pr.pfnRestoreGame );
		pr.loadgame = false;
	}

	// Clients have not been initialized yet
	for ( int i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free || !pEdictList[i].pvPrivateData )
			continue;

		ED_UpdateEdictFields( &pEdictList[i] );		
	}
}

void UpdateClientData( edict_t *pEntity )
{
	int index = ENTINDEX( pEntity ) - 1;
	pr_client_cache_t *pcache = &pr.client_cache[index];
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pEntity );
	int items;

	if( !pcache->server_hud_initialized )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, pEntity );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		pr_entvars_t *world = (pr_entvars_t *)GET_PRIVATE( INDEXENT( 0 ));

		// send levelname to client
		MESSAGE_BEGIN( MSG_ONE, gmsgLevelName, pEntity );
			WRITE_STRING( STRING( world->message ));
		MESSAGE_END();

		// send total secrets count
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_TOTALSECRETS );
			WRITE_LONG( pr.global_struct->total_secrets );
		MESSAGE_END();

		// send found secrets count (save\restore)
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_SECRETS );
			WRITE_LONG( pr.global_struct->found_secrets );
		MESSAGE_END();

		// send total monsters count
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_TOTALMONSTERS );
			WRITE_LONG( pr.global_struct->total_monsters );
		MESSAGE_END();

		// send killed monsters count (save\restore)
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_MONSTERS );
			WRITE_LONG( pr.global_struct->killed_monsters );
		MESSAGE_END();

		if ( !pcache->client_hud_initialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, pEntity );
			MESSAGE_END();

			pcache->client_hud_initialized = true;
		}

		pcache->server_hud_initialized = true;
		pcache->frags = -1.0f;
	}

	if (pev->health != pcache->health)
	{
		int iHealth = max( pev->health, 0 );  // make sure that no negative health values are sent

		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_HEALTH );
			WRITE_LONG( iHealth );
		MESSAGE_END();

		pcache->health = pev->health;
	}

	eval_t *val = GETEDICTFIELDVALUE( pEntity, pr.eval_items2 );

	if( val ) items = (int)pev->items | ((int)val->value << 23);
	else items = (int)pev->items | ((int)pr.global_struct->serverflags << 28); // store runes as high bits

	if (items != pcache->items )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItems, pEntity );
			WRITE_LONG( items );
		MESSAGE_END();
		pcache->items = items;
	}

	if (pev->weapon != pcache->weapon)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_ACTIVEWEAPON );
			WRITE_LONG( pev->weapon );
		MESSAGE_END();
		pcache->weapon = pev->weapon;
	}

	if (pev->weaponframe != pcache->weaponframe)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_WEAPONFRAME );
			WRITE_LONG( pev->weaponframe );
		MESSAGE_END();
		pcache->weaponframe = pev->weaponframe;
	}

	if (pev->currentammo != pcache->currentammo)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_AMMO );
			WRITE_LONG( pev->currentammo );
		MESSAGE_END();
		pcache->currentammo = pev->currentammo;
	}

	if (pev->ammo_shells != pcache->ammo_shells)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_SHELLS );
			WRITE_LONG( pev->ammo_shells );
		MESSAGE_END();
		pcache->ammo_shells = pev->ammo_shells;
	}

	if (pev->ammo_nails != pcache->ammo_nails)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_NAILS );
			WRITE_LONG( pev->ammo_nails );
		MESSAGE_END();
		pcache->ammo_nails = pev->ammo_nails;
	}

	if (pev->ammo_rockets != pcache->ammo_rockets)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_ROCKETS );
			WRITE_LONG( pev->ammo_rockets );
		MESSAGE_END();
		pcache->ammo_rockets = pev->ammo_rockets;
	}

	if (pev->ammo_cells != pcache->ammo_cells)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_CELLS );
			WRITE_LONG( pev->ammo_cells );
		MESSAGE_END();
		pcache->ammo_cells = pev->ammo_cells;
	}

	if (pev->armorvalue != pcache->armorvalue)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStats, pEntity );
			WRITE_BYTE( STAT_ARMOR );
			WRITE_LONG( (int)pev->armorvalue );
		MESSAGE_END();
		pcache->armorvalue = pev->armorvalue;
	}

	if (pev && (pev->dmg_take || pev->dmg_save))
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = INDEXENT( pev->dmg_inflictor );
		if( !other ) other = INDEXENT( 0 ); // ????

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, pEntity );
			WRITE_BYTE( pev->dmg_save );
			WRITE_BYTE( pev->dmg_take );
			WRITE_COORD( other->v.origin[0] + 0.5f * (other->v.mins[0] + other->v.maxs[0] ));
			WRITE_COORD( other->v.origin[1] + 0.5f * (other->v.mins[1] + other->v.maxs[1] ));
			WRITE_COORD( other->v.origin[2] + 0.5f * (other->v.mins[2] + other->v.maxs[2] ));
		MESSAGE_END();
	
		pev->dmg_take = 0;
		pev->dmg_save = 0;
	}

	if( gpGlobals->maxClients > 1 && pev->frags != pcache->frags )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_SHORT( pev->frags );
			WRITE_SHORT( 0 );
			WRITE_SHORT( 0 );
			WRITE_SHORT( 0 );
		MESSAGE_END();
		pcache->frags = pev->frags;
	}
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink( edict_t *pEntity )
{
	if (!pr.progs || !GET_PRIVATE( pEntity ))
		return;

	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pEntity );

	// call standard client pre-think
	pr.global_struct->time = gpGlobals->time;
	pr.global_struct->frametime = gpGlobals->frametime;
	pr.global_struct->self = ENTINDEX(pEntity);

	// copy buttons
	pev->button0 = FBitSet( pEntity->v.button, IN_ATTACK ) ? true : false;
//	pev->button1 = FBitSet( pEntity->v.button, IN_USE ) ? true : false;
	pev->button2 = FBitSet( pEntity->v.button, IN_JUMP ) ? true : false;

	// copy upcoming settings
	ED_UpdateProgsFields( pEntity );
	PR_ExecuteProgram (pr.global_struct->PlayerPreThink);
	ED_UpdateEdictFields( pEntity );

	UpdateClientData( pEntity );
}

void ImpulseCommands( edict_t *pEntity )
{
	TraceResult tr;
	edict_t *pWorld = INDEXENT( 0 );

	switch( pEntity->v.impulse )
	{
	case 107:
		Vector start = pEntity->v.origin + pEntity->v.view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		TRACE_LINE( start, end, TRUE, pEntity, &tr );
		if( tr.pHit ) pWorld = tr.pHit;
		const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
		if ( pTextureName )
			ALERT( at_console, "Texture: %s\n", pTextureName );
		break;
	}
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t *pEntity )
{
	if (!pr.progs || !GET_PRIVATE( pEntity ))
		return;

	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pEntity );

	// call standard client post-think
	pr.global_struct->time = gpGlobals->time;
	pr.global_struct->frametime = gpGlobals->frametime;
	pr.global_struct->self = ENTINDEX(pEntity);
	ImpulseCommands( pEntity );

	// catch changes after pmove
	ED_UpdateProgsFields( pEntity );
	PR_ExecuteProgram (pr.global_struct->PlayerPostThink);
	ED_UpdateEdictFields( pEntity );
}

void ParmsNewLevel( void )
{
}


void ParmsChangeLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if ( pSaveData ) pSaveData->connectionCount = 0;
}

//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame( void )
{
	if( g_psv_teamplay )
		gpGlobals->teamplay = g_psv_teamplay->value;

	pr.global_struct->deathmatch = gpGlobals->deathmatch;
	pr.global_struct->coop = gpGlobals->coop;
	pr.global_struct->teamplay = gpGlobals->teamplay;
	pr.global_struct->frametime = gpGlobals->frametime;
	pr.global_struct->time = gpGlobals->time;

	// let the progs know that a new frame has started
	pr.global_struct->self = 0;
	pr.global_struct->other = 0;
	pr.global_struct->time = gpGlobals->time;
	PR_ExecuteProgram (pr.global_struct->StartFrame);

	g_framecount++;
}

void EndFrame( void )
{
	if( pr.global_struct->force_retouch != 0.0f )
		pr.global_struct->force_retouch--;

	// catch changes for players
	for( int i = 0; i < gpGlobals->maxClients; i++ )
	{
		edict_t *pPlayer = INDEXENT( i + 1 );
		if ( !pPlayer || pPlayer->free || !pPlayer->pvPrivateData )
			continue;

		ED_UpdateEdictFields( pPlayer );	
	}

	// check fog changes
	if( gl_fogenable.value )
	{
		int	packed_fog[4], impulse;

		packed_fog[0] = ((gl_fogdensity.value / 100.0f) * 64.0f) * 255;
		packed_fog[1] = (gl_fogred.value) * 255;
		packed_fog[2] = (gl_foggreen.value) * 255;
		packed_fog[3] = (gl_fogblue.value) * 255;

		// temporare place for store fog settings
		impulse = (packed_fog[1]<<24)|(packed_fog[2]<<16)|(packed_fog[3]<<8)|packed_fog[0];

		if( pr.cached_fog != impulse )
		{
			UPDATE_PACKED_FOG( impulse );
			pr.cached_fog = impulse;
		}
	}
	else if( pr.cached_fog != 0 )
	{
		UPDATE_PACKED_FOG( 0 );
		pr.cached_fog = 0;
	}
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	return "Quake Wrapper v0.56 beta";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char *error_string )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t *pEntity )
{
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t *pEntity )
{
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t *pEntity )
{
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if ( pViewEntity )
	{
		pView = pViewEntity;
	}

	if ( pClient->v.flags & FL_PROXY )
	{
		*pvs = NULL;	// the spectator proxy sees
		*pas = NULL;	// and hears everything
		return;
	}

	org = pView->v.origin + pView->v.view_ofs;

	*pvs = ENGINE_SET_PVS ( (float *)&org );
	*pas = ENGINE_SET_PAS ( (float *)&org );
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
	pr_entvars_t *ev;
	int	i;

	if (!ED_UpdateEdictFields( ent ))
		return 0;

	ev = (pr_entvars_t *)GET_PRIVATE( ent );
	if( ev ) ClearBits( ev->effects, EF_MUZZLEFLASH );

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ( ( ent->v.effects == EF_NODRAW ) &&
		 ( ent != host ) )
		return 0;

	// Ignore ents without valid / visible models
	// NOTE: progs always set null string as "" not 0
	if ( !ent->v.modelindex || !*STRING( ent->v.model ) )
		return 0;

	// Don't send spectators to other players
	if ( ( ent->v.flags & FL_SPECTATOR ) && ( ent != host ) )
	{
		return 0;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host )
	{
		if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *)ent, pSet ) )
		{
			return 0;
		}
	}

	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number	  = e;
	state->entityType = ENTITY_NORMAL;
	
	// Flag custom entities.
	if ( ent->v.flags & FL_CUSTOMENTITY )
	{
		state->entityType = ENTITY_BEAM;
	}

	// 
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime   = (int)(1000.0 * ent->v.animtime ) / 1000.0;

	memcpy( state->origin, ent->v.origin, 3 * sizeof( float ) );
	memcpy( state->angles, ent->v.angles, 3 * sizeof( float ) );
	memcpy( state->mins, ent->v.mins, 3 * sizeof( float ) );
	memcpy( state->maxs, ent->v.maxs, 3 * sizeof( float ) );

	memcpy( state->startpos, ent->v.startpos, 3 * sizeof( float ) );
	memcpy( state->endpos, ent->v.endpos, 3 * sizeof( float ) );
	memcpy( state->velocity, ent->v.velocity, 3 * sizeof( float ) );

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;
		
	state->frame      = ent->v.frame;

	state->skin       = ent->v.skin;
	state->effects    = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if ( !player &&
		 ent->v.animtime &&
		 ent->v.velocity[ 0 ] == 0 && 
		 ent->v.velocity[ 1 ] == 0 && 
		 ent->v.velocity[ 2 ] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale	  = ent->v.scale;
	state->solid	  = ent->v.solid;
	state->colormap   = ent->v.colormap;

	state->movetype   = ent->v.movetype;
	state->sequence   = ent->v.sequence;
	state->framerate  = ent->v.framerate;
	state->body       = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i]   = ent->v.blending[i];
	}

	state->rendermode    = ent->v.rendermode;
	state->renderamt     = ent->v.renderamt; 
	state->renderfx      = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor.x;
	state->rendercolor.g = ent->v.rendercolor.y;
	state->rendercolor.b = ent->v.rendercolor.z;

	state->aiment = 0;
	if ( ent->v.aiment )
	{
		state->aiment = ENTINDEX( ent->v.aiment );
	}

	state->owner = 0;
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );
		
		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
		{
			state->owner = owner;	
		}
	}

	state->onground = 0;
	if ( ent->v.groundentity )
	{
		state->onground = ENTINDEX( ent->v.groundentity );
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if ( !player )
	{
		state->playerclass  = ent->v.playerclass;
	}

	memcpy( state->vuser1, ent->v.vuser1, 3 * sizeof( float ) );

	state->weaponmodel  = MODEL_INDEX( STRING( ent->v.weaponmodel ) );

	// Special stuff for players only
	if ( player )
	{
		memcpy( state->basevelocity, ent->v.basevelocity, 3 * sizeof( float ) );

		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction     = ent->v.friction;

		state->gravity      = ent->v.gravity;
//		state->team	= ent->v.team;
//		
		state->usehull      = 0;
		state->health	= ent->v.health;
	}

	return 1;
}

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin		= entity->v.origin;
	baseline->angles		= entity->v.angles;
	baseline->frame			= entity->v.frame;
	baseline->skin			= (short)entity->v.skin;

	// render information
	baseline->rendermode	= (byte)entity->v.rendermode;
	baseline->renderamt		= (byte)entity->v.renderamt;
	baseline->rendercolor.r	= (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g	= (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b	= (byte)entity->v.rendercolor.z;
	baseline->renderfx		= (byte)entity->v.renderfx;

	if ( player )
	{
		baseline->mins			= player_mins;
		baseline->maxs			= player_maxs;

		baseline->colormap		= eindex;
		baseline->modelindex	= playermodelindex;
		baseline->friction		= 1.0;
		baseline->movetype		= MOVETYPE_WALK;

		baseline->scale			= entity->v.scale;
		baseline->solid			= SOLID_SLIDEBOX;
		baseline->framerate		= 1.0;
		baseline->gravity		= 1.0;

	}
	else
	{
		baseline->mins			= entity->v.mins;
		baseline->maxs			= entity->v.maxs;

		baseline->colormap		= 0;
		baseline->modelindex	= entity->v.modelindex;//SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype		= entity->v.movetype;

		baseline->scale			= entity->v.scale;
		baseline->solid			= entity->v.solid;
		baseline->framerate		= entity->v.framerate;
		baseline->gravity		= entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int	 field;
} entity_field_alias_t;

#define FIELD_ORIGIN0			0
#define FIELD_ORIGIN1			1
#define FIELD_ORIGIN2			2
#define FIELD_ANGLES0			3
#define FIELD_ANGLES1			4
#define FIELD_ANGLES2			5

static entity_field_alias_t entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
};

void Entity_FieldInit( struct delta_s *pFields )
{
	entity_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN0 ].name );
	entity_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN1 ].name );
	entity_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN2 ].name );
	entity_field_alias[ FIELD_ANGLES0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES0 ].name );
	entity_field_alias[ FIELD_ANGLES1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES1 ].name );
	entity_field_alias[ FIELD_ANGLES2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES2 ].name );
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->impacttime != 0 ) && ( t->starttime != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );

		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

static entity_field_alias_t player_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
};

void Player_FieldInit( struct delta_s *pFields )
{
	player_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN0 ].name );
	player_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN1 ].name );
	player_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN2 ].name );
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, player_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

#define CUSTOMFIELD_ORIGIN0			0
#define CUSTOMFIELD_ORIGIN1			1
#define CUSTOMFIELD_ORIGIN2			2
#define CUSTOMFIELD_ANGLES0			3
#define CUSTOMFIELD_ANGLES1			4
#define CUSTOMFIELD_ANGLES2			5
#define CUSTOMFIELD_SKIN			6
#define CUSTOMFIELD_SEQUENCE		7
#define CUSTOMFIELD_ANIMTIME		8

entity_field_alias_t custom_entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
	{ "skin",				0 },
	{ "sequence",			0 },
	{ "animtime",			0 },
};

void Custom_Entity_FieldInit( struct delta_s *pFields )
{
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	beamType = t->rendermode & 0x0f;
		
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field );
	}

	if ( beamType != BEAM_POINTS )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field );
	}

	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ( (int)f->animtime == (int)t->animtime )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field );
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders( void )
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
	memset( info, 0, 32 * sizeof( weapon_data_t ) );
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData ( edict_t *ent, int sendweapons, struct clientdata_s *cd )
{
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );

	pev->fixangle = ent->v.fixangle;
	pev->avelocity.y = ent->v.avelocity.y;

	cd->flags			= ent->v.flags;
	cd->health		= ent->v.health;

	cd->viewmodel		= MODEL_INDEX( STRING( ent->v.weaponmodel ) );

	cd->waterlevel		= ent->v.waterlevel;
	cd->watertype		= ent->v.watertype;
	cd->weapons		= ent->v.weapons;

	// Vectors
	cd->origin			= ent->v.origin;
	cd->velocity		= ent->v.velocity;
	cd->view_ofs		= ent->v.view_ofs;
	cd->punchangle		= ent->v.punchangle;

	cd->bInDuck			= ent->v.bInDuck;
	cd->flTimeStepSound = ent->v.flTimeStepSound;
	cd->flDuckTime		= ent->v.flDuckTime;
	cd->flSwimTime		= ent->v.flSwimTime;
	cd->waterjumptime	= ent->v.teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed		= ent->v.maxspeed;
	cd->fov				= ent->v.fov;
	cd->weaponanim		= ent->v.weaponanim;

	cd->pushmsec		= ent->v.pushmsec;
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd ( const edict_t *player )
{
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		VEC_HULL_MIN.CopyToArray( mins );
		VEC_HULL_MAX.CopyToArray( maxs );
		iret = 1;
		break;
	case 1:
	case 2:				// Point based hull
		Vector( 0, 0, 0 ).CopyToArray( mins );
		Vector( 0, 0, 0 ).CopyToArray( maxs );
		iret = 1;
		break;
	case 3:				// Large hull
		VEC_LARGE_HULL_MIN.CopyToArray( mins );
		VEC_LARGE_HULL_MAX.CopyToArray( maxs );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines ( void )
{
	int iret = 0;
	entity_state_t state;

	memset( &state, 0, sizeof( state ) );

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int	InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
	// Server doesn't care?
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;

	// Default behavior is to kick the player
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation( void )
{
	return 1;
}

/*
================================
ShouldCollide

  Called when the engine believes two entities are about to collide. Return 0 if you
  want the two entities to just pass through each other without colliding or calling the
  touch function.
================================
*/
int ShouldCollide( edict_t *pentTouched, edict_t *pentOther )
{
//	ED_UpdateEdictFields( pentTouched );
//	ED_UpdateEdictFields( pentOther );

	return 1;
}