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
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "ref_params.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "stringlib.h"

#define STAT_MINUS		10 // num frame for '-' stats digit

DECLARE_MESSAGE( m_sbar, Stats )
DECLARE_MESSAGE( m_sbar, Items )
DECLARE_MESSAGE( m_sbar, LevelName )
DECLARE_MESSAGE( m_sbar, FoundSecret )
DECLARE_MESSAGE( m_sbar, KillMonster )
DECLARE_MESSAGE( m_sbar, ShowLMP )
DECLARE_MESSAGE( m_sbar, HideLMP )

int CHudSBar :: hipweapons[4] = { HIT_LASER_CANNON_BIT, HIT_MJOLNIR_BIT, 4, HIT_PROXIMITY_GUN_BIT };

int CHudSBar::Init(void)
{
	char	gamedir[256];

	HOOK_MESSAGE( Stats );
	HOOK_MESSAGE( Items );
	HOOK_MESSAGE( LevelName );
	HOOK_MESSAGE( FoundSecret );
	HOOK_MESSAGE( KillMonster );
	HOOK_MESSAGE( ShowLMP );
	HOOK_MESSAGE( HideLMP );

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	gametype = GAMETYPE_QUAKE;

	COM_FileBase( gEngfuncs.pfnGetGameDirectory(), gamedir );

	if( !Q_stricmp( gamedir, "hipnotic" ) || !Q_stricmp( gamedir, "quoth" ))
		gametype = GAMETYPE_HIPNOTIC;
	else if( !Q_stricmp( gamedir, "rogue" ))
		gametype = GAMETYPE_ROGUE;
	else if( !Q_stricmp( gamedir, "nehahra" ))
		gametype = GAMETYPE_NEHAHRA;

	return 1;
};

int CHudSBar::VidInit( void )
{
	int i;

	memset( gHUD.item_gettime, 0, sizeof( gHUD.item_gettime ));

	for( i = 0; i < SHOWLMP_MAXLABELS; i++ )
		showlmp[i].active = false;

	for( i = 0; i < 10; i++ )
	{
		sb_nums[0][i] = gHUD.DrawPicFromWad (UTIL_VarArgs("num_%i",i));
		sb_nums[1][i] = gHUD.DrawPicFromWad (UTIL_VarArgs("anum_%i",i));
	}

	sb_nums[0][10] = gHUD.DrawPicFromWad ("number_minus");
	sb_nums[1][10] = gHUD.DrawPicFromWad ("anumber_minus");

	sb_colon = gHUD.DrawPicFromWad ("num_colon");
	sb_slash = gHUD.DrawPicFromWad ("num_slash");

	sb_weapons[0][0] = gHUD.DrawPicFromWad ("inv_shotgun");
	sb_weapons[0][1] = gHUD.DrawPicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = gHUD.DrawPicFromWad ("inv_nailgun");
	sb_weapons[0][3] = gHUD.DrawPicFromWad ("inv_snailgun");
	sb_weapons[0][4] = gHUD.DrawPicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = gHUD.DrawPicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = gHUD.DrawPicFromWad ("inv_lightng");

	sb_weapons[1][0] = gHUD.DrawPicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = gHUD.DrawPicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = gHUD.DrawPicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = gHUD.DrawPicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = gHUD.DrawPicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = gHUD.DrawPicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = gHUD.DrawPicFromWad ("inv2_lightng");

	for( i = 0; i < 5; i++ )
	{
		sb_weapons[2+i][0] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_shotgun",i+1));
		sb_weapons[2+i][1] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_sshotgun",i+1));
		sb_weapons[2+i][2] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_nailgun",i+1));
		sb_weapons[2+i][3] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_snailgun",i+1));
		sb_weapons[2+i][4] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_rlaunch",i+1));
		sb_weapons[2+i][5] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_srlaunch",i+1));
		sb_weapons[2+i][6] = gHUD.DrawPicFromWad (UTIL_VarArgs("inva%i_lightng",i+1));
	}

	sb_ammo[0] = gHUD.DrawPicFromWad ("sb_shells");
	sb_ammo[1] = gHUD.DrawPicFromWad ("sb_nails");
	sb_ammo[2] = gHUD.DrawPicFromWad ("sb_rocket");
	sb_ammo[3] = gHUD.DrawPicFromWad ("sb_cells");

	sb_armor[0] = gHUD.DrawPicFromWad ("sb_armor1");
	sb_armor[1] = gHUD.DrawPicFromWad ("sb_armor2");
	sb_armor[2] = gHUD.DrawPicFromWad ("sb_armor3");

	sb_items[0] = gHUD.DrawPicFromWad ("sb_key1");
	sb_items[1] = gHUD.DrawPicFromWad ("sb_key2");
	sb_items[2] = gHUD.DrawPicFromWad ("sb_invis");
	sb_items[3] = gHUD.DrawPicFromWad ("sb_invuln");
	sb_items[4] = gHUD.DrawPicFromWad ("sb_suit");
	sb_items[5] = gHUD.DrawPicFromWad ("sb_quad");

	sb_sigil[0] = gHUD.DrawPicFromWad ("sb_sigil1");
	sb_sigil[1] = gHUD.DrawPicFromWad ("sb_sigil2");
	sb_sigil[2] = gHUD.DrawPicFromWad ("sb_sigil3");
	sb_sigil[3] = gHUD.DrawPicFromWad ("sb_sigil4");

	sb_faces[4][0] = gHUD.DrawPicFromWad ("face1");
	sb_faces[4][1] = gHUD.DrawPicFromWad ("face_p1");
	sb_faces[3][0] = gHUD.DrawPicFromWad ("face2");
	sb_faces[3][1] = gHUD.DrawPicFromWad ("face_p2");
	sb_faces[2][0] = gHUD.DrawPicFromWad ("face3");
	sb_faces[2][1] = gHUD.DrawPicFromWad ("face_p3");
	sb_faces[1][0] = gHUD.DrawPicFromWad ("face4");
	sb_faces[1][1] = gHUD.DrawPicFromWad ("face_p4");
	sb_faces[0][0] = gHUD.DrawPicFromWad ("face5");
	sb_faces[0][1] = gHUD.DrawPicFromWad ("face_p5");

	sb_face_invis = gHUD.DrawPicFromWad ("face_invis");
	sb_face_invuln = gHUD.DrawPicFromWad ("face_invul2");
	sb_face_invis_invuln = gHUD.DrawPicFromWad ("face_inv2");
	sb_face_quad = gHUD.DrawPicFromWad ("face_quad");

	sb_sbar = gHUD.DrawPicFromWad ("sbar");
	sb_ibar = gHUD.DrawPicFromWad ("ibar");
	sb_scorebar = gHUD.DrawPicFromWad ("scorebar");

//MED 01/04/97 added new hipnotic weapons
	if (gametype == GAMETYPE_HIPNOTIC)
	{
		hsb_weapons[0][0] = gHUD.DrawPicFromWad ("inv_laser");
		hsb_weapons[0][1] = gHUD.DrawPicFromWad ("inv_mjolnir");
		hsb_weapons[0][2] = gHUD.DrawPicFromWad ("inv_gren_prox");
		hsb_weapons[0][3] = gHUD.DrawPicFromWad ("inv_prox_gren");
		hsb_weapons[0][4] = gHUD.DrawPicFromWad ("inv_prox");

		hsb_weapons[1][0] = gHUD.DrawPicFromWad ("inv2_laser");
		hsb_weapons[1][1] = gHUD.DrawPicFromWad ("inv2_mjolnir");
	  	hsb_weapons[1][2] = gHUD.DrawPicFromWad ("inv2_gren_prox");
		hsb_weapons[1][3] = gHUD.DrawPicFromWad ("inv2_prox_gren");
		hsb_weapons[1][4] = gHUD.DrawPicFromWad ("inv2_prox");

		for (i=0 ; i<5 ; i++)
		{
			hsb_weapons[2+i][0] = gHUD.DrawPicFromWad (va("inva%i_laser",i+1));
		 	hsb_weapons[2+i][1] = gHUD.DrawPicFromWad (va("inva%i_mjolnir",i+1));
			hsb_weapons[2+i][2] = gHUD.DrawPicFromWad (va("inva%i_gren_prox",i+1));
		 	hsb_weapons[2+i][3] = gHUD.DrawPicFromWad (va("inva%i_prox_gren",i+1));
			hsb_weapons[2+i][4] = gHUD.DrawPicFromWad (va("inva%i_prox",i+1));
		}

		hsb_items[0] = gHUD.DrawPicFromWad ("sb_wsuit");
		hsb_items[1] = gHUD.DrawPicFromWad ("sb_eshld");
	}

	if (gametype == GAMETYPE_ROGUE)
	{
		rsb_invbar[0] = gHUD.DrawPicFromWad ("r_invbar1");
		rsb_invbar[1] = gHUD.DrawPicFromWad ("r_invbar2");

		rsb_weapons[0] = gHUD.DrawPicFromWad ("r_lava");
		rsb_weapons[1] = gHUD.DrawPicFromWad ("r_superlava");
		rsb_weapons[2] = gHUD.DrawPicFromWad ("r_gren");
		rsb_weapons[3] = gHUD.DrawPicFromWad ("r_multirock");
		rsb_weapons[4] = gHUD.DrawPicFromWad ("r_plasma");

		rsb_items[0] = gHUD.DrawPicFromWad ("r_shield1");
		rsb_items[1] = gHUD.DrawPicFromWad ("r_agrav1");

		// PGM 01/19/97 - team color border
		rsb_teambord = gHUD.DrawPicFromWad ("r_teambord");
		// PGM 01/19/97 - team color border

		rsb_ammo[0] = gHUD.DrawPicFromWad ("r_ammolava");
		rsb_ammo[1] = gHUD.DrawPicFromWad ("r_ammomulti");
		rsb_ammo[2] = gHUD.DrawPicFromWad ("r_ammoplasma");
	}

	sb_completed = gHUD.DrawPicFromWad ("complete");
	sb_finale = gHUD.DrawPicFromWad ("finale");
	sb_inter = gHUD.DrawPicFromWad ("inter");

	m_iFlags |= HUD_INTERMISSION;	// g-cont. allow episode finales

	return 1;
};

void CHudSBar::DrawPic( int x, int y, glpic_t *pic, bool showlmp )
{
	if (!showlmp && !gHUD.m_iIntermission)
	{
		x += ((ScreenWidth - 320)>>1);
		y += (ScreenHeight - SBAR_HEIGHT);
	}

	gHUD.DrawPic( x, y, pic );
}

void CHudSBar::DrawTransPic( int x, int y, glpic_t *pic, bool showlmp )
{
	if (!showlmp && !gHUD.m_iIntermission)
	{
		x += ((ScreenWidth - 320)>>1);
		y += (ScreenHeight - SBAR_HEIGHT);
	}

	gHUD.DrawTransPic( x, y, pic );
}

void CHudSBar::DrawNum( int x, int y, int num, int digits, int color )
{
	char	str[12];
	char	*ptr;
	int	l, frame;

	l = UTIL_IntegerToString( num, str );
	ptr = str;

	if( l > digits ) ptr += (l - digits);
	if( l < digits ) x += (digits - l) * 24;

	while( *ptr )
	{
		if( *ptr == '-' )
			frame = STAT_MINUS;
		else frame = *ptr -'0';

		DrawTransPic( x, y, sb_nums[color][frame] );
		x += 24;
		ptr++;
	}
}

void CHudSBar::DrawString( int x, int y, char *str )
{
	gEngfuncs.pfnDrawSetTextColor( 0.5, 0.5, 0.5 );
	DrawConsoleString( x + (( ScreenWidth - 320 ) >> 1), y + ScreenHeight - SBAR_HEIGHT + 2, str );
}

void CHudSBar::DrawCharacter( int x, int y, int num )
{
	char str[3];

	str[0] = (char)num;
	str[1] = '\0';

	DrawString( x, y, str );
}

void CHudSBar::DrawFace( float flTime )
{
	if(( gHUD.items & (IT_INVISIBILITY | IT_INVULNERABILITY)) == (IT_INVISIBILITY|IT_INVULNERABILITY))
	{
		DrawPic( 112, 0, sb_face_invis_invuln );
		return;
	}

	if( gHUD.items & IT_QUAD )
	{
		DrawPic( 112, 0, sb_face_quad );
		return;
	}

	if( gHUD.items & IT_INVISIBILITY)
	{
		DrawPic (112, 0, sb_face_invis );
		return;
	}

	if( gHUD.items & IT_INVULNERABILITY )
	{
		DrawPic( 112, 0, sb_face_invuln );
		return;
	}

	int f, anim;

	if (gHUD.stats[STAT_HEALTH] >= 100)
		f = 4;
	else f = gHUD.stats[STAT_HEALTH] / 20;

	if (flTime <= gHUD.faceanimtime)
		anim = 1;
	else
		anim = 0;

	DrawPic( 112, 0, sb_faces[f][anim] );
}

void CHudSBar::DrawScoreBoard( float flTime )
{
	char str[80];
	int l, minutes, seconds, tens, units;

	sprintf( str, "Monsters:%3i /%3i", gHUD.stats[STAT_MONSTERS], gHUD.stats[STAT_TOTALMONSTERS] );
	DrawString( 8, 1, str );

	sprintf( str, "Secrets :%3i /%3i", gHUD.stats[STAT_SECRETS], gHUD.stats[STAT_TOTALSECRETS] );
	l = ConsoleStringLen( str );
	DrawString( 8, 10, str );

	// time
	minutes = flTime / 60;
	seconds = flTime - 60 * minutes;
	tens = seconds / 10;
	units = seconds - 10 * tens;
	sprintf( str, "Time :%3i:%i%i", minutes, tens, units );
	DrawString( 184, 1, str );

	// draw level name
	DrawString( 16 + l, 10, levelname );
}

void CHudSBar::DrawIntermission( float flTime )
{
	int dig, num;

	DrawPic((ScreenWidth*0.5f)-94, (ScreenHeight*0.5f) - 96, sb_completed );
	DrawTransPic((ScreenWidth*0.5f)-160, (ScreenHeight*0.5f) - 64, sb_inter );

	// time
	dig = completed_time / 60;
	DrawNum(ScreenWidth*0.5f, (ScreenHeight*0.5f) - 56, dig, 3, 0 );
	num = completed_time - dig * 60;
	DrawTransPic((ScreenWidth*0.5f)+74 ,(ScreenHeight*0.5f)- 56, sb_colon );
	DrawTransPic((ScreenWidth*0.5f)+86 ,(ScreenHeight*0.5f)- 56, sb_nums[0][num/10] );
	DrawTransPic((ScreenWidth*0.5f)+106,(ScreenHeight*0.5f)- 56, sb_nums[0][num%10] );

	DrawNum((ScreenWidth*0.5f), (ScreenHeight*0.5f)- 16, gHUD.stats[STAT_SECRETS], 3, 0 );
	DrawTransPic((ScreenWidth*0.5f)+72,(ScreenHeight*0.5f)- 16, sb_slash );
	DrawNum((ScreenWidth*0.5f)+80, (ScreenHeight*0.5f)- 16, gHUD.stats[STAT_TOTALSECRETS], 3, 0 );

	DrawNum((ScreenWidth*0.5f), (ScreenHeight*0.5f)+ 24, gHUD.stats[STAT_MONSTERS], 3, 0 );
	DrawTransPic((ScreenWidth*0.5f)+72,(ScreenHeight*0.5f)+ 24, sb_slash );
	DrawNum((ScreenWidth*0.5f)+80, (ScreenHeight*0.5f)+ 24, gHUD.stats[STAT_TOTALMONSTERS], 3, 0 );
}

void CHudSBar::DrawFinale( float flTime )
{
	DrawTransPic((ScreenWidth-288)*0.5f, 16, sb_finale );
}

void CHudSBar::DrawInventory( float flTime )
{
	char	num[6];
	float	time;
	int	i, flashon;

	if (gametype == GAMETYPE_ROGUE)
	{
		if( gHUD.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
			DrawPic( 0, -24, rsb_invbar[0] );
		else
			DrawPic( 0, -24, rsb_invbar[1] );
	}
	else
	{
		DrawPic( 0, -24, sb_ibar );
	}

	// weapons
	for( i = 0; i < 7; i++ )
	{
		if( gHUD.items & ( IT_SHOTGUN << i ))
		{
			time = gHUD.item_gettime[i];
			flashon = (int)((flTime - time) * 10);
			if( flashon >= 10 )
			{
				if( gHUD.stats[STAT_ACTIVEWEAPON] == (IT_SHOTGUN << i))
					flashon = 1;
				else flashon = 0;
			}
			else flashon = (flashon % 5) + 2;

			DrawPic( i * 24, -16, sb_weapons[flashon][i] );
		}
	}

	// MED 01/04/97
	// hipnotic weapons
	if (gametype == GAMETYPE_HIPNOTIC)
	{
		int grenadeflashing = 0;
		for( i = 0; i < 4; i++ )
		{
			if( gHUD.items & ( 1 << hipweapons[i] ))
			{
				time = gHUD.item_gettime[i];
				flashon = (int)((flTime - time) * 10);
				if( flashon >= 10 )
				{
					if( gHUD.stats[STAT_ACTIVEWEAPON] == ( 1 << hipweapons[i] ))
						flashon = 1;
					else flashon = 0;
				}
				else flashon = (flashon % 5) + 2;

				// check grenade launcher
				if( i == 2 )
				{
					if (gHUD.items & HIT_PROXIMITY_GUN)
					{
						if (flashon)
						{
							grenadeflashing = 1;
							DrawPic (96, -16, hsb_weapons[flashon][2]);
						}
					}
				}
				else if( i == 3 )
				{
					if( gHUD.items & (IT_SHOTGUN << 4))
					{
						if (flashon && !grenadeflashing)
						{
							DrawPic (96, -16, hsb_weapons[flashon][3]);
						}
						else if (!grenadeflashing)
						{
							DrawPic (96, -16, hsb_weapons[0][3]);
						}
					}
					else
					{
						DrawPic (96, -16, hsb_weapons[flashon][4]);
					}
				}
				else
				{
					DrawPic (176 + (i*24), -16, hsb_weapons[flashon][i]);
				}
			}
		}
	}

	if (gametype == GAMETYPE_ROGUE)
	{
		// check for powered up weapon.
		if( gHUD.stats[STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for( i = 0; i < 5; i++ )
			{
				if( gHUD.stats[STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
				{
					DrawPic ((i+2)*24, -16, rsb_weapons[i]);
				}
			}
		}
	}

	// ammo counts
	for( i = 0; i < 4; i++ )
	{
		sprintf( num, "%3i", gHUD.stats[STAT_SHELLS+i] );

		if (num[0] != ' ')
			DrawCharacter((6 * i + 1) * 8 - 2, -26, num[0] );
		if (num[1] != ' ')
			DrawCharacter((6 * i + 2) * 8 - 2, -26, num[1] );
		if (num[2] != ' ')
			DrawCharacter((6 * i + 3) * 8 - 2, -26, num[2] );
	}

	flashon = 0;

	// items
	for( i = 0; i < 6; i++ )
          {
		if (gHUD.items & (1<<(17 + i)))
		{
			time = gHUD.item_gettime[17+i];

			if( time && time > flTime - 2 && flashon )
         			{
         				// flash frame
			}
			else
			{
				//MED 01/04/97 changed keys
				if( gametype != GAMETYPE_HIPNOTIC || (i > 1))
				{
					DrawPic( 192 + i * 16, -16, sb_items[i] );
				}
			}
		}
	}

	//MED 01/04/97 added hipnotic items
	// hipnotic items
	if (gametype == GAMETYPE_HIPNOTIC)
	{
		for (i=0 ; i<2 ; i++)
		{
			if (gHUD.items & (1<<(24+i)))
			{
				time = gHUD.item_gettime[24+i];
				if( time && time > flTime - 2 && flashon )
				{
					// flash frame
				}
				else
				{
					DrawPic (288 + i*16, -16, hsb_items[i]);
				}
			}
		}
	}

	if (gametype == GAMETYPE_ROGUE)
	{
		// new rogue items
		for( i = 0; i < 2; i++ )
		{
			if( gHUD.items & (1 << (29 + i)))
			{
				time = gHUD.item_gettime[29+i];

				if( time && time > flTime - 2 && flashon )
				{	
					// flash frame
				}
				else
				{
					DrawPic (288 + i*16, -16, rsb_items[i]);
				}
			}
		}
	}
	else
	{
		// sigils
		for( i = 0; i < 4; i++ )
		{
			if (gHUD.items & (1<<( 28 + i )))
			{
				time = gHUD.item_gettime[28+i];
				if( time && time > flTime - 2 && flashon )
         				{
         					// flash frame
				}
				else
				{
					DrawPic( 320 - 32 + i * 8, -16, sb_sigil[i] );
				}
			}
		}
	}
}

int CHudSBar::Draw(float fTime)
{
	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HUD )
		return 1;

	for( int i = 0; i < SHOWLMP_MAXLABELS; i++ )
	{
		if( showlmp[i].active )
			DrawTransPic( showlmp[i].x, showlmp[i].y, showlmp[i].pic, true );
	}

	if (!gHUD.m_iIntermission)
	{
		completed_time = fTime;
	}
	else if (gHUD.m_iIntermission == 1)
	{
		DrawIntermission( fTime );
		return 1;
	}
	else if (gHUD.m_iIntermission == 2)
	{
		DrawFinale( fTime );
		return 1;
	}

	if (gHUD.sb_lines > 24)
	{
		DrawInventory( fTime );
	}

	if ((gEngfuncs.GetMaxClients() == 1 ) && (gHUD.showscores || gHUD.stats[STAT_HEALTH] <= 0))
	{
		if (gHUD.sb_lines)
			DrawPic( 0, 0, sb_scorebar );
		DrawScoreBoard ( fTime );
	}
	else if (gHUD.sb_lines)
	{
		DrawPic( 0, 0, sb_sbar );
	}

	if ((gEngfuncs.GetMaxClients() == 1 ) && (gHUD.showscores || gHUD.stats[STAT_HEALTH] <= 0))
		return 1;
#if 0
	if (gHUD.sb_lines <= 0)
		return 1;
#endif
	DrawFace( fTime );

	// health
	DrawNum( 136, 0, gHUD.stats[STAT_HEALTH], 3, gHUD.stats[STAT_HEALTH] <= 25 );

	// keys (hipnotic only)
	//MED 01/04/97 moved keys here so they would not be overwritten
	if (gametype == GAMETYPE_HIPNOTIC)
	{
		if (gHUD.items & IT_KEY1)
			DrawPic (209, 3, sb_items[0]);
		if (gHUD.items & IT_KEY2)
			DrawPic (209, 12, sb_items[1]);
	}

	// armor
	if (gHUD.items & IT_INVULNERABILITY)
	{
		DrawNum( 24, 0, 666, 3, 1 );
		DrawPic( 0, 0, sb_items[3] );
	}
	else
	{
		if (gametype == GAMETYPE_ROGUE)
		{
			DrawNum( 24, 0, gHUD.stats[STAT_ARMOR], 3, gHUD.stats[STAT_ARMOR] <= 25 );

			if (gHUD.items & RIT_ARMOR3)
				DrawPic (0, 0, sb_armor[2]);
			else if (gHUD.items & RIT_ARMOR2)
				DrawPic (0, 0, sb_armor[1]);
			else if (gHUD.items & RIT_ARMOR1)
				DrawPic (0, 0, sb_armor[0]);
		}
		else
		{
			DrawNum( 24, 0, gHUD.stats[STAT_ARMOR], 3, gHUD.stats[STAT_ARMOR] <= 25 );

			if (gHUD.items & IT_ARMOR3)
				DrawPic( 0, 0, sb_armor[2] );
			else if (gHUD.items & IT_ARMOR2)
				DrawPic( 0, 0, sb_armor[1] );
			else if (gHUD.items & IT_ARMOR1)
				DrawPic( 0, 0, sb_armor[0] );
		}
	}

	// ammo icon
	if (gametype == GAMETYPE_ROGUE)
	{
		if (gHUD.items & RIT_SHELLS)
			DrawPic (224, 0, sb_ammo[0]);
		else if (gHUD.items & RIT_NAILS)
			DrawPic (224, 0, sb_ammo[1]);
		else if (gHUD.items & RIT_ROCKETS)
			DrawPic (224, 0, sb_ammo[2]);
		else if (gHUD.items & RIT_CELLS)
			DrawPic (224, 0, sb_ammo[3]);
		else if (gHUD.items & RIT_LAVA_NAILS)
			DrawPic (224, 0, rsb_ammo[0]);
		else if (gHUD.items & RIT_PLASMA_AMMO)
			DrawPic (224, 0, rsb_ammo[1]);
		else if (gHUD.items & RIT_MULTI_ROCKETS)
			DrawPic (224, 0, rsb_ammo[2]);

		// g-cont. hide ammo count on axe
		if (gHUD.stats[STAT_ACTIVEWEAPON] != 2048)
			DrawNum( 248, 0, gHUD.stats[STAT_AMMO], 3, gHUD.stats[STAT_AMMO] <= 10 );
	}
	else
	{
		if (gHUD.items & IT_SHELLS)
			DrawPic( 224, 0, sb_ammo[0] );
		else if (gHUD.items & IT_NAILS)
			DrawPic( 224, 0, sb_ammo[1] );
		else if (gHUD.items & IT_ROCKETS)
			DrawPic( 224, 0, sb_ammo[2] );
		else if (gHUD.items & IT_CELLS)
			DrawPic( 224, 0, sb_ammo[3] );

		// g-cont. hide ammo count on axe
		if (gHUD.stats[STAT_ACTIVEWEAPON] != IT_AXE)
			DrawNum( 248, 0, gHUD.stats[STAT_AMMO], 3, gHUD.stats[STAT_AMMO] <= 10 );
	}

	return 1;
}

int CHudSBar::MsgFunc_Stats(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int statnum = READ_BYTE();

	if( statnum < 0 || statnum >= MAX_STATS )
	{
		gEngfuncs.Con_Printf( "gmsgStats: bad stat %i\n", statnum );
		return 0;
	}

	// update selected stat
	gHUD.stats[statnum] = (unsigned int)READ_LONG();

	// animate viewmodel
	if( statnum == STAT_WEAPONFRAME )
	{
		cl_entity_t	*view = gEngfuncs.GetViewModel();
		float		flTime = gEngfuncs.GetClientTime();

		// HACKHACK: to properly lerping first frame
		if(( view->curstate.animtime + 0.1f ) < flTime )
			view->latched.prevanimtime = flTime - 0.1f;
		else view->latched.prevanimtime = view->curstate.animtime;
		view->curstate.animtime = flTime;
		view->latched.prevframe = view->curstate.frame;
		view->curstate.frame = gHUD.stats[statnum];
	}

	return 1;
}

int CHudSBar::MsgFunc_Items(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	unsigned int newItems = (unsigned int)READ_LONG();

	if (gHUD.items != newItems)
	{
		// set flash times
		for( int i = 0; i < 32; i++ )
			if(( newItems & ( 1<<i )) && !( gHUD.items & ( 1<<i )))
				gHUD.item_gettime[i] = gEngfuncs.GetClientTime();
		gHUD.items = newItems;
	}

	return 1;
}

int CHudSBar::MsgFunc_LevelName( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	strncpy( levelname, READ_STRING(), sizeof( levelname ) - 1 );

	return 1;
}

int CHudSBar::MsgFunc_FoundSecret( const char *pszName, int iSize, void *pbuf )
{
	gHUD.stats[STAT_SECRETS]++;
	return 1;
}

int CHudSBar::MsgFunc_KillMonster( const char *pszName, int iSize, void *pbuf )
{
	gHUD.stats[STAT_MONSTERS]++;
	return 1;
}

int CHudSBar::MsgFunc_ShowLMP( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	char lmplabel[256], picname[256];
	Q_strcpy( lmplabel, READ_STRING( ));
	Q_strcpy( picname, READ_STRING( ));
	float x = READ_BYTE();
	float y = READ_BYTE();
	int k = -1;
	for( int i = 0; i < SHOWLMP_MAXLABELS; i++ )
	{
		if( showlmp[i].active )
		{
			if( !Q_strcmp( showlmp[i].label, lmplabel ))
			{
				k = i;
				break; // drop out to replace it
			}
		}
		else if( k < 0 ) // find first empty one to replace
			k = i;
	}

	if( k < 0 ) return 1; // none found to replace

	// change existing one
	showlmp[k].active = true;
	Q_strcpy( showlmp[k].label, lmplabel );
	showlmp[k].pic = gHUD.DrawPicFromWad( picname, true );
	showlmp[k].x = x;
	showlmp[k].y = y;

	return 1;
}

int CHudSBar::MsgFunc_HideLMP( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	char *lmplabel = READ_STRING();

	for( int i = 0; i < SHOWLMP_MAXLABELS; i++ )
	{
		if( showlmp[i].active && !Q_strcmp( showlmp[i].label, lmplabel ))
		{
			showlmp[i].active = false;
			return 1;
		}
	}

	return 1;
}