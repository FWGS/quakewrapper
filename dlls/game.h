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

#ifndef GAME_H
#define GAME_H

extern void GameDLLInit( void );
extern void GameDLLShutdown( void );
extern void GameResetFog( void );

// multiplayer server rules
extern cvar_t	*g_psv_teamplay;
extern cvar_t	fraglimit;
extern cvar_t	timelimit;
extern cvar_t	friendlyfire;
extern cvar_t	forcerespawn;
extern cvar_t	aimcrosshair;
extern cvar_t	decalfrequency;
extern cvar_t	teamlist;
extern cvar_t	teamoverride;
extern cvar_t	defaultteam;
extern cvar_t	allowmonsters;

extern cvar_t	gl_fogenable;
extern cvar_t	gl_fogdensity;
extern cvar_t	gl_fogred;
extern cvar_t	gl_fogblue;
extern cvar_t	gl_foggreen;

// Engine Cvars
extern cvar_t	*g_psv_gravity;
extern cvar_t	*g_psv_aim;
extern cvar_t	*g_footsteps;
extern cvar_t	*g_psv_quakehulls;

extern int	gmsgHudText;
extern int	gmsgSayText;
extern int	gmsgHideHUD;
extern int	gmsgTextMsg;
extern int	gmsgScoreInfo;
extern int	gmsgTempEntity;	// quake missing sfx
extern int	gmsgFoundSecret;
extern int	gmsgKilledMonster;
extern int	gmsgLevelName;
extern int	gmsgResetHUD;
extern int	gmsgInitHUD;
extern int	gmsgStats;
extern int	gmsgItems;
extern int	gmsgDamage;
extern int	gmsgShowLMP;
extern int	gmsgHideLMP;

#endif		// GAME_H
