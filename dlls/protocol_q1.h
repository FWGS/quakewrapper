/*
protocol_q1.h - Quake network protocol
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

#ifndef PROTOCOL_Q1_H
#define PROTOCOL_Q1_H

//
// server to client
//
#define svc_bad			0	// compatible
#define svc_nop			1	// compatible
#define svc_disconnect		2	// compatible
#define svc_updatestat		3	// remapped
#define svc_version			4	// NOT SUPPORT
#define svc_setview			5	// compatible
#define svc_sound			6	// NOT SUPPORT
#define svc_time			7	// NOT SUPPORT
#define svc_print			8	// remapped and expanded
#define svc_stufftext		9	// compatible
#define svc_setangle		10	// NOT SUPPORT
#define svc_serverinfo		11	// NOT SUPPORT
#define svc_lightstyle		12	// expanded
#define svc_updatename		13	// NOT SUPPORT
#define svc_updatefrags		14	// NOT SUPPORT
#define svc_clientdata		15	// NOT SUPPORT
#define svc_stopsound		16	// NOT SUPPORT (but probably can it)
#define svc_updatecolors		17	// NOT SUPPORT
#define svc_particle		18	// compatible (added one extra byte at the end)
#define svc_damage			19	// remapped
#define svc_spawnstatic		20	// NOT SUPPORT
#define svc_spawnbaseline		22	// NOT SUPPORT
#define svc_temp_entity		23	// remapped
#define svc_setpause		24	// NOT SUPPORT
#define svc_signonnum		25	// NOT SUPPORT
#define svc_centerprint		26	// compatible
#define svc_killedmonster		27	// remapped
#define svc_foundsecret		28	// remapped
#define svc_spawnstaticsound		29	// NOT SUPPORT
#define svc_intermission		30	// compatible
#define svc_finale			31	// split for svc_finale and svc_centerprint
#define svc_cdtrack			32	// compatible
#define svc_sellscreen		33	// will be replaced with console command
#define svc_cutscene		34	// split for svc_cutscene and svc_centerprint

//
// Nehahra expansion
//
#define svc_showlmp			35	// [string] slotname [string] lmpfilename [coord] x [coord] y
#define svc_hidelmp			36	// [string] slotname
#define svc_skybox			37	// [string] skyname

//
// temp entity events
//
#define TE_SPIKE			0
#define TE_SUPERSPIKE		1
#define TE_GUNSHOT			2
#define TE_EXPLOSION		3
#define TE_TAREXPLOSION		4
#define TE_LIGHTNING1		5
#define TE_LIGHTNING2		6
#define TE_WIZSPIKE			7
#define TE_KNIGHTSPIKE		8
#define TE_LIGHTNING3		9
#define TE_LAVASPLASH		10
#define TE_TELEPORT			11
#define TE_EXPLOSION2		12
#define TE_BEAM			13

// NEHAHRA
#define TE_EXPLOSION3		16
#define TE_LIGHTNING4		17
#define TE_SMOKE			18	// not used

#endif//PROTOCOL_Q1_H