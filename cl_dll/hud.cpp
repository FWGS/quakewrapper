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
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

extern client_sprite_t	*GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);
hud_player_info_t		g_PlayerInfoList[MAX_PLAYERS+1];	// player info from the engine
extra_player_info_t		g_PlayerExtraInfo[MAX_PLAYERS+1];	// additional player info sent directly to the client dll
team_info_t		g_TeamInfo[MAX_TEAMS+1];
int			g_iGameType;

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;

void ShutdownInput (void);

int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_HideHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HideHUD(pszName, iSize, pbuf );
}

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_Damage(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_Damage( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

int __MsgFunc_TempEntity(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_TempEntity( pszName, iSize, pbuf );
}
 
// This is called every time the DLL is loaded
void CHud :: Init( void )
{
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( HideHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( TempEntity );

	m_iFOV = 0;

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_CREATE( "default_fov", "90", 0 );
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );

	// weapon auto-switch preference, read by the server from userinfo
	// for the rerelease CheckPlayerEXFlags builtin:
	// 0 - never switch, 1 - vanilla "better weapon", 2 - only new weapons
	CVAR_CREATE( "cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	m_pCvarCrosshair = CVAR_CREATE( "crosshair", "1", FCVAR_ARCHIVE );

	// status bar height override: 0 - follow viewsize, 1 - as if viewsize 120, 2 - viewsize 110, 3 - viewsize 100
	m_pCvarSBLines = CVAR_CREATE( "hud_sblines", "0", FCVAR_ARCHIVE );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );

	// Clear any old HUD list
	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_sbar.Init();
	m_SayText.Init();
	m_Message.Init();
	m_TextMessage.Init();
	m_Scoreboard.Init();
	m_Sound.Init();

	MsgFunc_ResetHUD( 0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	if( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}
}

void CHud :: VidInit( void )
{
	// real window size: pfnGetScreenInfo reports a fake resolution when hud_scale is set
	m_scrinfo.iSize = sizeof(m_scrinfo);
	m_scrinfo.iWidth = CVAR_GET_FLOAT( "vid_width" );
	m_scrinfo.iHeight = CVAR_GET_FLOAT( "vid_height" );

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// we do still honor hud_scale, just in our way:
	// 0 = autoscale, otherwise nearest integer clamped to the largest scale the real screen size can fit
	int maxscale = Q_max( 1, Q_min( ScreenWidth / 320, ScreenHeight / 240 ));
	float scale = round( CVAR_GET_FLOAT( "hud_scale" ));

	if( scale <= 0.0f )
		m_iScale = maxscale;
	else
		m_iScale = Q_max( 1, Q_min((int)scale, maxscale ));

	m_iConcharsTex = gRenderfuncs.GL_LoadTexture( "gfx/conchars", NULL, 0, TF_CLAMP|TF_NOMIPMAP|TF_NEAREST );
	m_iWhiteTex = gRenderfuncs.GL_FindTexture( "*white" );

	m_sbar.VidInit();
	m_Message.VidInit();
	m_SayText.VidInit();
	m_TextMessage.VidInit();
	m_Scoreboard.VidInit();
	m_Sound.VidInit();

	CL_ClearBeams();
}

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	if ( newfov == 0 )
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == def_fov )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}
	return 1;
}


void CHud::AddHudElem(CHudBase *phudelem)
{
	HUDLIST *pdl, *ptemp;

	if (!phudelem)
		return;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}