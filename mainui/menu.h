#pragma once
#ifndef MENU_H
#define MENU_H

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4244)	// MIPS

typedef unsigned char byte;
typedef int qboolean;

#define FALSE 0
#define TRUE (!FALSE)

extern "C"
{
#include "menu_int.h" // engine interface
#include "keydefs.h"
#include "netadr.h"
}

enum
{
	KEY_CONSOLE = 0,
	KEY_GAME,
	KEY_MENU
};

// NEHAHRA types
#define TYPE_CLASSIC	0	// not a nehahra
#define TYPE_NEHGAME	1
#define TYPE_NEHDEMO	2
#define TYPE_NEHFULL	3

#define MAX_INFO_STRING 128

const char *Info_ValueForKey( const char *infostring, const char *key );

#define S_LocalSound engfuncs.pfnPlayLocalSound
#define Cvar_SetValue engfuncs.pfnCvarSetValue
#define Cvar_SetString engfuncs.pfnCvarSetString
#define Cvar_Set Cvar_SetString
#define Key_KeynumToString engfuncs.pfnKeynumToString
#define Key_SetBinding engfuncs.pfnKeySetBinding

#define ScreenWidth (gpGlobals->scrWidth)
#define ScreenHeight (gpGlobals->scrHeight)

#define Cvar_GetValue engfuncs.pfnGetCvarFloat //a1ba: added by me
#define Cvar_GetString engfuncs.pfnGetCvarString

#define Cmd_AddCommand engfuncs.pfnAddCommand

#define realtime (gpGlobals->time)

#define min( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define max( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define bound( low, val, high ) max( low, min( val, high ) )

#define Con_DPrintf engfuncs.pfnCon_DPrintf

#define CL_IsActive()	(engfuncs.pfnClientInGame() && !Cvar_GetValue( "cl_background" ))
#define FILE_EXISTS( file )	(*engfuncs.pfnFileExists)( file, FALSE )

#define MAX_CLIENTS 32

extern ui_enginefuncs_t engfuncs;
extern ui_globalvars_t	*gpGlobals;

extern qboolean visible;

int  UI_VidInit( void );
void UI_Init( void );
void UI_Shutdown( void );
void UI_Redraw( float flTime );
void UI_KeyEvent( int key, int down );
void UI_MouseMove( int x, int y );
void UI_SetActiveMenu( int active );
void UI_AddServerToList( struct netadr_s adr, const char *info );
void UI_GetCursorPos( int *pos_x, int *pos_y );
void UI_SetCursorPos( int pos_x, int pos_y );
void UI_ShowCursor( int show );
void UI_CharEvent( int key );
int  UI_MouseInRect( void );	// mouse entering\leave game window
int  UI_IsVisible( void );
int  UI_CreditsActive( void );	// unused
void UI_FinalCredits( void );	// show credits + game end

inline HIMAGE Draw_CachePic( const char *picname, int flags = 0 )
{
	return engfuncs.pfnPIC_Load( picname, NULL, 0, flags );
}

inline void Draw_TransPic( int x, int y, HIMAGE pic )
{
	engfuncs.pfnPIC_Set( pic, 255, 255, 255, 255 );
	engfuncs.pfnPIC_DrawTrans( x, y, -1, -1, 0 );
}

inline void Draw_Pic( int x, int y, HIMAGE pic )
{
	engfuncs.pfnPIC_Set( pic, 255, 255, 255, 255 );
	engfuncs.pfnPIC_Draw( x, y, -1, -1, NULL );
}

inline void Draw_PicFull( HIMAGE pic )
{
	engfuncs.pfnPIC_Set( pic, 255, 255, 255, 255 );
	engfuncs.pfnPIC_Draw( 0, 0, gpGlobals->scrWidth, gpGlobals->scrHeight, NULL );
}

inline bool Con_ToggleConsole_f( void )
{
	if( gpGlobals->allow_console )
	{
		UI_SetActiveMenu( 0 );
		engfuncs.pfnSetKeyDest( KEY_CONSOLE );
		return true;
	}

	return false;
}

inline void Cbuf_InsertText( const char *msg )
{
	engfuncs.pfnClientCmd( 1, msg );
}

inline void Cbuf_AddText( const char *msg )
{
	engfuncs.pfnClientCmd( 0, msg );	
}

inline int Draw_PicWidth( HIMAGE pic )
{
	return engfuncs.pfnPIC_Width( pic );
}

char *va( const char *fmt, ... );

extern char com_gamedir[64];
extern int hipnotic, rogue, nehahra;
extern qboolean localSearch;
extern HIMAGE player_image;

#endif // MENU_H
