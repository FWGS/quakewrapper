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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "game.h"
#include "shake.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

int gmsgShake = 0;
int gmsgFade = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgDamage = 0;
int gmsgHudText = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgHideHUD = 0;
int gmsgTempEntity = 0;
int gmsgLevelName = 0;
int gmsgStats = 0;		// generic stats message
int gmsgItems = 0;
int gmsgFoundSecret = 0;
int gmsgKilledMonster = 0;
int gmsgShowLMP;
int gmsgHideLMP;

cvar_t	noexit = { "noexit", "0", FCVAR_SERVER };
cvar_t	samelevel = { "samelevel", "0", FCVAR_SERVER };
cvar_t	pr_checkextension = { "pr_checkextension", "1" };			// a part of extension system
cvar_t	nospr32 = { "nospr32", "0", FCVAR_ARCHIVE };

// multiplayer server rules
cvar_t	fragsleft	= {"mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	// Don't spam console/log files/users with this changing
cvar_t	timeleft	= {"mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	// "      "

// multiplayer server rules
cvar_t	*g_psv_teamplay;
cvar_t	fraglimit	= {"fraglimit","0", FCVAR_SERVER };
cvar_t	timelimit	= {"timelimit","0", FCVAR_SERVER };
cvar_t	friendlyfire= {"mp_friendlyfire","0", FCVAR_SERVER };
cvar_t	forcerespawn= {"mp_forcerespawn","1", FCVAR_SERVER };
cvar_t	aimcrosshair= {"mp_autocrosshair","1", FCVAR_SERVER };
cvar_t	decalfrequency = {"decalfrequency","30", FCVAR_SERVER };
cvar_t	teamlist = {"mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t	teamoverride = {"mp_teamoverride","1" };
cvar_t	defaultteam = {"mp_defaultteam","0" };
cvar_t	allowmonsters={"mp_allowmonsters","0", FCVAR_SERVER };

cvar_t  mp_chattime = {"mp_chattime","10", FCVAR_SERVER };

// NEHAHRA stuff
cvar_t	gl_fogenable = { "gl_fogenable", "0" };
cvar_t	gl_fogdensity = { "gl_fogdensity", "0.8" };
cvar_t	gl_fogred = { "gl_fogred", "0.3" };
cvar_t	gl_fogblue = { "gl_fogblue", "0.3" };
cvar_t	gl_foggreen = { "gl_foggreen", "0.3" };

// Engine Cvars
cvar_t 	*g_psv_gravity = NULL;
cvar_t	*g_psv_aim = NULL;
cvar_t	*g_footsteps = NULL;
cvar_t	*g_psv_quakehulls = NULL;

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

short BigShort( short l )
{
	byte b1, b2;
	b1 = l & 255;
	b2 = (l>>8) & 255;
	return (b1<<8) + b2;
}

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered( void )
{
	unsigned short check[128];
	unsigned char *file;
	int h;

	g_registered = FALSE;
	file = LOAD_FILE( "gfx/pop.lmp", &h );

	if( !file || !h )
	{
		ALERT( at_console, "Playing shareware version.\n" );
		return;
	}

	memcpy( check, file, sizeof( check ));
	FREE_FILE( file );

	for( int i = 0; i < 128; i++ )
	{
		if( pop[i] != (unsigned short)BigShort (check[i] ))
		{
			ALERT( at_console, "Playing shareware version.\n" );
			return;
		}
	}	

	g_registered = TRUE;
	ALERT( at_console, "Playing registered version.\n" );
}

void LinkUserMessages( void )
{
	// Already taken care of?
	if ( gmsgDamage )
	{
		return;
	}

	gmsgDamage = REG_USER_MSG( "Damage", 8 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 );
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1);		// called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0 );		// called every time a new player joins the server
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgHideHUD = REG_USER_MSG( "HideHUD", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgTempEntity = REG_USER_MSG("TempEntity", -1);
	gmsgLevelName = REG_USER_MSG("LevelName", -1);
	gmsgStats = REG_USER_MSG( "Stats", 5 );
	gmsgItems = REG_USER_MSG( "Items", 4 );
	gmsgFoundSecret = REG_USER_MSG( "FoundSecret", 0 );
	gmsgKilledMonster = REG_USER_MSG( "KillMonster", 0 );
	gmsgShowLMP = REG_USER_MSG( "ShowLMP", -1 );
	gmsgHideLMP = REG_USER_MSG( "HideLMP", -1 );
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_psv_aim = CVAR_GET_POINTER( "sv_aim" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );
	g_psv_quakehulls = CVAR_GET_POINTER( "sv_quakehulls" );
	g_psv_teamplay = CVAR_GET_POINTER( "teamplay" );

	if (CVAR_GET_POINTER( "host_gameloaded" ))
		g_fXashEngine = TRUE;
	else g_fXashEngine = FALSE;

	CVAR_REGISTER (&noexit);
	CVAR_REGISTER (&samelevel);
	CVAR_REGISTER (&nospr32);

	CVAR_REGISTER (&fraglimit);
	CVAR_REGISTER (&timelimit);

	CVAR_REGISTER (&fragsleft);
	CVAR_REGISTER (&timeleft);

	CVAR_REGISTER (&friendlyfire);
	CVAR_REGISTER (&forcerespawn);
	CVAR_REGISTER (&aimcrosshair);
	CVAR_REGISTER (&decalfrequency);
	CVAR_REGISTER (&teamlist);
	CVAR_REGISTER (&teamoverride);
	CVAR_REGISTER (&defaultteam);
	CVAR_REGISTER (&allowmonsters);

	CVAR_REGISTER (&mp_chattime);

	CVAR_REGISTER (&pr_checkextension);

	CVAR_REGISTER (&gl_fogenable);
	CVAR_REGISTER (&gl_fogdensity);
	CVAR_REGISTER (&gl_fogred);
	CVAR_REGISTER (&gl_foggreen); 
	CVAR_REGISTER (&gl_fogblue);
 
	LinkUserMessages ();

	// ripped out from quake
	COM_CheckRegistered ();

	PR_LoadProgs( "progs.dat" );
}

void GameResetFog( void )
{
	CVAR_SET_FLOAT( "gl_fogenable", 0.0f );
	CVAR_SET_FLOAT( "gl_fogdensity", 0.0f );
	CVAR_SET_FLOAT( "gl_fogred", 0.0f );
	CVAR_SET_FLOAT( "gl_foggreen", 0.0f );
	CVAR_SET_FLOAT( "gl_fogblue", 0.0f );
}

void GameDLLShutdown( void )
{
	PR_UnloadProgs();
}