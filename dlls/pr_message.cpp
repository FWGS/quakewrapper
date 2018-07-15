/*
pr_message.cpp - Quake virtual machine wrapper impmenentation
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
#include "protocol_q1.h"
#include "client.h"
#include "skill.h"
#include "game.h"
#include "progs.h"

const char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",
	"svc_setview",
	"svc_sound",
	"svc_time",
	"svc_print",
	"svc_stufftext",
	"svc_setangle",
	"svc_serverinfo",
	"svc_lightstyle",
	"svc_updatename",
	"svc_updatefrags",
	"svc_clientdata",
	"svc_stopsound",
	"svc_updatecolors",
	"svc_particle",
	"svc_damage",
	"svc_spawnstatic",
	"svc_spawnbinary",
	"svc_spawnbaseline",
	"svc_temp_entity",
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",
	"svc_cdtrack",
	"svc_sellscreen",
	"svc_cutscene",
	"svc_showlmp",
	"svc_hidelmp",
	"svc_skybox"
};

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/
void PF_WriteDest( pr_message_t *msg, int type, int dest )
{
	msg->type = type;
	msg->dest = dest;

	if( pr.global_struct->msg_entity > 0 )
		msg->entity = INDEXENT( pr.global_struct->msg_entity );
	else msg->entity = NULL;

	pr.global_struct->msg_entity = 0;
}

void PF_WriteByte( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_BYTE, G_FLOAT( OFS_PARM0 ));
	msg->integer = (byte)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteChar( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_CHAR, G_FLOAT( OFS_PARM0 ));
	msg->integer = (char)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteShort( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_SHORT, G_FLOAT( OFS_PARM0 ));
	msg->integer = (short)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteLong( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_LONG, G_FLOAT( OFS_PARM0 ));
	msg->integer = (byte)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteAngle( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_ANGLE, G_FLOAT( OFS_PARM0 ));
	msg->value = (float)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteCoord( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_COORD, G_FLOAT( OFS_PARM0 ));
	msg->value = (float)G_FLOAT( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteString( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_STRING, G_FLOAT( OFS_PARM0 ));
	msg->string = (char *)G_STRING( OFS_PARM1 );
	pr.num_messages++;
}

void PF_WriteEntity( void )
{
	if( pr.num_messages >= MAX_MESSAGES )
		return;

	pr_message_t *msg = &pr.messages[pr.num_messages];

	PF_WriteDest( msg, MSG_ENTITY, G_FLOAT( OFS_PARM0 ));
	msg->integer = (int)G_EDICTNUM( OFS_PARM1 );
	pr.num_messages++;
}

/*
===============================================================================

MESSAGE SENDING

===============================================================================
*/
void WRITE_GENERIC( pr_message_t *msg )
{
	switch( msg->type )
	{
	case MSG_BYTE:
		WRITE_BYTE( msg->integer );
		break;
	case MSG_CHAR:
		WRITE_CHAR( msg->integer );
		break;
	case MSG_SHORT:
		WRITE_SHORT( msg->integer );
		break;
	case MSG_LONG:
		WRITE_LONG( msg->integer );
		break;
	case MSG_ANGLE:
		WRITE_ANGLE( msg->value );
		break;
	case MSG_COORD:
		WRITE_COORD( msg->value );
		break;
	case MSG_STRING:
		WRITE_STRING( msg->string );
		break;
	case MSG_ENTITY:
		WRITE_ENTITY( msg->integer );
		break;
	default:
		HOST_ERROR( "PR_WriteMessageGeneric: bad type\n" );
		break;
	}
}

void PR_SendMessage( void )
{
	// clear out messages during precache
	if( pr.precache )
	{
		pr.num_messages = 0;
		return;
	}

	for( int msg_num = 0; msg_num < pr.num_messages; )
	{
		pr_message_t *msg = &pr.messages[msg_num];

		if( msg->type != MSG_BYTE )
		{
			ALERT( at_console, "PR_SendMessage: syncronization failed, message not send\n" );
			break;
		}

//		ALERT( at_console, "%s\n", svc_strings[msg->integer] );

		switch( msg->integer )
		{
		case svc_bad:
			ALERT( at_error, "PR_SendMessage: svc_bad\n" );
			pr.num_messages = 0;
			return;
		case svc_nop:
			msg_num++;
			break;
		case svc_disconnect:
			MESSAGE_BEGIN( msg->dest, svc_disconnect, msg->entity );
			MESSAGE_END();
			msg_num++;
			break;
		case svc_updatestat:
			MESSAGE_BEGIN( msg->dest, gmsgStats, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
				WRITE_LONG( pr.messages[msg_num+2].integer );	// replace long with short
			MESSAGE_END();
			msg_num += 3;
			break;
		case svc_setview:
			MESSAGE_BEGIN( msg->dest, svc_setview, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_print:
			MESSAGE_BEGIN( msg->dest, svc_print, msg->entity );
				WRITE_BYTE( 2 ); // PRINT_HIGH
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_stufftext:
			MESSAGE_BEGIN( msg->dest, svc_stufftext, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_setangle:
			if( msg->entity != NULL )
			{
				pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE(msg->entity);				
				pev->angles.x = pr.messages[msg_num+1].value;
				pev->angles.y = pr.messages[msg_num+2].value;
				pev->angles.z = pr.messages[msg_num+3].value;
				pev->fixangle = TRUE;
			}
			msg_num += 4;
			break;
		case svc_lightstyle:
			MESSAGE_BEGIN( msg->dest, svc_lightstyle, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
				WRITE_GENERIC( &pr.messages[msg_num+2] );
				WRITE_LONG( 0 );
			MESSAGE_END();
			msg_num += 3;
			break;
		case svc_particle:
			MESSAGE_BEGIN( msg->dest, svc_particle, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+2] );
				WRITE_GENERIC( &pr.messages[msg_num+3] );
				WRITE_GENERIC( &pr.messages[msg_num+4] );	// dir
				WRITE_GENERIC( &pr.messages[msg_num+5] );
				WRITE_GENERIC( &pr.messages[msg_num+6] );
				WRITE_GENERIC( &pr.messages[msg_num+7] );	// color
				WRITE_GENERIC( &pr.messages[msg_num+8] );	// count
				WRITE_BYTE( 0 );	// life, unused
			MESSAGE_END();
			msg_num += 9;
			break;
		case svc_damage:
			MESSAGE_BEGIN( msg->dest, gmsgDamage, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );	// armor
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// blood
				WRITE_GENERIC( &pr.messages[msg_num+3] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				WRITE_GENERIC( &pr.messages[msg_num+5] );			
			MESSAGE_END();
			msg_num += 6;
			break;
		case svc_temp_entity:
			MESSAGE_BEGIN( msg->dest, gmsgTempEntity, msg->entity );
			msg = &pr.messages[msg_num+1];

			if( msg->type != MSG_BYTE )
			{
				ALERT( at_warning, "PR_SendMessage: temp entity bad event type, message not send\n" );
				pr.num_messages = 0;
				return;
			}

			WRITE_GENERIC( &pr.messages[msg_num+1] );	// write TE_type

			switch( msg->integer )
			{
			case TE_SPIKE:
			case TE_SUPERSPIKE:
			case TE_GUNSHOT:
			case TE_EXPLOSION:
			case TE_TAREXPLOSION:
			case TE_WIZSPIKE:
			case TE_KNIGHTSPIKE:
			case TE_LAVASPLASH:
			case TE_TELEPORT:
			case TE_SMOKE:
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+3] );
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				msg_num += 5;
				break;
			case TE_EXPLOSION2:
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+3] );
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				WRITE_GENERIC( &pr.messages[msg_num+5] );	// color start
				WRITE_GENERIC( &pr.messages[msg_num+6] );	// color length
				msg_num += 7;
				break;
			case TE_EXPLOSION3:
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+3] );
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				WRITE_GENERIC( &pr.messages[msg_num+5] );	// dlight red
				WRITE_GENERIC( &pr.messages[msg_num+6] );	// dlight green
				WRITE_GENERIC( &pr.messages[msg_num+7] );	// dlight blue
				msg_num += 8;
				break;
			case TE_LIGHTNING1:
			case TE_LIGHTNING2:
			case TE_LIGHTNING3:
			case TE_BEAM:
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// entity to tie
				WRITE_GENERIC( &pr.messages[msg_num+3] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				WRITE_GENERIC( &pr.messages[msg_num+5] );
				WRITE_GENERIC( &pr.messages[msg_num+6] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+7] );
				WRITE_GENERIC( &pr.messages[msg_num+8] );
				msg_num += 9;
				break;
			case TE_LIGHTNING4:
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// entity to tie
				WRITE_GENERIC( &pr.messages[msg_num+3] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+4] );
				WRITE_GENERIC( &pr.messages[msg_num+5] );
				WRITE_GENERIC( &pr.messages[msg_num+6] );	// coord
				WRITE_GENERIC( &pr.messages[msg_num+7] );
				WRITE_GENERIC( &pr.messages[msg_num+8] );
				WRITE_GENERIC( &pr.messages[msg_num+9] );	// modelname
				msg_num += 10;
				break;
			}
			MESSAGE_END();
			break;
		case svc_centerprint:
			MESSAGE_BEGIN( msg->dest, gmsgHudText, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_killedmonster:
			MESSAGE_BEGIN( msg->dest, gmsgKilledMonster, msg->entity );
			MESSAGE_END();
			msg_num += 1;
			break;
		case svc_foundsecret:
			MESSAGE_BEGIN( msg->dest, gmsgFoundSecret, msg->entity );
			MESSAGE_END();
			msg_num += 1;
			break;
		case svc_intermission:
			MESSAGE_BEGIN( msg->dest, svc_intermission, msg->entity );
			MESSAGE_END();
			msg_num += 1;
			break;
		case svc_finale:
			MESSAGE_BEGIN( msg->dest, svc_finale, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_cdtrack:
			MESSAGE_BEGIN( msg->dest, svc_cdtrack, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );	// main track
				WRITE_GENERIC( &pr.messages[msg_num+2] );	// loop track
			MESSAGE_END();
			msg_num += 3;
			break;
		case svc_sellscreen:
#if 1
			g_engfuncs.pfnEndSection( "Shareware game end" );
#else
			SERVER_COMMAND( "help\n" );	// UNDONE: create help in menu
			SERVER_EXECUTE( );
#endif
			msg_num += 1;
			break;
		case svc_cutscene:
			MESSAGE_BEGIN( msg->dest, svc_finale, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		case svc_skybox:
			CVAR_SET_STRING( "sv_skyname", pr.messages[msg_num+1].string );
			msg_num += 2;
			break;
		case svc_showlmp:
			MESSAGE_BEGIN( msg->dest, gmsgShowLMP, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
				WRITE_GENERIC( &pr.messages[msg_num+2] );
				WRITE_GENERIC( &pr.messages[msg_num+3] );
				WRITE_GENERIC( &pr.messages[msg_num+4] );
			MESSAGE_END();
			msg_num += 5;
			break;
		case svc_hidelmp:
			MESSAGE_BEGIN( msg->dest, gmsgHideLMP, msg->entity );
				WRITE_GENERIC( &pr.messages[msg_num+1] );
			MESSAGE_END();
			msg_num += 2;
			break;
		default:
			if( msg->integer >= 0 && msg->integer <= svc_skybox )
				ALERT( at_error, "PR_SendMessage: unsupported message %s\n", svc_strings[msg->integer] );
			else ALERT( at_error, "PR_SendMessage: unknown message svc_%i\n", msg->integer );
			goto done;
		}
	}
done:
	if( pr.num_messages != msg_num )
		ALERT( at_error, "PR_SendMessage: parsed %i, total %i\n", msg_num, pr.num_messages );

	// clear the queue
	pr.num_messages = 0;
}