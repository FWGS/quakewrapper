 /*
dll_int.cpp - dll entry point
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "menu.h"
#include "exportdef.h"

ui_enginefuncs_t engfuncs;
ui_globalvars_t	*gpGlobals;
qboolean visible;
HIMAGE player_image;

int UI_VidInit( void )
{
	player_image = Draw_CachePic( "gfx/menuplyr.lmp", PIC_KEEP_SOURCE );
	engfuncs.pfnSetConsoleDefaultColor( 192, 192, 192 );
	return 1;
}

void UI_Shutdown( void )
{
}

void UI_MouseMove( int x, int y )
{
}

void UI_GetCursorPos( int *pos_x, int *pos_y )
{
}

void UI_SetCursorPos( int pos_x, int pos_y )
{
}

void UI_ShowCursor( int show )
{
}

void UI_CharEvent( int key )
{
}

int UI_MouseInRect( void )	// mouse entering\leave game window
{
	return 1;
}

int  UI_IsVisible( void )
{
	return visible;
}

int  UI_CreditsActive( void )	// unused
{
	return 0;
}

void UI_FinalCredits( void )	// show credits + game end
{
}

static UI_FUNCTIONS gFunctionTable = 
{
	UI_VidInit,
	UI_Init,
	UI_Shutdown,
	UI_Redraw,
	UI_KeyEvent,
	UI_MouseMove,
	UI_SetActiveMenu,
	UI_AddServerToList,
	UI_GetCursorPos,
	UI_SetCursorPos,
	UI_ShowCursor,
	UI_CharEvent,
	UI_MouseInRect,
	UI_IsVisible,
	UI_CreditsActive,
	UI_FinalCredits
};

//=======================================================================
//			GetApi
//=======================================================================
extern "C" EXPORT int GetMenuAPI(UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals)
{
	if( !pFunctionTable || !pEngfuncsFromEngine )
	{
		return FALSE;
	}

	// copy HUD_FUNCTIONS table to engine, copy engfuncs table from engine
	memcpy( pFunctionTable, &gFunctionTable, sizeof( UI_FUNCTIONS ));
	memcpy( &engfuncs, pEngfuncsFromEngine, sizeof( ui_enginefuncs_t ));

	gpGlobals = pGlobals;

	return TRUE;
}
