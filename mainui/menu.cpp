/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2017 a1batross

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "menu.h"

enum m_state_e 
{
	m_none, 
	m_main, 
	m_demo, 
	m_singleplayer, 
	m_load, 
	m_save, 
	m_multiplayer, 
	m_setup,
	m_net, 
	m_options, 
	m_video,
	m_keys,
	m_help, 
	m_quit, 
	m_serialconfig, 
	m_modemconfig,
	m_lanconfig, 
	m_gameoptions, 
	m_search, 
	m_slist,
	m_mods
} m_state;

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
		void M_Menu_Net_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Mods_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);
void M_Menu_SerialConfig_f (void);
	void M_Menu_ModemConfig_f (void);
void M_Menu_LanConfig_f (void);
void M_Menu_GameOptions_f (void);
void M_Menu_Search_f (void);
void M_Menu_ServerList_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
		void M_Net_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Mods_Draw (void);
		void M_Video_Draw (void);
	void M_Help_Draw (void);
	void M_Quit_Draw (void);
void M_SerialConfig_Draw (void);
	void M_ModemConfig_Draw (void);
void M_LanConfig_Draw (void);
void M_GameOptions_Draw (void);
void M_Search_Draw (void);
void M_ServerList_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
		void M_Mods_Key (int key);
		void M_Video_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);
void M_SerialConfig_Key (int key);
	void M_ModemConfig_Key (int key);
void M_LanConfig_Key (int key);
void M_GameOptions_Key (int key);
void M_Search_Key (int key);
void M_ServerList_Key (int key);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

int	m_return_state;
qboolean	m_return_onerror;
char	m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
#define SerialConfig	(m_net_cursor == 0)
#define DirectConfig	(m_net_cursor == 1)
#define IPXConfig		(m_net_cursor == 2)
#define TCPIPConfig		(m_net_cursor == 3)

// Nehahra
int NumberOfDemos;
typedef struct
{
	char name[50];
	char desc[50];
} demonames_t;

demonames_t	Demos[35];
int		demo_cursor;

// void M_ConfigureNetSubsystem(void);

//=============================================================================
/* Support Routines */

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;			// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );
	// Terminate it
	out[len] = 0;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char *Info_ValueForKey( const char *s, const char *key )
{
	char	pkey[MAX_INFO_STRING];
	static	char value[2][MAX_INFO_STRING]; // use two buffers so compares work without stomping on each other
	static	int valueindex;
	char	*o;
	
	valueindex ^= 1;
	if( *s == '\\' ) s++;
	
	while( 1 )
	{
		o = pkey;
		while( *s != '\\' && *s != '\n' )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[valueindex];

		while( *s != '\\' && *s != '\n' && *s )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}
		*o = 0;

		if( !strcmp( key, pkey ))
			return value[valueindex];
		if( !*s ) return "";
		s++;
	}
}

char *va( const char *format, ... )
{
	va_list		argptr;
	static char	string[256][1024], *s;
	static int	stringindex = 0;

	s = string[stringindex];
	stringindex = (stringindex + 1) & 255;
	va_start( argptr, format );
	_vsnprintf( s, sizeof( string[0] ), format, argptr );
	va_end( argptr );

	return s;
}

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	char str[] = { num, '\0' };
	engfuncs.pfnDrawConsoleString( cx + ((ScreenWidth - 320)>>1), line + ((ScreenHeight - 240)>>1), str );
	// Draw_Character ( cx + ((ScreenWidth - 320)>>1), line, num);
}

void M_Print (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, HIMAGE pic)
{
	Draw_TransPic (x + ((ScreenWidth - 320)>>1), y + ((ScreenHeight - 240)>>1), pic);
}

void M_DrawPic (int x, int y, HIMAGE pic)
{
	Draw_TransPic (x + ((ScreenWidth - 320)>>1), y + ((ScreenHeight - 240)>>1), pic);
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	HIMAGE p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}

//=============================================================================

int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f( void )
{
	UI_SetActiveMenu( !visible );
}

void UI_SetActiveMenu ( int fActive )
{
	if( fActive )
	{
		engfuncs.pfnKeyClearStates();
		
		engfuncs.pfnSetKeyDest( KEY_MENU );
		
		if( !visible )
		{
			visible = 1;
			M_Menu_Main_f();
		}
	}
	else
	{
		engfuncs.pfnSetKeyDest( KEY_GAME );
		m_state = m_none;
		visible = 0;
	}
}

void M_Demo_Draw( void )
{
	int	i;

	for( i = 0; i < NumberOfDemos; i++ )
		M_PrintWhite( 16, 16 + 8 * i, Demos[i].desc );

	// line cursor
	M_DrawCharacter( 8, 16 + demo_cursor * 8, 12 + ((int)(realtime*4)&1));
}


void M_Menu_Demos_f (void)
{
	UI_SetActiveMenu( 1 );
	m_state = m_demo;
	m_entersound = true;

	NumberOfDemos = 34;

	strcpy(Demos[0].name,  "INTRO");         strcpy(Demos[0].desc,  "Prologue");
	strcpy(Demos[1].name,  "GENF");          strcpy(Demos[1].desc,  "The Beginning");
	strcpy(Demos[2].name,  "GENLAB");        strcpy(Demos[2].desc,  "A Doomed Project");
	strcpy(Demos[3].name,  "NEHCRE");        strcpy(Demos[3].desc,  "The New Recruits");
	strcpy(Demos[4].name,  "MAXNEH");        strcpy(Demos[4].desc,  "Breakthrough");
	strcpy(Demos[5].name,  "MAXCHAR");       strcpy(Demos[5].desc,  "Renewal and Duty");
	strcpy(Demos[6].name,  "CRISIS");        strcpy(Demos[6].desc,  "Worlds Collide");
	strcpy(Demos[7].name,  "POSTCRIS");      strcpy(Demos[7].desc,  "Darkening Skies");
	strcpy(Demos[8].name,  "HEARING");       strcpy(Demos[8].desc,  "The Hearing");
	strcpy(Demos[9].name,  "GETJACK");       strcpy(Demos[9].desc,  "On a Mexican Radio");
	strcpy(Demos[10].name, "PRELUDE");       strcpy(Demos[10].desc, "Honor and Justice");
	strcpy(Demos[11].name, "ABASE");         strcpy(Demos[11].desc, "A Message Sent");
	strcpy(Demos[12].name, "EFFECT");        strcpy(Demos[12].desc, "The Other Side");
	strcpy(Demos[13].name, "UHOH");          strcpy(Demos[13].desc, "Missing in Action");
	strcpy(Demos[14].name, "PREPARE");       strcpy(Demos[14].desc, "The Response");
	strcpy(Demos[15].name, "VISION");        strcpy(Demos[15].desc, "Farsighted Eyes");
	strcpy(Demos[16].name, "MAXTURNS");      strcpy(Demos[16].desc, "Enter the Immortal");
	strcpy(Demos[17].name, "BACKLOT");       strcpy(Demos[17].desc, "Separate Ways");
	strcpy(Demos[18].name, "MAXSIDE");       strcpy(Demos[18].desc, "The Ancient Runes");
	strcpy(Demos[19].name, "COUNTER");       strcpy(Demos[19].desc, "The New Initiative");
	strcpy(Demos[20].name, "WARPREP");       strcpy(Demos[20].desc, "Ghosts to the World");
	strcpy(Demos[21].name, "COUNTER1");      strcpy(Demos[21].desc, "A Fate Worse Than Death");
	strcpy(Demos[22].name, "COUNTER2");      strcpy(Demos[22].desc, "Friendly Fire");
	strcpy(Demos[23].name, "COUNTER3");      strcpy(Demos[23].desc, "Minor Setback");
	strcpy(Demos[24].name, "MADMAX");        strcpy(Demos[24].desc, "Scores to Settle");
	strcpy(Demos[25].name, "QUAKE");         strcpy(Demos[25].desc, "One Man");
	strcpy(Demos[26].name, "CTHMM");         strcpy(Demos[26].desc, "Shattered Masks");
	strcpy(Demos[27].name, "SHADES");        strcpy(Demos[27].desc, "Deal with the Dead");
	strcpy(Demos[28].name, "GOPHIL");        strcpy(Demos[28].desc, "An Unlikely Hero");
	strcpy(Demos[29].name, "CSTRIKE");       strcpy(Demos[29].desc, "War in Hell");
	strcpy(Demos[30].name, "SHUBSET");       strcpy(Demos[30].desc, "The Conspiracy");
	strcpy(Demos[31].name, "SHUBDIE");       strcpy(Demos[31].desc, "Even Death May Die");
	strcpy(Demos[32].name, "NEWRANKS");      strcpy(Demos[32].desc, "An Empty Throne");
	strcpy(Demos[33].name, "SEAL");          strcpy(Demos[33].desc, "The Seal is Broken");

	// find the last demo what we seeing
	for( int i = 0; i < NumberOfDemos; i++ )
	{
		if( !stricmp( Demos[i].name, Cvar_GetString( "lastdemo" )))
		{
			demo_cursor = i;
			break;
		}

	}
}

void M_Demo_Key( int k )
{
	switch( k )
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_state = m_none;
		UI_SetActiveMenu(0);
		engfuncs.pfnClientCmd( FALSE, va ("playdemo %s 1\n", Demos[demo_cursor].name));
		return;
	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		demo_cursor--;
		if (demo_cursor < 0)
			demo_cursor = NumberOfDemos - 1;
		break;
	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		demo_cursor++;
		if (demo_cursor > NumberOfDemos - 1 )
			demo_cursor = 0;
		break;
	}
}
		
//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
int	MAIN_ITEMS = 5;

void M_Menu_Main_f (void)
{
	if( nehahra == TYPE_NEHDEMO )
		MAIN_ITEMS = 4;
	else if( nehahra == TYPE_NEHFULL )
		MAIN_ITEMS = 6;
	else MAIN_ITEMS = 5;

	/*if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}*/
	UI_SetActiveMenu( 1 );
	m_state = m_main;
	m_entersound = true;
}
				

void M_Main_Draw (void)
{
	int		f;
	HIMAGE p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_main.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	switch( nehahra )
	{
	case TYPE_NEHDEMO:
		M_DrawTransPic (72, 32, Draw_CachePic ("gfx/demomenu.lmp"));
		break;
	case TYPE_NEHGAME:
		M_DrawTransPic (72, 32, Draw_CachePic ("gfx/gamemenu.lmp") );
		break;
	default:
		// nehahra both game and movie or all other games
		M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mainmenu.lmp") );
		break;
	}

	f = (int)(realtime * 10)%6;
	
	M_DrawTransPic (54, 32 + m_main_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		if( CL_IsActive() || gpGlobals->allow_console )
		{
			UI_SetActiveMenu( 0 );
			m_state = m_none;
		}
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			if( nehahra == TYPE_NEHDEMO )
				M_Menu_Demos_f ();
			else M_Menu_SinglePlayer_f ();
			break;

		case 1:
			if( nehahra == TYPE_NEHFULL )
				M_Menu_Demos_f ();
			else if( nehahra == TYPE_NEHDEMO )
				M_Menu_Help_f ();
			else if( nehahra == TYPE_CLASSIC )
				M_Menu_MultiPlayer_f ();
			break;

		case 2:
			if( nehahra != TYPE_NEHFULL )
				M_Menu_Options_f ();
			break;

		case 3:
			if( nehahra == TYPE_NEHDEMO )
				M_Menu_Quit_f ();
			else if( nehahra == TYPE_NEHFULL )
				M_Menu_Options_f ();
			else M_Menu_Help_f ();
			break;

		case 4:
			if( nehahra == TYPE_NEHFULL )
				M_Menu_Help_f ();
			else M_Menu_Quit_f ();
			break;
		case 5:
			// nehahra game & movie
			M_Menu_Quit_f ();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
int m_active_modal = 0;
const char *m_modal_msg1, *m_modal_msg2;
#define	SINGLEPLAYER_ITEMS	3


void SCR_ModalMessage()
{
	
}

void M_Menu_SinglePlayer_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw (void)
{
	int		f;
	HIMAGE p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp_menu.lmp") );

	f = (int)(realtime * 10)%6;

	M_DrawTransPic (54, 32 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
	
	if( m_active_modal )
	{
		M_DrawTextBox (60, 10*8, 23, 4);
		M_PrintWhite(68, 11*8, m_modal_msg1);
		M_PrintWhite(68, 12*8, m_modal_msg2);
	}
}

void M_StartNewGame( void )
{
	UI_SetActiveMenu(0);
	if( Cvar_GetValue( "host_serverstate" ) && Cvar_GetValue( "maxplayers" ) > 1 )
		engfuncs.pfnHostEndGame( "end of the game" );

	Cvar_SetValue( "skill", 1.0f );	// default
	Cvar_SetValue( "deathmatch", 0.0f );
	Cvar_SetValue( "teamplay", 0.0f );
	Cvar_SetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	Cvar_SetValue( "maxplayers", 1.0f );
	Cvar_SetValue( "coop", 0.0f );
	engfuncs.pfnClientCmd( FALSE, "newgame\n" );
}

void M_SinglePlayer_Key (int key)
{
	if( m_active_modal )
	{
		switch( key )
		{
			case 'y':
			case 'Y':
				M_StartNewGame();
				m_active_modal = 0;
				break;
			case 'n':
			case 'N':
				m_active_modal = 0;
				break;
		}
		
	}
	
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (CL_IsActive() && !m_active_modal)
			{
				m_modal_msg1 = "Are you sure you want to";
				m_modal_msg2 = "    start a new game?   ";
				m_active_modal = 1;
				break;
			}
			M_StartNewGame();
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
#define	SAVEGAME_COMMENT_LENGTH	39
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int		i, j;
	char	name[256];
	char	comment[256];

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf (name, "save/s%i.sav", i);
		
		if( !engfuncs.pfnFileExists( name, true ) )
			continue;
		
		
		engfuncs.pfnGetSaveComment( name, comment );
		// fscanf (f, "%i\n", &version);
		// fscanf (f, "%79s\n", name);
		strncpy (m_filenames[i], comment, SAVEGAME_COMMENT_LENGTH);
		m_filenames[i][SAVEGAME_COMMENT_LENGTH] = 0;

	// change _ back to space
		for (j=0 ; j<SAVEGAME_COMMENT_LENGTH ; j++)
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		loadable[i] = true;
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	UI_SetActiveMenu(1);
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!CL_IsActive())
		return;
	/*if (cl.intermission)
		return;*/
	if (Cvar_GetValue("maxplayers") > 1)
		return;
	m_entersound = true;
	m_state = m_save;
	UI_SetActiveMenu(1);
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	HIMAGE p;

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;
	HIMAGE p;

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		UI_SetActiveMenu(0);

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
		m_state = m_none;
		UI_SetActiveMenu(0);
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	3


void M_Menu_MultiPlayer_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw (void)
{
	int		f;
	HIMAGE p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	f = (int)(realtime * 10)%6;

	M_DrawTransPic (54, 32 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			M_Menu_Net_f ();
			break;

		case 1:
			M_Menu_Net_f ();
			break;

		case 2:
			M_Menu_Setup_f ();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int		setup_cursor = 4;
int		setup_cursor_table[] = {40, 56, 80, 104, 140};

char	setup_hostname[16];
char	setup_myname[16];
int		setup_oldtop;
int		setup_oldbottom;
int		setup_top;
int		setup_bottom;

#define	NUM_SETUP_CMDS	5

void M_Menu_Setup_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_setup;
	m_entersound = true;
	strcpy(setup_myname, Cvar_GetString("name"));
	strcpy(setup_hostname, Cvar_GetString("hostname"));
	setup_top = setup_oldtop = Cvar_GetValue("topcolor");
	setup_bottom = setup_oldbottom = Cvar_GetValue("bottomcolor");
	engfuncs.pfnProcessImage( player_image, -1.0f, setup_top, setup_bottom);
}


void M_Setup_Draw (void)
{
	HIMAGE p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_Print (168, 56, setup_myname);

	M_Print (64, 80, "Shirt color");
	M_Print (64, 104, "Pants color");

	M_DrawTextBox (64, 140-8, 14, 1);
	M_Print (72, 140, "Accept Changes");

	p = Draw_CachePic ("gfx/bigbox.lmp");
	M_DrawTransPic (160, 64, p);
	p = player_image;
	M_DrawTransPic (172, 72, p); //!!! TRANSLATE

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}


void M_Setup_Key (int k)
{
	int	l;

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top - 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom - 1;
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cvar_SetValue("topcolor", setup_top);
			Cvar_SetValue("bottomcolor", setup_bottom);
			
			engfuncs.pfnProcessImage( player_image, -1.0f, setup_top, setup_bottom);
		}
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
forward:
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 2)
			setup_top = setup_top + 1;
		if (setup_cursor == 3)
			setup_bottom = setup_bottom + 1;
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cvar_SetValue("topcolor", setup_top);
			Cvar_SetValue("bottomcolor", setup_bottom);
			
			engfuncs.pfnProcessImage( player_image, -1.0f, setup_top, setup_bottom);
		}
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 2 || setup_cursor == 3)
			goto forward;

		// setup_cursor == 4 (OK)
		if (strcmp(Cvar_GetString("name"), setup_myname) != 0)
			Cvar_Set ("name", setup_myname );
		if (strcmp(Cvar_GetString("hostname"), setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cvar_SetValue("topcolor", setup_top);
			Cvar_SetValue("bottomcolor", setup_bottom);
			
			engfuncs.pfnProcessImage( player_image, -1.0f, setup_top, setup_bottom);
		}
		m_entersound = true;
		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;
		if (setup_cursor == 0)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l+1] = 0;
				setup_hostname[l] = k;
			}
		}
		if (setup_cursor == 1)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l+1] = 0;
				setup_myname[l] = k;
			}
		}
	}

	if (setup_top > 13)
		setup_top = 0;
	if (setup_top < 0)
		setup_top = 13;
	if (setup_bottom > 13)
		setup_bottom = 0;
	if (setup_bottom < 0)
		setup_bottom = 13;
}

//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;
int m_net_saveHeight;

#define ENABLE_DEAD_PROTOCOLS 0

char *net_helpMessage [] =
{
/* .........1.........2.... */
#if ENABLE_DEAD_PROTOCOLS
  "                        ",
  " Two computers connected",
  "   through two modems.  ",
  "                        ",

  "                        ",
  " Two computers connected",
  " by a null-modem cable. ",
  "                        ",

  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",
#endif

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_net;
	m_entersound = true;
	m_net_items = 1; // TCP/IP only

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	HIMAGE p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	f = 32;
	//!!!

#if ENABLE_DEAD_PROTOCOLS
	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen1.lmp");
	}
	else
	{
#ifdef _WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_modm.lmp");
#endif
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;

	if (serialAvailable)
	{
		p = Draw_CachePic ("gfx/netmen2.lmp");
	}
	else
	{
#ifdef _WIN32
		p = NULL;
#else
		p = Draw_CachePic ("gfx/dim_drct.lmp");
#endif
	}

	if (p)
		M_DrawTransPic (72, f, p);

	f += 19;
	if (ipxAvailable)
		p = Draw_CachePic ("gfx/netmen3.lmp");
	else
		p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);

	f += 19;
	if (tcpipAvailable)
#endif
		p = Draw_CachePic ("gfx/netmen4.lmp");
#if ENABLE_DEAD_PROTOCOLS
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
#endif
	M_DrawTransPic (72, f, p);

#if ENABLE_DEAD_PROTOCOLS
	if (m_net_items == 5)	// JDC, could just be removed
	{
		f += 19;
		p = Draw_CachePic ("gfx/netmen5.lmp");
		M_DrawTransPic (72, f, p);
	}
#endif

	f = (320-26*8)/2;
	M_DrawTextBox (f, 134, 24, 4);
	f += 8;
	M_Print (f, 142, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 150, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 158, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 166, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(realtime * 10)%6;
	M_DrawTransPic (54, 32 + m_net_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_Net_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
#if ENABLE_DEAD_PROTOCOLS
		case 0:
			M_Menu_SerialConfig_f ();
			break;

		case 1:
			M_Menu_SerialConfig_f ();
			break;

		case 2:
			M_Menu_LanConfig_f ();
			break;

		case 3:
			M_Menu_LanConfig_f ();
			break;

		case 4:
// multiprotocol
			break;
#endif
		case 0:
			M_Menu_LanConfig_f ();
			break;
		}
	}

#if ENABLE_DEAD_PROTOCOLS
	if (m_net_cursor == 0 && !serialAvailable)
		goto again;
	if (m_net_cursor == 1 && !serialAvailable)
		goto again;
	if (m_net_cursor == 2 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 3 && !tcpipAvailable)
		goto again;
#endif
}

//=============================================================================
/* OPTIONS MENU */

#define	OPTIONS_ITEMS	14

#define	SLIDER_RANGE	10

int		options_cursor;

void M_Menu_Options_f (void)
{
	UI_SetActiveMenu( 1 );
	m_state = m_options;
	m_entersound = true;
}


void M_AdjustSliders (int dir)
{
	float val;
	S_LocalSound ("misc/menu3.wav");

	switch (options_cursor)
	{
	case 4:	// screen size
		/*scr_viewsize.value += dir * 10;
		if (scr_viewsize.value < 30)
			scr_viewsize.value = 30;
		if (scr_viewsize.value > 120)
			scr_viewsize.value = 120;*/
		val = bound( 30, Cvar_GetValue( "viewsize" ) + dir * 10, 120 );
		Cvar_SetValue ("viewsize", val);
		break;
	case 5:	// gamma
		val = bound( 0.5, Cvar_GetValue( "gamma" ) + dir * 0.05, 3 );
		/*v_gamma.value -= dir * 0.05;
		if (v_gamma.value < 0.5)
			v_gamma.value = 0.5;
		if (v_gamma.value > 1)
			v_gamma.value = 1;*/
		Cvar_SetValue ("gamma", val);
		break;
	case 6:	// mouse speed
		val = bound( 1, Cvar_GetValue( "sensitivity" ) + dir * 0.5, 11 );
		/*sensitivity.value += dir * 0.5;
		if (sensitivity.value < 1)
			sensitivity.value = 1;
		if (sensitivity.value > 11)
			sensitivity.value = 11;*/
		Cvar_SetValue ("sensitivity", val);
		break;
	case 7:	// music volume
/*#ifdef _WIN32
		bgmvolume.value += dir * 1.0;
#else
		bgmvolume.value += dir * 0.1;
#endif
		if (bgmvolume.value < 0)
			bgmvolume.value = 0;
		if (bgmvolume.value > 1)
			bgmvolume.value = 1;*/
		val = bound( 0, Cvar_GetValue( "MP3Volume" ) + dir * 0.1, 1 );

		Cvar_SetValue ("MP3Volume", val);
		break;
	case 8:	// sfx volume
		/*volume.value += dir * 0.1;
		if (volume.value < 0)
			volume.value = 0;
		if (volume.value > 1)
			volume.value = 1;*/
		val = bound( 0, Cvar_GetValue( "volume" ) + dir * 0.1, 1 );
		Cvar_SetValue ("volume", val);
		break;
		
	case 9:	// allways run
		if (Cvar_GetValue( "cl_forwardspeed" ) > 200)
		{
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
			Cvar_SetValue ("cl_sidespeed", 200);
		}
		else
		{
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
			Cvar_SetValue ("cl_sidespeed", 400);
		}
		break;
	
	case 10:	// invert mouse
		Cvar_SetValue ("m_pitch", -Cvar_GetValue("m_pitch"));
		break;
	
	case 11:	// lookspring
		Cvar_SetValue ("lookspring", !Cvar_GetValue("lookspring"));
		break;
	
	case 12:	// lookstrafe
		Cvar_SetValue ("lookstrafe", !Cvar_GetValue("lookstrafe"));
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i=0 ; i<SLIDER_RANGE ; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	float		r;
	HIMAGE	p;
	int y;
	
	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);
	
	y = 32;
	
	M_Print (16, y, "    Customize controls"); y+=8;
	M_Print (16, y, "         Go to console"); y+=8;
	M_Print (16, y, "     Reset to defaults"); y+=8;
	M_Print (16, y, "           Browse Mods"); y+=8;

	M_Print (16, y, "           Screen size");
	r = (Cvar_GetValue("viewsize") - 30) / (120 - 30);
	M_DrawSlider (220, y, r); y+=8;

	M_Print (16, y, "            Brightness");
	r = (Cvar_GetValue("gamma") - 0.5) / (3 - 0.5);
	M_DrawSlider (220, y, r); y+=8;

	M_Print (16, y, "           Mouse Speed");
	r = (Cvar_GetValue("sensitivity") - 1)/10;
	M_DrawSlider (220, y, r); y+=8;

	M_Print (16, y, "       CD Music Volume");
	r = Cvar_GetValue("MP3Volume");
	M_DrawSlider (220, y, r); y+=8;

	M_Print (16, y, "          Sound Volume");
	r = Cvar_GetValue("volume");
	M_DrawSlider (220, y, r); y+=8;

	M_Print (16, y,  "            Always Run");
	M_DrawCheckbox (220, y, Cvar_GetValue("cl_forwardspeed") > 200); y+=8;

	M_Print (16, y, "          Invert Mouse");
	M_DrawCheckbox (220, y, Cvar_GetValue("m_pitch") < 0); y+=8;

	M_Print (16, y, "            Lookspring");
	M_DrawCheckbox (220, y, Cvar_GetValue("lookspring")); y+=8;

	M_Print (16, y, "            Lookstrafe");
	M_DrawCheckbox (220, y, Cvar_GetValue("lookstrafe")); y+=8;


	M_Print (16, y, "         Video Options"); y+=8;

// cursor
	M_DrawCharacter (200, 32 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case 0:
			M_Menu_Keys_f ();
			break;
		case 1:
			if( Con_ToggleConsole_f( ))
				m_state = m_none;	
			break;
		case 2:
			Cbuf_AddText ("exec default.cfg\n");
			break;
		case 3: 
			M_Menu_Mods_f();
			break;
		case 13:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;
	
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;	

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

	if (options_cursor == OPTIONS_ITEMS)
	{
		if (k == K_UPARROW)
			options_cursor = OPTIONS_ITEMS-1;
		else
			options_cursor = 0;
	}
}


//=============================================================================
/* KEYS MENU */

char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"impulse 10", 		"change weapon"},
{"+jump", 			"jump / swim up"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"swim up"},
{"+movedown",		"swim down"}
};

#define	NUMCOMMANDS	(sizeof(bindnames)/sizeof(bindnames[0]))

int		keys_cursor;
int		bind_grab;

void M_Menu_Keys_f (void)
{
	UI_SetActiveMenu( 1 );
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	const char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = engfuncs.pfnKeyGetBinding(j);
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	const char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = engfuncs.pfnKeyGetBinding(j);
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}


void M_Keys_Draw (void)
{
	int		i, l;
	int		keys[2];
	const char	*name;
	int		x, y;
	HIMAGE p;

	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	if (bind_grab)
		M_Print (12, 32, "Press a key or button for this action");
	else
		M_Print (18, 32, "Enter to change, backspace to clear");
		
// search for known bindings
	for (i=0 ; i<NUMCOMMANDS ; i++)
	{
		y = 48 + 8*i;

		M_Print (16, y, bindnames[i][1]);

		l = strlen (bindnames[i][0]);
		
		M_FindKeysForCommand (bindnames[i][0], keys);
		
		if (keys[0] == -1)
		{
			M_Print (140, y, "???");
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			M_Print (140, y, name);
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				M_Print (140 + x + 8, y, "or");
				M_Print (140 + x + 32, y, Key_KeynumToString (keys[1]));
			}
		}
	}
	
	if (bind_grab)
		M_DrawCharacter (130, 48 + keys_cursor*8, '=');
	else
		M_DrawCharacter (130, 48 + keys_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[2];
	
	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf (cmd, "bind %s \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor][0]);			
			Cbuf_InsertText (cmd);
		}
		
		bind_grab = false;
		return;
	}
	
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
			keys_cursor = NUMCOMMANDS-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= NUMCOMMANDS)
			keys_cursor = 0;
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand (bindnames[keys_cursor][0], keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[1] != -1)
			M_UnbindCommand (bindnames[keys_cursor][0]);
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

#define NUM_MAXMODES 32

#define MAX_COLUMN_SIZE 16

const char *modes[NUM_MAXMODES];
int nummodes;
int vid_modenum, vid_prevmode;
int vid_testingmode;
float vid_testendtime;
int vid_column_size;
int vid_line;

void M_Menu_Video_f (void)
{
	UI_SetActiveMenu( 1 );	
	m_state = m_video;
	m_entersound = true;
	nummodes = 0;
	
	for( int i = 0; i < NUM_MAXMODES; i++ )
	{
		modes[i] = engfuncs.pfnGetModeString( i );
		if( modes[i] )
			nummodes++;
	}
}


void M_Video_Draw (void)
{
	HIMAGE		p;
	const char	*ptr;
	int		i, column, row;
	char		temp[100];

	p = Draw_CachePic ("gfx/vidmodes.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	vid_column_size = (nummodes + 1) / 2;

	column = 16;
	row = 36;

	for (i=0 ; i<nummodes ; i++)
	{
		if ((int)Cvar_GetValue("vid_mode") == i)
			M_PrintWhite (column, row, modes[i]);
		else
			M_Print (column, row, modes[i]);

		row += 8;

		if ((i % vid_column_size) == (vid_column_size - 1))
		{
			column += 20*8;
			row = 36;
		}
	}

// line cursor
	if (vid_testingmode)
	{
		sprintf (temp, "TESTING %s",
				modes[vid_line]);
		M_Print (13*8, 36 + MAX_COLUMN_SIZE * 8 + 8*4, temp);
		M_Print (9*8, 36 + MAX_COLUMN_SIZE * 8 + 8*6,
				"Please wait 5 seconds...");
		
		if( realtime > vid_testendtime )
		{
			Cvar_SetValue( "vid_mode", vid_prevmode );
			vid_testingmode = false;
		}
	}
	else
	{
		M_Print (9*8, 36 + MAX_COLUMN_SIZE * 8 + 8,
				"Press Enter to set mode");
		M_Print (6*8, 36 + MAX_COLUMN_SIZE * 8 + 8*3,
				"T to test mode for 5 seconds");
		sprintf (temp, "D to make %s the default", modes[vid_line]);
		M_Print (6*8, 36 + MAX_COLUMN_SIZE * 8 + 8*5, temp);
		int vidmode = (int)Cvar_GetValue("vid_mode");
		if( vidmode >= 0 && vidmode < NUM_MAXMODES )
			ptr = modes[vidmode];
		else
			ptr = NULL;

		if (ptr)
		{
			sprintf (temp, "Current default is %s", ptr);
			M_Print (7*8, 36 + MAX_COLUMN_SIZE * 8 + 8*6, temp);
		}

		M_Print (15*8, 36 + MAX_COLUMN_SIZE * 8 + 8*8,
				"Esc to exit");

		row = 36 + (vid_line % vid_column_size) * 8;
		column = 8 + (vid_line / vid_column_size) * 20*8;

		M_DrawCharacter (column, row, 12+((int)(realtime*4)&1));
	}
}


void M_Video_Key (int key)
{
	if (vid_testingmode)
		return;

	switch (key)
	{
	case K_ESCAPE:
		S_LocalSound ("misc/menu1.wav");
		M_Menu_Options_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_line--;

		if (vid_line < 0)
			vid_line = nummodes - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_line++;

		if (vid_line >= nummodes)
			vid_line = 0;
		break;

	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_line -= vid_column_size;

		if (vid_line < 0)
		{
			vid_line += ((nummodes + (vid_column_size - 1)) /
					vid_column_size) * vid_column_size;

			while (vid_line >= nummodes)
				vid_line -= vid_column_size;
		}
		break;

	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		vid_line += vid_column_size;

		if (vid_line >= nummodes)
		{
			vid_line -= ((nummodes + (vid_column_size - 1)) /
					vid_column_size) * vid_column_size;

			while (vid_line < 0)
				vid_line += vid_column_size;
		}
		break;

	case 'T':
	case 't':
		S_LocalSound ("misc/menu1.wav");
		vid_prevmode = Cvar_GetValue( "vid_mode" );
		Cvar_SetValue( "vid_mode", vid_line );
		vid_testingmode = 1;
		vid_testendtime = realtime + 5.0;
		break;
		
	case K_ENTER:
	case 'D':
	case 'd':
		S_LocalSound ("misc/menu1.wav");
		Cvar_SetValue( "vid_mode", vid_line );
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	if( nehahra )
	{
		m_state = m_none;
		UI_SetActiveMenu(0);
		engfuncs.pfnClientCmd( FALSE, "playdemo ENDCRED" );
	}
	else
	{
		UI_SetActiveMenu( 1 );
		m_state = m_help;
		m_entersound = true;
		help_page = 0;
	}
}

void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%i.lmp", help_page)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f ();
		break;
		
	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
m_state_e		m_quit_prevstate;
qboolean	wasInMenus;

char *quitMessage [] = 
{
/* .........1.........2.... */
  "  Are you gonna quit    ",
  "  this game just like   ",
  "   everything else?     ",
  "                        ",
 
  " Milord, methinks that  ",
  "   thou art a lowly     ",
  " quitter. Is this true? ",
  "                        ",

  " Do I need to bust your ",
  "  face open for trying  ",
  "        to quit?        ",
  "                        ",

  " Man, I oughta smack you",
  "   for trying to quit!  ",
  "     Press Y to get     ",
  "      smacked out.      ",
 
  " Press Y to quit like a ",
  "   big loser in life.   ",
  "  Press N to stay proud ",
  "    and successful!     ",
 
  "   If you press Y to    ",
  "  quit, I will summon   ",
  "  Satan all over your   ",
  "      hard drive!       ",
 
  "  Um, Asmodeus dislikes ",
  " his children trying to ",
  " quit. Press Y to return",
  "   to your Tinkertoys.  ",
 
  "  If you quit now, I'll ",
  "  throw a blanket-party ",
  "   for you next time!   ",
  "                        "
};

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	UI_SetActiveMenu( 1 );	
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = engfuncs.pfnRandomLong( 0, 7 );
}


void M_Quit_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (UI_IsVisible())
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			UI_SetActiveMenu( 0 );
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		engfuncs.pfnClientCmd(0, "quit\n");
		break;

	default:
		break;
	}

}

void M_Quit_Draw (void)
{
	M_DrawTextBox (0, 0, 38, 23);
	M_PrintWhite (16, 12,   "  QuakeWrapper 0.7 by Unkle Mike\n\n");
	M_PrintWhite (16, 28,   "Programming        Testing \n");
	M_Print (16, 36,        " Unkle Mike        ...\n");
	M_Print (16, 44,        " a1batross         ...\n");

	if( nehahra )
	{
		M_DrawTextBox (0, 0, 38, 23);
		M_PrintWhite (16, 12,  "  Nehahra \n\n");
		M_PrintWhite (16, 180, "Press y to exit\n");
	}	
	else
	{
		for( int i = 0; i < 4; i++ )
		{
			M_PrintWhite( 16, 68 + i*8, quitMessage[msgNumber*4 + i] );
		}

		M_PrintWhite (16, 140, "Quake is a trademark of Id Software,\n");
		M_PrintWhite (16, 148, "inc., (c)1996 Id Software, inc. All\n");
		M_PrintWhite (16, 156, "rights reserved. NIN logo is a\n");
		M_PrintWhite (16, 164, "registered trademark licensed to\n");
		M_PrintWhite (16, 172, "Nothing Interactive, Inc. All rights\n");
		M_PrintWhite (16, 180, "reserved. Press y to exit\n");
	}
}

//=============================================================================

/* SERIAL CONFIG MENU */

#if ENABLE_DEAD_PROTOCOLS

int		serialConfig_cursor;
int		serialConfig_cursor_table[] = {48, 64, 80, 96, 112, 132};
#define	NUM_SERIALCONFIG_CMDS	6

static int ISA_uarts[]	= {0x3f8,0x2f8,0x3e8,0x2e8};
static int ISA_IRQs[]	= {4,3,4,3};
int serialConfig_baudrate[] = {9600,14400,19200,28800,38400,57600};

int		serialConfig_comport;
int		serialConfig_irq ;
int		serialConfig_baud;
char	serialConfig_phone[16];

void M_Menu_SerialConfig_f (void)
{
	int		n;
	int		port;
	int		baudrate;
	qboolean	useModem;

	UI_SetActiveMenu( 1 );
	m_state = m_serialconfig;
	m_entersound = true;
	
	if (JoiningGame && SerialConfig)
		serialConfig_cursor = 4;
	else
		serialConfig_cursor = 5;
	
	port = 0;
	serialConfig_irq = 0;
	serialConfig_comport = 0;
	serialConfig_baud = 0;
	baudrate = 0;
	useModem = false;

	/*// map uart's port to COMx
	for (n = 0; n < 4; n++)
		if (ISA_uarts[n] == port)
			break;
	if (n == 4)
	{
		n = 0;
		serialConfig_irq = 4;
	}
	serialConfig_comport = n + 1;

	// map baudrate to index
	for (n = 0; n < 6; n++)
		if (serialConfig_baudrate[n] == baudrate)
			break;
	if (n == 6)
		n = 5;
	serialConfig_baud = n;*/

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_SerialConfig_Draw (void)
{
	HIMAGE p;
	int		basex;
	char	*startJoin;
	char	*directModem;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-Draw_PicWidth(p))/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	if (SerialConfig)
		directModem = "Modem";
	else
		directModem = "Direct Connect";
	M_Print (basex, 32, va ("%s - %s", startJoin, directModem));
	basex += 8;

	M_Print (basex, serialConfig_cursor_table[0], "Port");
	M_DrawTextBox (160, 40, 4, 1);
	M_Print (168, serialConfig_cursor_table[0], va("COM%u", serialConfig_comport));

	M_Print (basex, serialConfig_cursor_table[1], "IRQ");
	M_DrawTextBox (160, serialConfig_cursor_table[1]-8, 1, 1);
	M_Print (168, serialConfig_cursor_table[1], va("%u", serialConfig_irq));

	M_Print (basex, serialConfig_cursor_table[2], "Baud");
	M_DrawTextBox (160, serialConfig_cursor_table[2]-8, 5, 1);
	M_Print (168, serialConfig_cursor_table[2], va("%u", serialConfig_baudrate[serialConfig_baud]));

	if (SerialConfig)
	{
		M_Print (basex, serialConfig_cursor_table[3], "Modem Setup...");
		if (JoiningGame)
		{
			M_Print (basex, serialConfig_cursor_table[4], "Phone number");
			M_DrawTextBox (160, serialConfig_cursor_table[4]-8, 16, 1);
			M_Print (168, serialConfig_cursor_table[4], serialConfig_phone);
		}
	}

	if (JoiningGame)
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 7, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "Connect");
	}
	else
	{
		M_DrawTextBox (basex, serialConfig_cursor_table[5]-8, 2, 1);
		M_Print (basex+8, serialConfig_cursor_table[5], "OK");
	}

	M_DrawCharacter (basex-8, serialConfig_cursor_table [serialConfig_cursor], 12+((int)(realtime*4)&1));

	if (serialConfig_cursor == 4)
		M_DrawCharacter (168 + 8*strlen(serialConfig_phone), serialConfig_cursor_table [serialConfig_cursor], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_SerialConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor--;
		if (serialConfig_cursor < 0)
			serialConfig_cursor = NUM_SERIALCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		serialConfig_cursor++;
		if (serialConfig_cursor >= NUM_SERIALCONFIG_CMDS)
			serialConfig_cursor = 0;
		break;

	case K_LEFTARROW:
		if (serialConfig_cursor > 2)
			break;
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport--;
			if (serialConfig_comport == 0)
				serialConfig_comport = 4;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq--;
			if (serialConfig_irq == 6)
				serialConfig_irq = 5;
			if (serialConfig_irq == 1)
				serialConfig_irq = 7;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud--;
			if (serialConfig_baud < 0)
				serialConfig_baud = 5;
		}

		break;

	case K_RIGHTARROW:
		if (serialConfig_cursor > 2)
			break;
forward:
		S_LocalSound ("misc/menu3.wav");

		if (serialConfig_cursor == 0)
		{
			serialConfig_comport++;
			if (serialConfig_comport > 4)
				serialConfig_comport = 1;
			serialConfig_irq = ISA_IRQs[serialConfig_comport-1];
		}

		if (serialConfig_cursor == 1)
		{
			serialConfig_irq++;
			if (serialConfig_irq == 6)
				serialConfig_irq = 7;
			if (serialConfig_irq == 8)
				serialConfig_irq = 2;
		}

		if (serialConfig_cursor == 2)
		{
			serialConfig_baud++;
			if (serialConfig_baud > 5)
				serialConfig_baud = 0;
		}

		break;

	case K_ENTER:
		if (serialConfig_cursor < 3)
			goto forward;

		m_entersound = true;

		/*if (serialConfig_cursor == 3)
		{
			(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

			M_Menu_ModemConfig_f ();
			break;
		}

		if (serialConfig_cursor == 4)
		{
			serialConfig_cursor = 5;
			break;
		}

		// serialConfig_cursor == 5 (OK/CONNECT)
		(*SetComPortConfig) (0, ISA_uarts[serialConfig_comport-1], serialConfig_irq, serialConfig_baudrate[serialConfig_baud], SerialConfig);

		M_ConfigureNetSubsystem ();*/

		if (StartingGame)
		{
			M_Menu_GameOptions_f ();
			break;
		}

		m_return_state = m_state;
		m_return_onerror = true;
		UI_SetActiveMenu(0);
		m_state = m_none;

		if (SerialConfig)
			Cbuf_AddText (va ("connect \"%s\"\n", serialConfig_phone));
		else
			Cbuf_AddText ("connect\n");
		break;

	case K_BACKSPACE:
		if (serialConfig_cursor == 4)
		{
			if (strlen(serialConfig_phone))
				serialConfig_phone[strlen(serialConfig_phone)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;
		if (serialConfig_cursor == 4)
		{
			l = strlen(serialConfig_phone);
			if (l < 15)
			{
				serialConfig_phone[l+1] = 0;
				serialConfig_phone[l] = key;
			}
		}
	}

	if (DirectConfig && (serialConfig_cursor == 3 || serialConfig_cursor == 4))
		if (key == K_UPARROW)
			serialConfig_cursor = 2;
		else
			serialConfig_cursor = 5;

	if (SerialConfig && StartingGame && serialConfig_cursor == 4)
		if (key == K_UPARROW)
			serialConfig_cursor = 3;
		else
			serialConfig_cursor = 5;
}

//=============================================================================
/* MODEM CONFIG MENU */

int		modemConfig_cursor;
int		modemConfig_cursor_table [] = {40, 56, 88, 120, 156};
#define NUM_MODEMCONFIG_CMDS	5

char	modemConfig_dialing;
char	modemConfig_clear [16];
char	modemConfig_init [32];
char	modemConfig_hangup [16];

void M_Menu_ModemConfig_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_modemconfig;
	m_entersound = true;
	// (*GetModemConfig) (0, &modemConfig_dialing, modemConfig_clear, modemConfig_init, modemConfig_hangup);
}


void M_ModemConfig_Draw (void)
{
	HIMAGE p;
	int		basex;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-Draw_PicWidth(p))/2;
	M_DrawPic (basex, 4, p);
	basex += 8;

	if (modemConfig_dialing == 'P')
		M_Print (basex, modemConfig_cursor_table[0], "Pulse Dialing");
	else
		M_Print (basex, modemConfig_cursor_table[0], "Touch Tone Dialing");

	M_Print (basex, modemConfig_cursor_table[1], "Clear");
	M_DrawTextBox (basex, modemConfig_cursor_table[1]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[1]+12, modemConfig_clear);
	if (modemConfig_cursor == 1)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_clear), modemConfig_cursor_table[1]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[2], "Init");
	M_DrawTextBox (basex, modemConfig_cursor_table[2]+4, 30, 1);
	M_Print (basex+8, modemConfig_cursor_table[2]+12, modemConfig_init);
	if (modemConfig_cursor == 2)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_init), modemConfig_cursor_table[2]+12, 10+((int)(realtime*4)&1));

	M_Print (basex, modemConfig_cursor_table[3], "Hangup");
	M_DrawTextBox (basex, modemConfig_cursor_table[3]+4, 16, 1);
	M_Print (basex+8, modemConfig_cursor_table[3]+12, modemConfig_hangup);
	if (modemConfig_cursor == 3)
		M_DrawCharacter (basex+8 + 8*strlen(modemConfig_hangup), modemConfig_cursor_table[3]+12, 10+((int)(realtime*4)&1));

	M_DrawTextBox (basex, modemConfig_cursor_table[4]-8, 2, 1);
	M_Print (basex+8, modemConfig_cursor_table[4], "OK");

	M_DrawCharacter (basex-8, modemConfig_cursor_table [modemConfig_cursor], 12+((int)(realtime*4)&1));
}


void M_ModemConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_SerialConfig_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor--;
		if (modemConfig_cursor < 0)
			modemConfig_cursor = NUM_MODEMCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		modemConfig_cursor++;
		if (modemConfig_cursor >= NUM_MODEMCONFIG_CMDS)
			modemConfig_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			S_LocalSound ("misc/menu1.wav");
		}
		break;

	case K_ENTER:
		if (modemConfig_cursor == 0)
		{
			if (modemConfig_dialing == 'P')
				modemConfig_dialing = 'T';
			else
				modemConfig_dialing = 'P';
			m_entersound = true;
		}

		if (modemConfig_cursor == 4)
		{
			// (*SetModemConfig) (0, va ("%c", modemConfig_dialing), modemConfig_clear, modemConfig_init, modemConfig_hangup);
			m_entersound = true;
			M_Menu_SerialConfig_f ();
		}
		break;

	case K_BACKSPACE:
		if (modemConfig_cursor == 1)
		{
			if (strlen(modemConfig_clear))
				modemConfig_clear[strlen(modemConfig_clear)-1] = 0;
		}

		if (modemConfig_cursor == 2)
		{
			if (strlen(modemConfig_init))
				modemConfig_init[strlen(modemConfig_init)-1] = 0;
		}

		if (modemConfig_cursor == 3)
		{
			if (strlen(modemConfig_hangup))
				modemConfig_hangup[strlen(modemConfig_hangup)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (modemConfig_cursor == 1)
		{
			l = strlen(modemConfig_clear);
			if (l < 15)
			{
				modemConfig_clear[l+1] = 0;
				modemConfig_clear[l] = key;
			}
		}

		if (modemConfig_cursor == 2)
		{
			l = strlen(modemConfig_init);
			if (l < 29)
			{
				modemConfig_init[l+1] = 0;
				modemConfig_init[l] = key;
			}
		}

		if (modemConfig_cursor == 3)
		{
			l = strlen(modemConfig_hangup);
			if (l < 15)
			{
				modemConfig_hangup[l+1] = 0;
				modemConfig_hangup[l] = key;
			}
		}
	}
}

#endif

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
int		lanConfig_cursor_table [] = {72, 92, 100, 124};
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor == 2)
		lanConfig_cursor = 1;
	// lanConfig_port = DEFAULTnet_hostport;
	lanConfig_port = 27015;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw (void)
{
	HIMAGE p;
	int		basex;
	char	*startJoin;
	char	*protocol;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-Draw_PicWidth(p))/2;
	M_DrawPic (basex, 4, p);

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	/*if (IPXConfig)
		protocol = "IPX";
	else*/
		protocol = "TCP/IP";
	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print (basex, 52, "Address:");
	/*if (IPXConfig)
		M_Print (basex+9*8, 52, my_ipx_address);
	else
		M_Print (basex+9*8, 52, my_tcpip_address);*/

	M_Print (basex, lanConfig_cursor_table[0], "Port");
	M_DrawTextBox (basex+8*8, lanConfig_cursor_table[0]-8, 6, 1);
	M_Print (basex+9*8, lanConfig_cursor_table[0], lanConfig_portname);

	if (JoiningGame)
	{
		M_Print (basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print (basex, lanConfig_cursor_table[2], "Search for Internet games...");
		M_Print (basex, 108, "Join game at:");
		M_DrawTextBox (basex+8, lanConfig_cursor_table[3]-8, 22, 1);
		M_Print (basex+16, lanConfig_cursor_table[3], lanConfig_joinname);
	}
	else
	{
		M_DrawTextBox (basex, lanConfig_cursor_table[1]-8, 2, 1);
		M_Print (basex+8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter (basex-8, lanConfig_cursor_table [lanConfig_cursor], 12+((int)(realtime*4)&1));

	if (lanConfig_cursor == 0)
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), lanConfig_cursor_table [0], 10+((int)(realtime*4)&1));

	if (lanConfig_cursor == 3)
		M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), lanConfig_cursor_table [3], 10+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		// M_ConfigureNetSubsystem ();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			localSearch = true;
			M_Menu_Search_f();
			break;
		}
		
		if (lanConfig_cursor == 2)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f ();
				break;
			}
			localSearch = false;
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 3)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			UI_SetActiveMenu(0);
			Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			break;
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;

	default:
		if (key < 32 || key > 127)
			break;

		if (lanConfig_cursor == 3)
		{
			l = strlen(lanConfig_joinname);
			if (l < 21)
			{
				lanConfig_joinname[l+1] = 0;
				lanConfig_joinname[l] = key;
			}
		}

		if (key < '0' || key > '9')
			break;
		if (lanConfig_cursor == 0)
		{
			l = strlen(lanConfig_portname);
			if (l < 5)
			{
				lanConfig_portname[l+1] = 0;
				lanConfig_portname[l] = key;
			}
		}
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	char	*name;
	char	*description;
} level_t;

level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
   {"start", "Command HQ"},  // 0

   {"hip1m1", "The Pumping Station"},          // 1
   {"hip1m2", "Storage Facility"},
   {"hip1m3", "The Lost Mine"},
   {"hip1m4", "Research Facility"},
   {"hip1m5", "Military Complex"},

   {"hip2m1", "Ancient Realms"},          // 6
   {"hip2m2", "The Black Cathedral"},
   {"hip2m3", "The Catacombs"},
   {"hip2m4", "The Crypt"},
   {"hip2m5", "Mortum's Keep"},
   {"hip2m6", "The Gremlin's Domain"},

   {"hip3m1", "Tur Torment"},       // 12
   {"hip3m2", "Pandemonium"},
   {"hip3m3", "Limbo"},
   {"hip3m4", "The Gauntlet"},

   {"hipend", "Armagon's Lair"},       // 16

   {"hipdm1", "The Edge of Oblivion"}           // 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
   {"Scourge of Armagon", 0, 1},
   {"Fortress of the Dead", 1, 5},
   {"Dominion of Darkness", 6, 6},
   {"The Rift", 12, 4},
   {"Final Level", 16, 1},
   {"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int	startepisode;
int	startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_Menu_GameOptions_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = Cvar_GetValue("maxplayers");
	if (maxplayers < 2)
		maxplayers = MAX_CLIENTS;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 112, 120};
#define	NUM_GAMEOPTIONS	9
int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	HIMAGE p;
	int		x;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);

	M_DrawTextBox (152, 32, 10, 1);
	M_Print (160, 40, "begin game");

	M_Print (0, 56, "      Max players");
	M_Print (160, 56, va("%i", maxplayers) );

	M_Print (0, 64, "        Game Type");
	if (Cvar_GetValue("coop"))
		M_Print (160, 64, "Cooperative");
	else
		M_Print (160, 64, "Deathmatch");

	M_Print (0, 72, "        Teamplay");
	if (rogue)
	{
		char *msg;

		switch((int)Cvar_GetValue("teamplay"))
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}
	else
	{
		char *msg;

		switch((int)Cvar_GetValue("teamplay"))
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, 72, msg);
	}

	M_Print (0, 80, "            Skill");
	if (Cvar_GetValue("skill") == 0)
		M_Print (160, 80, "Easy difficulty");
	else if (Cvar_GetValue("skill") == 1)
		M_Print (160, 80, "Normal difficulty");
	else if (Cvar_GetValue("skill") == 2)
		M_Print (160, 80, "Hard difficulty");
	else
		M_Print (160, 80, "Nightmare difficulty");

	M_Print (0, 88, "       Frag Limit");
	if (Cvar_GetValue("fraglimit") == 0)
		M_Print (160, 88, "none");
	else
		M_Print (160, 88, va("%i frags", (int)Cvar_GetValue("fraglimit")));

	M_Print (0, 96, "       Time Limit");
	if (Cvar_GetValue("timelimit") == 0)
		M_Print (160, 96, "none");
	else
		M_Print (160, 96, va("%i minutes", (int)Cvar_GetValue("timelimit")));

	M_Print (0, 112, "         Episode");
   //MED 01/06/97 added hipnotic episodes
   if (hipnotic)
      M_Print (160, 112, hipnoticepisodes[startepisode].description);
   //PGM 01/07/97 added rogue episodes
   else if (rogue)
      M_Print (160, 112, rogueepisodes[startepisode].description);
   else
      M_Print (160, 112, episodes[startepisode].description);

	M_Print (0, 120, "           Level");
   //MED 01/06/97 added hipnotic episodes
   if (hipnotic)
   {
      M_Print (160, 120, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
      M_Print (160, 128, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
   }
   //PGM 01/07/97 added rogue episodes
   else if (rogue)
   {
      M_Print (160, 120, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
      M_Print (160, 128, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
   }
   else
   {
      M_Print (160, 120, levels[episodes[startepisode].firstLevel + startlevel].description);
      M_Print (160, 128, levels[episodes[startepisode].firstLevel + startlevel].name);
   }

// line cursor
	M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320-26*8)/2;
			M_DrawTextBox (x, 138, 24, 4);
			x += 8;
			M_Print (x, 146, "  More than 4 players   ");
			M_Print (x, 154, " requires using command ");
			M_Print (x, 162, "line parameters; please ");
			M_Print (x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
}


void M_NetStart_Change (int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > MAX_CLIENTS)
		{
			maxplayers = MAX_CLIENTS;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetValue ("coop", Cvar_GetValue("coop") ? 0 : 1);
		break;

	case 3:
		if (rogue)
			count = 6;
		else
			count = 2;

		Cvar_SetValue ("teamplay", Cvar_GetValue("teamplay") + dir);
		if ( Cvar_GetValue("teamplay") > count)
			Cvar_SetValue ("teamplay", 0);
		else if ( Cvar_GetValue("teamplay") < 0)
			Cvar_SetValue ("teamplay", count);
		break;

	case 4:
		Cvar_SetValue ("skill", Cvar_GetValue("skill") + dir);
		if (Cvar_GetValue("skill") > 3)
			Cvar_SetValue ("skill", 0);
		if (Cvar_GetValue("skill") < 0)
			Cvar_SetValue ("skill", 3);
		break;

	case 5:
		Cvar_SetValue ("fraglimit", Cvar_GetValue("fraglimit") + dir*10);
		if (Cvar_GetValue("fraglimit") > 100)
			Cvar_SetValue ("fraglimit", 0);
		if (Cvar_GetValue("fraglimit") < 0)
			Cvar_SetValue ("fraglimit", 100);
		break;

	case 6:
		Cvar_SetValue ("timelimit", Cvar_GetValue("timelimit") + dir*5);
		if (Cvar_GetValue("timelimit") > 60)
			Cvar_SetValue ("timelimit", 0);
		if (Cvar_GetValue("timelimit") < 0)
			Cvar_SetValue ("timelimit", 60);
		break;

	case 7:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (hipnotic)
			count = 6;
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
			count = 4;
		/*
		else if (Cvar_GetValue("registered"))
			count = 7;
		else
			count = 2;*/
		else count = 7; // TODO: Make sure we're NOT playing shareware Quake

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
    //MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if( Cvar_GetValue( "host_serverstate" ) && Cvar_GetValue( "maxplayers" ) == 1 )
				engfuncs.pfnHostEndGame( "end of the game" );
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );

			if (hipnotic)
				Cbuf_AddText ( va ("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name) );
			else if (rogue)
				Cbuf_AddText ( va ("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name) );
			else
				Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;
qboolean slistSilent, slistLocal;
qboolean slistInProgress;
qboolean localSearch;
int hostCacheCount = 0;

#define HOSTCACHESIZE 128
#define MAX_SHOW_HOSTS 16

typedef struct
{
	char	name[16];
	char	map[16];
	int		users;
	int		maxusers;
	struct netadr_s adr;
} hostcache_t;

hostcache_t hostcache[HOSTCACHESIZE];

void M_Menu_Search_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	memset( hostcache, 0, sizeof( hostcache ) );
	hostCacheCount = 0;
	if( localSearch )
		Cbuf_AddText("localservers\n");
	else
		Cbuf_AddText("internetservers\n");
	
	// NET_Slist_f();
	// TODO: replace by Xash "internetservers" && "localservers"

}


void M_Search_Draw (void)
{
	HIMAGE p;
	int x;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	/*if(slistInProgress)
	{
		NET_Poll();
		return;
	}*/

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

int		slist_cursor;
int 	slist_low;
qboolean slist_sorted;

void UI_AddServerToList( struct netadr_s adr, const char *info )
{
	const char *gamedir = Info_ValueForKey( info, "gamedir" );
		
	if( !stricmp( com_gamedir, gamedir ))
	{
		if( hostCacheCount >= HOSTCACHESIZE )
			return;
		
		hostcache_t *host = hostcache + hostCacheCount;
		
		strncpy(host->name, Info_ValueForKey(info, "host"), 15 );
		host->name[15] = 0;
		
		strncpy(host->map, Info_ValueForKey(info, "map"), 15 );
		host->map[15] = 0;
		
		host->users = atoi(Info_ValueForKey(info, "numcl"));
		
		host->maxusers = atoi(Info_ValueForKey(info, "maxcl"));
		
		host->adr = adr;
		
		++hostCacheCount;	
	}
}

void M_Menu_ServerList_f (void)
{
	UI_SetActiveMenu(1);
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	slist_low = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	int		n;
	char	string [64];
	HIMAGE p;

	/*if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int	i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i+1; j < hostCacheCount; j++)
					if (strcmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Q_memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Q_memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Q_memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}*/

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-Draw_PicWidth(p))/2, 4, p);
	int count = min( hostCacheCount, slist_low + MAX_SHOW_HOSTS );
	int y = 32;
	
	for (n = slist_low; n < count; n++, y += 8)
	{
		sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		
		M_Print (16, y, string);
	}
	M_DrawCharacter (0, 32 + (slist_cursor - slist_low)*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f ();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
		{
			slist_cursor = hostCacheCount - 1;
			slist_low = hostCacheCount - MAX_SHOW_HOSTS;
		}
		else if( slist_cursor - 2 < slist_low )
		{
			slist_low = max( slist_low - 1, 0 );
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
		{
			slist_cursor = 0;
			slist_low = 0;
		}
		else if( slist_cursor + 2 > slist_low + MAX_SHOW_HOSTS )
		{
			slist_low = min( slist_low + MAX_SHOW_HOSTS, hostCacheCount );
		}
		break;

	case K_ENTER:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		UI_SetActiveMenu(0);
		engfuncs.pfnClientJoin( hostcache[slist_cursor].adr );
		break;

	default:
		break;
	}

}

//=============================================================================
/* Browse Mods */

#define MAX_MODS 128
#define MAX_SHOW_MODS 16

struct mod_t
{
	char gamedir[64];
	char description[64];
} modlist[MAX_MODS];
int mods, currentGame;
int modcursor;

void M_Menu_Mods_f( void )
{
	int i;
	UI_SetActiveMenu(1);
	m_state = m_mods;
	m_entersound = true;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	
	memset( modlist, 0, sizeof( modlist ) );
	mods = modcursor = 0;
	currentGame = -1;
	
	// debug scroll lists
	/*for( i = 0; i < MAX_MODS; i++ )
	{
		sprintf( modlist[i].description, "%i\n", i );
		strcpy( modlist[i].gamedir, modlist[i].description );
	}
	mods = MAX_MODS;
	return;*/
	
	GAMEINFO **games = engfuncs.pfnGetGamesList(&mods);
	
	mods = min( mods, MAX_MODS );
	
	for( i = 0; i < mods; i++ )
	{
		strcpy( modlist[i].gamedir, games[i]->gamefolder );
		strcpy( modlist[i].description, games[i]->gamefolder  );
		if( currentGame == -1 && !stricmp( modlist[i].gamedir, com_gamedir ) )
			currentGame = i;
	}
	modcursor = currentGame;
}

int modlow;

void M_Mods_Draw( void )
{
	int x = 16;
	int y = 16;
		
	/*if( modcursor < MAX_SHOW_MODS ) 
	{
		modlow = 0;
	}
	else if( mods - modcursor < MAX_SHOW_MODS ) // somewhere near end of list
	{
		modlow = mods - MAX_SHOW_MODS;
	}*/

	M_PrintWhite( x, y, "         Mod List        " );
	
	y = 32;
	
	int count = min( modlow + MAX_SHOW_MODS, mods );
	
	for( int i = modlow; i < count; i++, y += 8 )
	{
		if( i == currentGame )
			M_PrintWhite( x, y, modlist[i].description );
		else M_Print( x, y, modlist[i].description ); 
	}
	
	M_DrawCharacter (0, 32 + (modcursor - modlow)*8, 12+((int)(realtime*4)&1));
	
}

void M_Mods_Key( int k )
{
	char buf[128];
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_SPACE:
	case K_ENTER:
		sprintf( buf, "game %s\n", modlist[modcursor].gamedir );
		Cbuf_AddText( buf );
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		modcursor--;
		
		if(modcursor < 0)
		{
			modcursor = mods - 1;
			modlow = mods - MAX_SHOW_MODS;
		} 
		else if( modcursor - 2 < modlow )
		{
			modlow--;
		}
		if( modlow < 0 )
			modlow = 0;		
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		modcursor++;
		
		if (modcursor >= mods)
		{
			modcursor = 0;
			modlow = 0;
		}
		else if( modcursor + 2 > modlow + MAX_SHOW_MODS )
		{
			modlow++;
		}
		break;

	default:
		break;
	}
}


//=============================================================================
/* Menu Subsystem */

char com_gamedir[64];
int hipnotic = false, rogue = false, nehahra = false;

void UI_Init(void)
{
	char gamedir_path[1024];
	engfuncs.pfnGetGameDir( gamedir_path );
	COM_FileBase( gamedir_path, com_gamedir );
	
	if( !stricmp( com_gamedir, "rogue" ) ) rogue = true;
	else if( !stricmp( com_gamedir, "hipnotic" ) ) hipnotic = true;
	else if( !stricmp( com_gamedir, "nehahra" ) ) nehahra = true;

	// run additional checks
	if( nehahra )
	{
		if( FILE_EXISTS( "hearing.dem" ) && FILE_EXISTS( "maps/neh1m4.bsp" ))
			nehahra = TYPE_NEHFULL; // complete installation
		else if( FILE_EXISTS( "hearing.dem" ))
			nehahra = TYPE_NEHDEMO; // only movie
		else if( FILE_EXISTS( "maps/neh1m4.bsp" ))
			nehahra = TYPE_NEHGAME; // only game
		else nehahra = TYPE_CLASSIC;	// ???
	}
	
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	if( nehahra == TYPE_NEHFULL || nehahra == TYPE_NEHDEMO )
		Cmd_AddCommand ("menu_demos", M_Menu_Demos_f);
	Cmd_AddCommand ("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand ("menu_load", M_Menu_Load_f);
	Cmd_AddCommand ("menu_save", M_Menu_Save_f);
	if( nehahra == TYPE_CLASSIC )
		Cmd_AddCommand ("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand ("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
	Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("help", M_Menu_Help_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}

void M_DrawTile()
{
	if( Cvar_GetValue("cl_background") )
		return;
	
	if( engfuncs.pfnClientInGame() && Cvar_GetValue("ui_renderworld") )
		return;

	if( nehahra )
	{
		// nehahra shown console image in main menu, not a tile
		Draw_PicFull( Draw_CachePic("gfx/conback.lmp"));
		return;
	}
	
	HIMAGE hImage = Draw_CachePic("gfx/backtile.lmp");
	int x = 0, y = 0;
	int w, h;
	w = Draw_PicWidth( hImage );
	h = engfuncs.pfnPIC_Height( hImage );
	
	while( y < ScreenHeight + h)
	{
		while( x < ScreenWidth + w )
		{
			Draw_Pic( x, y, hImage );
			
			x += w;
		}
		x = 0;
		y += h;
	}
}

void UI_Redraw( float flTime )
{
	if (m_state == m_none)
		return;

	if (!m_recursiveDraw)
	{
		/*scr_copyeverything = 1;

		if (scr_con_current)
		{
			Draw_ConsoleBackground (vid.height);
			VID_UnlockBuffer ();
			S_ExtraUpdate ();
			VID_LockBuffer ();
		}
		else
			Draw_FadeScreen ();

		scr_fullupdate = 0;*/
		
		M_DrawTile();
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_demo:
		M_Demo_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		M_Quit_Draw ();
		break;

	case m_serialconfig:
#if ENABLE_DEAD_PROTOCOLS
		M_SerialConfig_Draw ();
#endif
		break;

	case m_modemconfig:
#if ENABLE_DEAD_PROTOCOLS
		M_ModemConfig_Draw ();
#endif
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	
	case m_mods:
		M_Mods_Draw();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}
}


void UI_KeyEvent(int key, int down)
{
	if( !down )
		return;
	
	/*if( key == '`' )
	{
		UI_SetActiveMenu( FALSE );
		engfuncs.pfnSetKeyDest( KEY_CONSOLE );
		return;
	}*/
	
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_demo:
		M_Demo_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_serialconfig:
#if ENABLE_DEAD_PROTOCOLS
		M_SerialConfig_Key (key);
#endif
		return;

	case m_modemconfig:
#if ENABLE_DEAD_PROTOCOLS
		M_ModemConfig_Key (key);
#endif
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
		
	case m_mods:
		M_Mods_Key(key);
		break;
	}
}


