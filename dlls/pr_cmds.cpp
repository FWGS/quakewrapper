/*
pr_cmds.cpp - Quake virtual machine wrapper impmenentation
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
#include "saverestore.h"
#include "com_model.h"
#include "client.h"
#include "game.h"
#include "progs.h"

/*
=======================================================================

		    PROGS EXTENSIONS

=======================================================================
*/
const char *pr_extensions =
"DP_QC_TRACEBOX "
"DP_QC_RANDOMVEC "
"DP_QC_MINMAXBOUND "
"DP_QC_SINCOSSQRTPOW "
;

/*
=======================================================================

		    PROGS COMMON ROUTINES

=======================================================================
*/
/*
=================
PR_ValidateArgs

check arguments count
=================
*/
bool PR_ValidateArgs( const char *builtin, int num_argc )
{
	if( pr.argc < num_argc )
	{
		ALERT( at_warning, "%s called with too few parameters\n", builtin );
		return false;
	}
	else if( pr.argc > num_argc )
	{
		ALERT( at_warning, "%s called with too many parameters\n", builtin );
		return false;
	}

	return true;
}

/*
=================
PR_ValidateString

make sure what string is valid
=================
*/
void _PR_ValidateString( const char *s, const char *filename, const int fileline )
{
	if( s[0] <= ' ' ) HOST_ERROR( "bad string (called at %s:%i)\n", filename, fileline );
}

/*
=================
PR_SetTraceGlobals

share trace result with progs
=================
*/
void PR_SetTraceGlobals( void )
{
	pr.global_struct->trace_allsolid = gpGlobals->trace_allsolid;
	pr.global_struct->trace_startsolid = gpGlobals->trace_startsolid;
	pr.global_struct->trace_fraction = gpGlobals->trace_fraction;
	pr.global_struct->trace_endpos = gpGlobals->trace_endpos;
	pr.global_struct->trace_plane_normal = gpGlobals->trace_plane_normal;
	pr.global_struct->trace_plane_dist =  gpGlobals->trace_plane_dist;
	pr.global_struct->trace_ent = ENTINDEX( gpGlobals->trace_ent );
	pr.global_struct->trace_inopen = gpGlobals->trace_inopen;
	pr.global_struct->trace_inwater = gpGlobals->trace_inwater;
}

// kind of helper function
static bool PR_checkextension( const char *name )
{
	const char	*e, *start;
	int		len = (int)Q_strlen( name );

	for( e = pr_extensions; *e; e++ )
	{
		while( *e == ' ' )
			e++;
		if( !*e ) break;

		start = e;
		while( *e && *e != ' ' )
			e++;

		if(( e - start ) == len && !Q_strnicmp( start, name, len ))
			return true;
	}

	return false;
}

/*
===============================================================================

			BUILT-IN FUNCTIONS

===============================================================================
*/
/*
=========
PF_VarArgs

supports follow prefixes:
%d - integer or bool (declared as float)
%i - integer or bool (declared as float)
%f - float
%g - formatted float with cutoff zeroes
%s - string
%p - function pointer (will be printed function name)
%e - entity (will be print entity number) - just in case
%v - vector (format: %g %g %g)
%x - print flags as hexadecimal
FIXME: add support for PF_VarString and replace them
=========
*/
const char *PF_VarArgs( int start_arg )
{
	static char	vm_string[MAX_VAR_STRING];
	int		arg = start_arg + 1;// skip format string	
	static char	vm_arg[MAX_VAR_ARG];
	char		*out, *outend;
	const char	*s, *m;
	dfunction_t	*func;
	float		*vec;

	// get the format string
	s = G_STRING( OFS_PARM0 + start_arg * 3 );
	out = vm_string;
	outend = out + MAX_VAR_STRING - 1;
	vm_string[0] = '\0';

	while( out < outend && *s )
	{
		if( arg > pr.argc )
			break;

		if( *s != '%' )
		{
			*out++ = *s++;
			continue;
		}

		switch( (int)s[1] )
		{
		case 'd': Q_snprintf( vm_arg, MAX_VAR_ARG, "%d", (int)G_FLOAT( OFS_PARM0 + arg * 3 )); break;
		case 'i': Q_snprintf( vm_arg, MAX_VAR_ARG, "%i", (int)G_FLOAT( OFS_PARM0 + arg * 3 )); break;
		case 'x': Q_snprintf( vm_arg, MAX_VAR_ARG, "%p", (int)G_FLOAT( OFS_PARM0 + arg * 3 )); break;
		case 's': Q_snprintf( vm_arg, MAX_VAR_ARG, "%s", G_STRING( OFS_PARM0 + arg * 3 )); break;
		case 'f': Q_snprintf( vm_arg, MAX_VAR_ARG, "%f", G_FLOAT( OFS_PARM0 + arg * 3 )); break;
		case 'g': Q_snprintf( vm_arg, MAX_VAR_ARG, "%g", G_FLOAT( OFS_PARM0 + arg * 3 )); break;
		case 'e': Q_snprintf( vm_arg, MAX_VAR_ARG, "%i", G_EDICTNUM( OFS_PARM0 + arg * 3 )); break;
		case 'p': // function ptr
			func = pr.functions + G_INT( OFS_PARM0 + arg * 3 );
			if( !func->s_name ) Q_strncpy( vm_arg, "(null)", MAX_VAR_ARG ); // MSVCRT style
			else Q_snprintf( vm_arg, MAX_VAR_ARG, "%s", STRING( func->s_name ));
			break;
		case 'v': // vector
			vec = G_VECTOR( OFS_PARM0 + arg * 3 );
			Q_snprintf( vm_arg, MAX_VAR_ARG, "%g %g %g", vec[0], vec[1], vec[2] );
			break;
		default:
			arg++; // skip invalid arg
			continue;
		}

		m = vm_arg;
		s += 2;
		arg++;

		while( out < outend && *m )
			*out++ = *m++; // copy next arg
	}

	return vm_string;
}

/*
=================
PF_VarString

simply version of PF_VarArgs
=================
*/
char *PF_VarString( int start_arg )
{
	static char	out[MAX_VAR_STRING];
	
	out[0] = '\0';

	for( int i = start_arg; i < pr.argc; i++ )
		Q_strncat( out, G_STRING( OFS_PARM0 + i * 3 ), MAX_VAR_STRING );

	return out;
}

/*
=================
PF_error

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

void error( string ... )
=================
*/
void PF_error( void )
{
	edict_t	*ed;
	char	*s;

	if( pr.precache ) return;
	
	s = PF_VarString( 0 );
	ALERT( at_console, "======SERVER ERROR in %s:\n%s\n", STRING( pr.xfunction->s_name ), s );
	ed = INDEXENT( pr.global_struct->self );
	ED_Print( ed );

	HOST_ERROR( "Program error\n" );
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

void objerror( string ... )
=================
*/
void PF_objerror( void )
{
	edict_t	*ed;
	char	*s;

	if( pr.precache ) return;
	
	s = PF_VarString( 0 );
	ALERT( at_console, "======OBJECT ERROR in %s:\n%s\n", STRING( pr.xfunction->s_name ), s );
	ed = INDEXENT( pr.global_struct->self );
	ED_Print( ed );
	REMOVE_ENTITY( ed );
}

/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
void makevectors( vector angles )
==============
*/
void PF_makevectors( void )
{
	if( !PR_ValidateArgs( "makevectors", 1 ) || pr.precache )
		return;

	MAKE_VECTORS( G_VECTOR( OFS_PARM0 ));

	pr.global_struct->v_forward = gpGlobals->v_forward;
	pr.global_struct->v_right = gpGlobals->v_right;
	pr.global_struct->v_up = gpGlobals->v_up;
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).
Directly changing origin will not set internal links correctly, so clipping would be messed up.
This should be called when an object is spawned, and then only if it is teleported.

void setorigin( entity e, vector origin )
=================
*/
void PF_setorigin( void )
{
	if( !PR_ValidateArgs( "setorigin", 2 ) || pr.precache )
		return;

	edict_t *pent = G_EDICT( OFS_PARM0 );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pent );
	pent->v.origin = pev->origin = G_VECTOR( OFS_PARM1 );

	SV_LinkEdict( pent, FALSE );
}

/*
=================
PF_setsize

the size box is rotated by the current angle

void setsize( entity e, vector mins, vector maxs )
=================
*/
void PF_setsize( void )
{
	if( !PR_ValidateArgs( "setsize", 3 ) || pr.precache )
		return;

	edict_t *pent = G_EDICT( OFS_PARM0 );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pent );

	pev->mins = pent->v.mins = G_VECTOR( OFS_PARM1 );
	pev->maxs = pent->v.maxs = G_VECTOR( OFS_PARM2 );
	pev->size = pent->v.size = (pev->maxs - pev->mins);

	SV_LinkEdict( pent, FALSE );
}

/*
=================
PF_setmodel

void setmodel( entity e, string modelpath )
=================
*/
void PF_setmodel( void )
{
	model_t *mod = NULL;

	if( !PR_ValidateArgs( "setmodel", 2 ) || pr.precache )
		return;

	edict_t *pent = G_EDICT( OFS_PARM0 );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( pent );

	if( Q_strcmp( G_STRING( OFS_PARM1 ), "" ))
	{
		int i = MODEL_INDEX( G_STRING( OFS_PARM1 ));
		if( i == 0 ) return;

		pev->model = pent->v.model = MAKE_STRING( MODEL_NAME( i ));
		pev->modelindex = pent->v.modelindex = i;

		mod = (model_t *)MODEL_HANDLE( pent->v.modelindex );
	}
	else
	{
		// Arcane Dimensions style items hiding
		pev->model = pent->v.model = iStringNull;
		pev->modelindex = pent->v.modelindex = 0;
	}

	vec3_t mins = g_vecZero;
	vec3_t maxs = g_vecZero;

	if( mod && mod->type != mod_studio )
	{
		mins = mod->mins;
		maxs = mod->maxs;
	}

	if( mod && mod->type == mod_sprite )
	{
		// FIXME: kRenderTransAlpha doesn't have lerping between frames
		// but looks better than kRenderTransAdd. What i should choose?
		pent->v.rendermode = kRenderTransAlpha;
		pent->v.renderamt = 255;
	}

	pev->mins = pent->v.mins = mins;
	pev->maxs = pent->v.maxs = maxs;
	pev->size = pent->v.size = (pev->maxs - pev->mins);

	SV_LinkEdict( pent, FALSE );
}

/*
=================
PF_bprint

broadcast print to everyone on server

void bprint( string ... )
=================
*/
void PF_bprint( void )
{
	g_engfuncs.pfnServerPrint( PF_VarString( 0 ));
}

/*
=================
PF_sprint

single print to a specific client

void sprint( entity client, string ... )
=================
*/
void PF_sprint( void )
{
	int	entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	if( entnum < 1 || entnum > gpGlobals->maxClients )
	{
		ALERT( at_warning, "tried to sprint to a non-client\n" );
		return;
	}

	CLIENT_PRINTF( G_EDICT( OFS_PARM0 ), print_console, PF_VarString( 1 ));	
}

/*
=================
PF_centerprint

single print to a specific client

void centerprint( entity client, string ... )
=================
*/
void PF_centerprint( void )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgHudText, G_EDICT( OFS_PARM0 ));
		WRITE_STRING( PF_VarString( 1 ));
	MESSAGE_END();
}

/*
=================
PF_normalize

vector normalize( vector src )
=================
*/
void PF_normalize( void )
{
	vec3_t	result;
	float	length;
	float	*vec;

	if( !PR_ValidateArgs( "normalize", 1 ))
	{
		G_VECTOR( OFS_RETURN )[0] = 0.0f;
		G_VECTOR( OFS_RETURN )[1] = 0.0f;
		G_VECTOR( OFS_RETURN )[2] = 0.0f;
		return;
	}	

	vec = G_VECTOR( OFS_PARM0 );

	length = sqrt( vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] );

	if( length == 0 )
	{
		result = g_vecZero;
	}
	else
	{
		length = ( 1.0f / length );
		result.x = vec[0] * length;
		result.y = vec[1] * length;
		result.z = vec[2] * length;
	}

	G_VECTOR( OFS_RETURN )[0] = result.x;
	G_VECTOR( OFS_RETURN )[1] = result.y;
	G_VECTOR( OFS_RETURN )[2] = result.z;
}

/*
=================
PF_vlen

float vlen( vector src )
=================
*/
void PF_vlen( void )
{
	float	*vec;

	if( !PR_ValidateArgs( "vlen", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}
	
	vec = G_VECTOR( OFS_PARM0 );
	G_FLOAT( OFS_RETURN ) = sqrt( vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] );
}

/*
=================
PF_vectoyaw

float vectoyaw( vector src )
=================
*/
void PF_vectoyaw( void )
{
	if( !PR_ValidateArgs( "vectoyaw", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = VEC_TO_YAW( G_VECTOR( OFS_PARM0 ));
}

/*
=================
PF_vectoangles

vector vectoangles( vector src )
=================
*/
void PF_vectoangles( void )
{
	float	rgflVecOut[3];

	if( !PR_ValidateArgs( "vectoangles", 1 ))
	{
		G_VECTOR( OFS_RETURN )[0] = 0.0f;
		G_VECTOR( OFS_RETURN )[1] = 0.0f;
		G_VECTOR( OFS_RETURN )[2] = 0.0f;
		return;
	}

	VEC_TO_ANGLES( G_VECTOR( OFS_PARM0 ), rgflVecOut );

	G_VECTOR( OFS_RETURN )[0] = rgflVecOut[0];
	G_VECTOR( OFS_RETURN )[1] = rgflVecOut[1];
	G_VECTOR( OFS_RETURN )[2] = rgflVecOut[2];
}

/*
=================
PF_Random

Returns a number from 0 <= num < 1

float random( void )
=================
*/
void PF_random( void )
{
//	G_FLOAT( OFS_RETURN ) = (rand() & 0x7fff) / ((float)0x7fff);
	G_FLOAT( OFS_RETURN ) = (RANDOM_LONG( 0, 0x7fff ) / ((float)0x7fff));
}

/*
=================
PF_particle

void particle( vector origin, vector direction, float color, float count )
=================
*/
void PF_particle( void )
{
	if( !PR_ValidateArgs( "particle", 4 ))
		return;

	PARTICLE_EFFECT( G_VECTOR( OFS_PARM0 ), G_VECTOR( OFS_PARM1 ), G_FLOAT( OFS_PARM2 ), G_FLOAT( OFS_PARM3 ));
}

/*
=================
PF_ambientsound

void ambientsound( vector pos, string sample, float volume, float attenuation )
=================
*/
void PF_ambientsound( void )
{
	if( !PR_ValidateArgs( "ambientsound", 4 ))
		return;

	EMIT_AMBIENT_SOUND( NULL, G_VECTOR( OFS_PARM0 ), G_STRING( OFS_PARM1 ), G_FLOAT( OFS_PARM2 ), G_FLOAT( OFS_PARM3 ), 0, PITCH_NORM );
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

sound( entity e, float channel, string sample, float volume, float attenuation )
=================
*/
void PF_sound( void )
{
	if( !PR_ValidateArgs( "sound", 5 ))
		return;

	ED_UpdateEdictFields( G_EDICT( OFS_PARM0 ));

	EMIT_SOUND( G_EDICT( OFS_PARM0 ), (int)G_FLOAT( OFS_PARM1 ), G_STRING( OFS_PARM2 ), G_FLOAT( OFS_PARM3 ), G_FLOAT( OFS_PARM4 ));
}

/*
=================
PF_break

void break( void )
=================
*/
void PF_break( void )
{
	PR_RunError( "break statement" );
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

void traceline( vector src, vector dst, float nomonsters, entity ignore )
=================
*/
void PF_traceline( void )
{
	TraceResult	dummy;

	if( !PR_ValidateArgs( "traceline", 4 ) || pr.precache )
		return;

	TRACE_LINE( G_VECTOR( OFS_PARM0 ), G_VECTOR( OFS_PARM1 ), ( G_FLOAT( OFS_PARM2 ) ? TRUE : FALSE ), G_EDICT( OFS_PARM3 ), &dummy );

	PR_SetTraceGlobals();
}

/*
=================
PF_tracebox

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

tracebox (vector1, vector mins, vector maxs, vector2, tryents)
=================
*/
// LordHavoc: added this for my own use, VERY useful, similar to traceline
void PF_tracebox( void )
{
	if( !PR_ValidateArgs( "traceline", 6 ) || pr.precache )
		return;

	TRACE( G_VECTOR(OFS_PARM0), G_VECTOR(OFS_PARM1), G_VECTOR(OFS_PARM2), G_VECTOR(OFS_PARM3), G_FLOAT(OFS_PARM4), G_EDICT(OFS_PARM5));

	PR_SetTraceGlobals();
}

void PF_tracetoss( void )
{
	TraceResult	trace;

	if( !PR_ValidateArgs( "tracetoss", 2 ) || pr.precache )
		return;

	TRACE_TOSS( G_EDICT( OFS_PARM0 ), G_EDICT( OFS_PARM1 ), &trace );

	PR_SetTraceGlobals();
}

/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
float checkpos( entity e, vector pos )
=================
*/
void PF_checkpos( void )
{
	if( !PR_ValidateArgs( "checkpos", 2 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = 0;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

entity checkclient( void )
=================
*/
void PF_checkclient( void )
{
	edict_t	*ed;

	if( pr.precache )
	{
		RETURN_EDICT( INDEXENT( 0 ));
		return;
	}

	ed = INDEXENT( pr.global_struct->self );
	ED_UpdateEdictFields( ed );
	ed = FIND_CLIENT_IN_PVS( ed );

	if( ed->v.health <= 0.0f )
		RETURN_EDICT( INDEXENT( 0 ));
	else RETURN_EDICT( ed );
}

//============================================================================
/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

void stuffcmd( entity client, string cmd )
=================
*/
void PF_stuffcmd( void )
{
	if( !PR_ValidateArgs( "stuffcmd", 2 ))
		return;

	// goes directly to client
	MESSAGE_BEGIN( MSG_ONE, svc_stufftext, G_EDICT( OFS_PARM0 ));
		WRITE_STRING( G_STRING( OFS_PARM1 ));	
	MESSAGE_END();
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

void localcmd( string cmd )
=================
*/
void PF_localcmd( void )
{
	const char	*s;

	if( !PR_ValidateArgs( "localcmd", 1 ))
		return;

	if( !Q_strncmp( G_STRING( OFS_PARM0 ), "restart", 7 ))
		s = "reload\n";
	else s = G_STRING( OFS_PARM0 );

	SERVER_COMMAND( s );
}

/*
=================
PF_cvar

float cvar( string name )
=================
*/
void PF_cvar( void )
{
	if( !PR_ValidateArgs( "cvar", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	// special check for register
	if( !Q_strcmp( G_STRING( OFS_PARM0 ), "registered" ))
	{
		G_FLOAT( OFS_RETURN ) = g_registered;
	}
	else if( !Q_strcmp( G_STRING( OFS_PARM0 ), "developer" ))
	{
		float developer = CVAR_GET_FLOAT( G_STRING( OFS_PARM0 ));
		G_FLOAT( OFS_RETURN ) = ( developer <= 3.0f ) ? 0.0f : 1.0f;
	}
	else if( !Q_strcmp( G_STRING( OFS_PARM0 ), "r_wateralpha" ))
	{
		G_FLOAT( OFS_RETURN ) = CVAR_GET_FLOAT( "sv_wateralpha" );
	}
	else if( !Q_strcmp( G_STRING( OFS_PARM0 ), "cl_rollangle" ))
	{
		G_FLOAT( OFS_RETURN ) = CVAR_GET_FLOAT( "sv_rollangle" );
	}
	else
	{
		G_FLOAT( OFS_RETURN ) = CVAR_GET_FLOAT( G_STRING( OFS_PARM0 ));
	}
}

/*
=================
PF_cvar_set

void cvar_set( string name, string value )
=================
*/
void PF_cvar_set( void )
{
	if( !PR_ValidateArgs( "cvar_set", 2 ))
		return;

	if( !Q_strcmp( G_STRING( OFS_PARM0 ), "r_wateralpha" ))
	{
		CVAR_SET_STRING( "sv_wateralpha", G_STRING( OFS_PARM1 ));
	}
	else if( !Q_strcmp( G_STRING( OFS_PARM0 ), "cl_rollangle" ))
	{
		CVAR_SET_STRING( "sv_rollangle", G_STRING( OFS_PARM1 ));
	}
	else
	{
		CVAR_SET_STRING( G_STRING( OFS_PARM0 ), G_STRING( OFS_PARM1 ));
	}
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

entity findradius( vector origin, float radius )
=================
*/
void PF_findradius( void )
{
	edict_t		*ent, *chain;
	float		rad;
	float		*org;
	vec3_t		eorg;
	pr_entvars_t	*ev;

	if( !PR_ValidateArgs( "findradius", 2 ))
	{
		RETURN_EDICT( INDEXENT( 0 ));
		return;
	}

	org = G_VECTOR( OFS_PARM0 );
	rad = G_FLOAT( OFS_PARM1 );
	chain = INDEXENT( 0 );

	for( int e = 1; e < gpGlobals->maxEntities; e++ )
	{
		if(( ent = INDEXENT( e )) == NULL )
			break; // end of list

		if( ent->free || !GET_PRIVATE( ent ))
			continue;

		ev = (pr_entvars_t *)GET_PRIVATE( ent );

		if( ev->solid == SOLID_NOT )
			continue;

		for( int j = 0; j < 3; j++ )
			eorg[j] = org[j] - (ev->origin[j] + (ev->mins[j] + ev->maxs[j]) * 0.5f);			

		if( eorg.Length() > rad )
			continue;
			
		ev->chain = ENTINDEX( chain );
		chain = ent;
	}

	RETURN_EDICT( chain );
}

/*
=========
PF_dprint

print message into console if developer-mode are enabled

void dprint( string ... )
=========
*/
void PF_dprint( void )
{
	ALERT( at_aiconsole, "%s", PF_VarString( 0 ));
}

/*
=========
PF_ftos

converts float to string

string ftos( float value )
=========
*/
void PF_ftos( void )
{
	if( !PR_ValidateArgs( "ftos", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	float	v = G_FLOAT( OFS_PARM0 );
	
	if( v != (int)v ) Q_snprintf( pr.string_temp, sizeof( pr.string_temp ), "%g", v );
	else Q_snprintf( pr.string_temp, sizeof( pr.string_temp ), "%d",(int)v );

	G_INT( OFS_RETURN ) = MAKE_STRING( pr.string_temp );
}

/*
=========
PF_fabs

return abs value

float fabs( float value )
=========
*/
void PF_fabs( void )
{
	if( !PR_ValidateArgs( "fabs", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = fabs( G_FLOAT( OFS_PARM0 ));
}

/*
=========
PF_vtos

convert vector to string

string vtos( vector src )
=========
*/
void PF_vtos( void )
{
	if( !PR_ValidateArgs( "vtos", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	Q_snprintf( pr.string_temp, sizeof( pr.string_temp ), "'%5.1f %5.1f %5.1f'",
	G_VECTOR( OFS_PARM0 )[0], G_VECTOR( OFS_PARM0 )[1], G_VECTOR( OFS_PARM0 )[2] );
	G_INT( OFS_RETURN ) = MAKE_STRING( pr.string_temp );
}

/*
=========
PF_vtos

convert entity to string

string vtos( entity e )
=========
*/
void PF_etos( void )
{
	if( !PR_ValidateArgs( "etos", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	Q_snprintf( pr.string_temp, sizeof( pr.string_temp ), "entity %i", G_EDICTNUM( OFS_PARM0 ));
	G_INT( OFS_RETURN ) = MAKE_STRING( pr.string_temp );
}

/*
=========
PR_stof

float stof(...[string])
=========
*/
void PR_stof( void )
{
	G_FLOAT( OFS_RETURN ) = atof( PF_VarString( 0 ));
}

/*
=========
PF_spawn

allocate new entity

entity spawn( void )
=========
*/
void PF_spawn( void )
{
	edict_t *ed = CREATE_ENTITY();

	// progs have constant class size for each entity
	ALLOC_PRIVATE( ed, pr.progs->entityfields * 4 );

	ed->v.pContainingEntity = ed; // re-link

	if( pr.precache )
	{
		pr.moved_edict[pr.num_moved] = ed;
		pr.num_moved++;
	}

	RETURN_EDICT( ed );
}

/*
=========
PF_remove

free specified entity

void remove( entity e )
=========
*/
void PF_remove( void )
{
	if( !PR_ValidateArgs( "remove", 1 ) || pr.precache )
		return;

	edict_t *ent = G_EDICT( OFS_PARM0 );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );

	// NOTE: we can't remove entity immediately because
	// we using non-solid array with dynamically reallocated pointers
	if( GET_SERVER_STATE() == SERVER_ACTIVE )
	{
		SetBits( pev->flags, FL_KILLME );
		ent->v.flags = pev->flags;
	}
	else REMOVE_ENTITY( G_EDICT( OFS_PARM0 ));

}

/*
=========
PF_find

find entity by specified value for field

entity find( entity start, .string field, string match )
=========
*/
void PF_find( void )
{
	const char	*s, *t;
	int		e, f;
	edict_t		*ent;
	pr_entvars_t	*ev;

	if( !PR_ValidateArgs( "find", 3 ))
	{
		RETURN_EDICT( INDEXENT( 0 ));
		return;
	}

	e = G_EDICTNUM( OFS_PARM0 );
	f = G_INT( OFS_PARM1 );
	s = G_STRING( OFS_PARM2 );
	if( !s ) PR_RunError( "find: bad search string" );

	for( e++; e < gpGlobals->maxEntities; e++ )		
	{
		if(( ent = INDEXENT( e )) == NULL )
			break; // end of list

		if( ent->free || !GET_PRIVATE( ent ))
			continue;

		ev = (pr_entvars_t *)GET_PRIVATE( ent );

		t = E_STRING( ev, f );
		if( !t ) continue;

		if( !Q_strcmp( t, s ))
		{
			RETURN_EDICT( ent );
			return;
		}
	}

	RETURN_EDICT( INDEXENT( 0 ));
}

/*
=================
PF_precache_file

precache_file is only used to copy files with qcc, it does nothing

string precache_file( string filepath )
=================
*/
void PF_precache_file( void )
{	
	if( !PR_ValidateArgs( "precache_file", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	// g-cont. Hey, now i'm have PRECACHE_FILE :-)
	PR_ValidateString( G_STRING( OFS_PARM0 ));
	PRECACHE_FILE( G_STRING( OFS_PARM0 ));
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
}

/*
=================
PF_precache_sound

precache specified sound

string precache_sound( string soundpath )
=================
*/
void PF_precache_sound( void )
{
	if( !PR_ValidateArgs( "precache_sound", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	PR_ValidateString( G_STRING( OFS_PARM0 ));
	PRECACHE_SOUND( G_STRING( OFS_PARM0 ));
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
}

/*
=================
PF_precache_file

precache_file is only used to copy files with qcc, it does nothing

string precache_model( string modelpath )
=================
*/
void PF_precache_model( void )
{
	if( !PR_ValidateArgs( "precache_model", 1 ))
	{
		G_INT( OFS_RETURN ) = MAKE_STRING( "" );
		return;
	}

	PR_ValidateString( G_STRING( OFS_PARM0 ));
	PRECACHE_MODEL( G_STRING( OFS_PARM0 ));
	G_INT( OFS_RETURN ) = G_INT( OFS_PARM0 );
}

/*
=========
PF_coredump

print info about all active edicts

void coredump( void )
=========
*/
void PF_coredump( void )
{
	ED_PrintEdicts();
}

/*
=========
PF_coredump

enable progs trace

void traceon( void )
=========
*/
void PF_traceon( void )
{
	pr.trace = true;
}

/*
=========
PF_coredump

disable progs trace

void traceoff( void )
=========
*/
void PF_traceoff( void )
{
	pr.trace = false;
}

/*
=========
PF_eprint

print info about specified edict

void eprint( entity e )
=========
*/
void PF_eprint( void )
{
	if( !PR_ValidateArgs( "eprint", 1 ))
		return;

	ED_Print( G_EDICT( OFS_PARM0 ));
}

/*
===============
PF_walkmove

try to move entity in specified direction

float walkmove( float yaw, float dist )
===============
*/
void PF_walkmove( void )
{
	edict_t		*ent;
	dfunction_t	*oldf;
	int 		oldself;

	if( !PR_ValidateArgs( "walkmove", 2 ) || pr.precache )
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	ent = INDEXENT( pr.global_struct->self );

	// save program state, because SV_movestep may call other progs
	oldself = pr.global_struct->self;
	oldf = pr.xfunction;

	G_FLOAT( OFS_RETURN ) = SV_WalkMove( ent, G_FLOAT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ));

	// restore program state
	pr.global_struct->self = oldself;
	pr.xfunction = oldf;
}

/*
===============
PF_movetogoal

try to move entity to another entity

float movetogoal( float dist )
===============
*/
void PF_movetogoal( void )
{
	edict_t		*ent, *goal;

	G_FLOAT( OFS_RETURN ) = 0;

	if( !PR_ValidateArgs( "movetogoal", 1 ) || pr.precache )
		return;

	ent = INDEXENT( pr.global_struct->self );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );
	goal = INDEXENT( pev->goalentity );

	G_FLOAT( OFS_RETURN ) = SV_MoveToGoal( ent, goal, G_FLOAT( OFS_PARM0 ));
}

/*
===============
PF_droptofloor

drop entity to floor

void droptofloor( void )
===============
*/
void PF_droptofloor( void )
{
	edict_t		*pent = INDEXENT( pr.global_struct->self );
	pr_entvars_t	*pev = (pr_entvars_t *)GET_PRIVATE( pent );
	TraceResult	trace;
	vec3_t		end;

	G_FLOAT( OFS_RETURN ) = 0.0f;

	if( pr.precache ) return;

	end = pev->origin;
	end.z -= 256;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	TRACE_MONSTER_HULL( pent, pev->origin, end, FALSE, pent, &trace ); 

	PR_SetTraceGlobals();

	if( trace.fAllSolid )
	{
		G_FLOAT( OFS_RETURN ) = -1.0f;
		return;
	}
	if( trace.flFraction == 1.0f )
	{
		G_FLOAT( OFS_RETURN ) = 0.0f;
		return;
	}

	pev->origin = trace.vecEndPos;
	pev->groundentity = ENTINDEX( trace.pHit );
	SetBits( pev->flags, FL_ONGROUND );

	SV_LinkEdict( pent, FALSE );

	G_FLOAT( OFS_RETURN ) = 1.0f;
}

/*
===============
PF_lightstyle

change lightstyle pattern

void lighstyle( float style, string pattern )
===============
*/
void PF_lightstyle( void )
{
	if( !PR_ValidateArgs( "lightstyle", 2 ))
		return;

	if( pr.precache ) return;

	LIGHT_STYLE( G_FLOAT( OFS_PARM0 ), G_STRING( OFS_PARM1 ));
}

/*
=========
PF_rint

round float to nearest int

float rint( float value )
=========
*/
void PF_rint( void )
{
	if( !PR_ValidateArgs( "rint", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	float f = G_FLOAT( OFS_PARM0 );
	if( f > 0 ) G_FLOAT( OFS_RETURN ) = (int)(f + 0.5f);
	else G_FLOAT( OFS_RETURN ) = (int)(f - 0.5f);
}

/*
=========
PF_floor

floor value

float floor( float value )
=========
*/
void PF_floor( void )
{
	if( !PR_ValidateArgs( "floor", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = floor( G_FLOAT( OFS_PARM0 ));
}

/*
=========
PF_ceil

ceil value

float ciel( float value )
=========
*/
void PF_ceil( void )
{
	if( !PR_ValidateArgs( "ceil", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = ceil( G_FLOAT( OFS_PARM0 ));
}

/*
=============
PF_checkbottom

check if entity is on floor return true

float checkbottom( entity e )
=============
*/
void PF_checkbottom( void )
{
	if( !PR_ValidateArgs( "checkbottom", 1 ) || pr.precache )
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = SV_CheckBottom( G_EDICT( OFS_PARM0 ));
}

/*
=============
PF_pointcontents

return the contents type for a give point in world

float pointcontents( vector pos )
=============
*/
void PF_pointcontents( void )
{
	if( !PR_ValidateArgs( "pointcontents", 1 ) || pr.precache )
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = POINT_CONTENTS( G_VECTOR( OFS_PARM0 ));	
}

/*
=============
PF_nextent

get the next entity in array

entity nextent( entity e )
=============
*/
void PF_nextent( void )
{
	edict_t	*ent;

	if( !PR_ValidateArgs( "nextent", 1 ))
	{
		RETURN_EDICT( INDEXENT( 0 ));
		return;
	}	

	int i = G_EDICTNUM( OFS_PARM0 );

	while( 1 )
	{
		i++;
		ent = INDEXENT( i );

		if( !ent || i >= gpGlobals->maxEntities )
		{
			RETURN_EDICT( INDEXENT( 0 ));
			return;
		}

		RETURN_EDICT( ent );
		return;
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along

vector aim( entity e, float missilespeed )
=============
*/
void PF_aim( void )
{
	float	rgflReturn[3];

	if( !PR_ValidateArgs( "aim", 2 ) || pr.precache )
	{
		G_FLOAT( OFS_RETURN + 0 ) = pr.global_struct->v_forward.x;
		G_FLOAT( OFS_RETURN + 1 ) = pr.global_struct->v_forward.y;
		G_FLOAT( OFS_RETURN + 2 ) = pr.global_struct->v_forward.z;
		return;
	}

	GET_AIM_VECTOR( G_EDICT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ), rgflReturn );

	G_FLOAT( OFS_RETURN + 0 ) = rgflReturn[0];
	G_FLOAT( OFS_RETURN + 1 ) = rgflReturn[1];
	G_FLOAT( OFS_RETURN + 2 ) = rgflReturn[2];
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C

void changeyaw( void )
==============
*/
void PF_changeyaw( void )
{
	edict_t	*ed = INDEXENT( pr.global_struct->self );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ed );

	pev->angles.y = UTIL_AngleMod( pev->ideal_yaw, pev->angles.y, pev->yaw_speed );
}

/*
==============
PF_changepitch

Same as changeyaw but for pitch angle

void changepitch( entity e )
==============
*/
void PF_changepitch( void )
{
	edict_t	*ent;
	float	ideal_pitch, pitch_speed;
	eval_t	*val;

	if( !PR_ValidateArgs( "changepitch", 1 ))
		return;

	ent = G_EDICT( OFS_PARM0 );
	pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );

	if( val = GETEDICTFIELDVALUE( ent, pr.eval_idealpitch ))
		ideal_pitch = val->value;
	else PR_RunError( "PF_changepitch: .float idealpitch and .float pitch_speed must be defined to use changepitch" );

	if( val = GETEDICTFIELDVALUE( ent, pr.eval_pitch_speed ))
		pitch_speed = val->value;
	else PR_RunError( "PF_changepitch: .float idealpitch and .float pitch_speed must be defined to use changepitch" );

	pev->angles.x = UTIL_AngleMod( ideal_pitch, pev->angles.x, pitch_speed );
}

/*
==============
PF_makestatic

Move entity to client to save network bandwidth

void makestatic( entity e )
==============
*/
void PF_makestatic( void )
{
	if( pr.precache ) return;

	ED_UpdateEdictFields( G_EDICT( OFS_PARM0 ));
	MAKE_STATIC( G_EDICT( OFS_PARM0 ));	
}

/*
==============
PF_setspawnparms

keep player parms in safe place during changelevel

void setspawnparms( entity client )
==============
*/
void PF_setspawnparms( void )
{
	if( !PR_ValidateArgs( "setspawnparms", 1 ) || pr.precache )
		return;

	int i = G_EDICTNUM( OFS_PARM0 );
	if( i < 1 || i > gpGlobals->maxClients )
		PR_RunError( "entity is not a client" );

	for( int j = 0; j < NUM_SPAWN_PARMS; j++ )
		(&pr.global_struct->parm1)[j] = pr.spawn_parms[i][j];
}

/*
==============
PF_changelevel

loads a specified map

void changelevel( string map )
==============
*/
void PF_changelevel( void )
{
	char	level[64];

	if( !PR_ValidateArgs( "changelevel", 1 ) || pr.precache )
		return;

	// temp place for mapname
	Q_strncpy( level, G_STRING( OFS_PARM0 ), sizeof( level ));
	pr.serverflags = pr.global_struct->serverflags;

	for( int i = 0; i < gpGlobals->maxClients; i++ )
	{
		edict_t	*pPlayer = INDEXENT( i + 1 );

		if( pPlayer->free || !pPlayer->pvPrivateData )
			continue;

		// call the progs to get default spawn parms for the new client
		pr.global_struct->self = ENTINDEX( pPlayer );
		PR_ExecuteProgram (pr.global_struct->SetChangeParms);
		for( int j = 0; j < NUM_SPAWN_PARMS; j++ )
			pr.spawn_parms[i][j] = (&pr.global_struct->parm1)[j];
		pr.changelevel[i] = true; // this client in transition...
	}

	CHANGE_LEVEL( level, NULL );
}

/*
=========
PF_sin

return sin( value )

float sin( float value )
=========
*/
void PF_sin( void )
{
	if( !PR_ValidateArgs( "sin", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT(OFS_RETURN) = sin( G_FLOAT( OFS_PARM0 ));
}

/*
=========
PF_cos

return cos( value )

float cos( float value )
=========
*/
void PF_cos( void )
{
	if( !PR_ValidateArgs( "cos", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = cos( G_FLOAT( OFS_PARM0 ));
}

/*
=========
PF_sqrt

return sqrt( value )

float sqrt( float value )
=========
*/
void PF_sqrt( void )
{
	if( !PR_ValidateArgs( "sqrt", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = sqrt( G_FLOAT( OFS_PARM0 ));
}

/*
=================
PF_randomvec

Returns a vector of length < 1 and > 0

vector randomvec()
=================
*/
void PF_randomvec( void )
{
	vec3_t		temp;

	if( !PR_ValidateArgs( "randomvec", 0 ))
	{
		G_FLOAT( OFS_RETURN + 0 ) = 0.0f;
		G_FLOAT( OFS_RETURN + 1 ) = 0.0f;
		G_FLOAT( OFS_RETURN + 2 ) = 0.0f;
		return;
	}

	// circular gaussian spread
	do
	{
		temp[0] = (rand() & 32767) * (2.0f / 32767.0f) - 1.0f;
		temp[1] = (rand() & 32767) * (2.0f / 32767.0f) - 1.0f;
		temp[2] = (rand() & 32767) * (2.0f / 32767.0f) - 1.0f;
	} while( DotProduct( temp, temp ) >= 1.0f );

	G_FLOAT( OFS_RETURN + 0 ) = temp.x;
	G_FLOAT( OFS_RETURN + 1 ) = temp.y;
	G_FLOAT( OFS_RETURN + 2 ) = temp.z;
}

/*
=================
PF_getlight

Returns a color vector indicating the lighting at the requested point.

vector getlight( vector )
FIXME: implemnt
=================
*/
void PF_getlight( void )
{
	G_FLOAT( OFS_RETURN + 0 ) = 0.0f;
	G_FLOAT( OFS_RETURN + 1 ) = 0.0f;
	G_FLOAT( OFS_RETURN + 2 ) = 0.0f;
}

/*
=========
PF_registercvar

float registercvar (string name, string value[, float flags])
=========
*/
void PF_registercvar( void )
{
	const char	*name, *value;
	int		flags;

	name = G_STRING( OFS_PARM0 );
	value = G_STRING( OFS_PARM1 );
	flags = pr.argc >= 3 ? (int)G_FLOAT( OFS_PARM2 ) : 0;
	G_FLOAT( OFS_RETURN ) = 0;

	// FIXME: undone
}

/*
=================
PF_min

returns the minimum of two supplied floats

float min( float a, float b, ...[float] )
=================
*/
void PF_min( void )
{
	// LordHavoc: 3+ argument enhancement suggested by FrikaC
	if( pr.argc >= 3 )
	{
		float f = G_FLOAT( OFS_PARM0 );
		for( int i = 1; i < pr.argc; i++ )
			if( f > G_FLOAT(( OFS_PARM0 + i * 3)))
				f = G_FLOAT(( OFS_PARM0 + i * 3 ));
		G_FLOAT( OFS_RETURN ) = f;
	}
	else G_FLOAT( OFS_RETURN ) = Q_min( G_FLOAT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ));
}

/*
=================
PF_max

returns the maximum of two supplied floats

float max( float a, float b, ...[float] )
=================
*/
void PF_max( void )
{
	// LordHavoc: 3+ argument enhancement suggested by FrikaC
	if( pr.argc >= 3 )
	{
		float f = G_FLOAT( OFS_PARM0 );
		for( int i = 1; i < pr.argc; i++ )
			if( f < G_FLOAT(( OFS_PARM0 + i * 3 )))
				f = G_FLOAT(( OFS_PARM0 + i * 3 ));
		G_FLOAT( OFS_RETURN ) = f;
	}
	else G_FLOAT( OFS_RETURN ) = Q_max( G_FLOAT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ));
}

/*
=================
PF_bound

returns number bounded by supplied range

float bound(float min, float value, float max)
=================
*/
void PF_bound( void )
{
	if( !PR_ValidateArgs( "bound", 3 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = bound( G_FLOAT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ), G_FLOAT( OFS_PARM2 ));
}

/*
=================
PF_pow

returns a raised to power b

float pow( float a, float b )
=================
*/
void PF_pow( void )
{
	if( !PR_ValidateArgs( "pow", 2 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = pow( G_FLOAT( OFS_PARM0 ), G_FLOAT( OFS_PARM1 ));
}

/*
=========
PF_findfloat

  entity	findfloat(entity start, .float field, float match)
  entity	findentity(entity start, .entity field, entity match)
=========
*/
// LordHavoc: added this for searching float, int, and entity reference fields
void PF_findfloat( void )
{
	int		e;
	int		f;
	float		s;
	edict_t		*ent;
	pr_entvars_t	*ev;

	if( !PR_ValidateArgs( "findfloat", 3 ))
	{
		RETURN_EDICT( INDEXENT( 0 ));
		return;
	}

	e = G_EDICTNUM( OFS_PARM0 );
	f = G_INT( OFS_PARM1 );
	s = G_FLOAT( OFS_PARM2 );

	for( e++; e < gpGlobals->maxEntities; e++ )
	{
		if(( ent = INDEXENT( e )) == NULL )
			break; // end of list

		if( ent->free || !GET_PRIVATE( ent ))
			continue;

		ev = (pr_entvars_t *)GET_PRIVATE( ent );

		if( E_FLOAT( ent, f ) == s )
		{
			RETURN_EDICT( ent );
			return;
		}
	}

	RETURN_EDICT( INDEXENT( 0 ));
}

/*
=================
PF_checkextension

returns true if the extension is supported by the server

float checkextension( extensionname )
=================
*/
void PF_checkextension( void )
{
	if( !PR_ValidateArgs( "checkextension", 1 ))
	{
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	G_FLOAT( OFS_RETURN ) = PR_checkextension( G_STRING( OFS_PARM0 ));
}

static builtin_t prog_builtin[] =
{
NULL,			// #0 NULL function (not callable)
PF_makevectors,		// #1 void(vector ang) makevectors
PF_setorigin,		// #2 void(entity e, vector o) setorigin
PF_setmodel,		// #3 void(entity e, string m) setmodel
PF_setsize,		// #4 void(entity e, vector min, vector max) setsize
NULL,			// #5 reserved
PF_break,			// #6 void() break
PF_random,		// #7 float() random
PF_sound,			// #8 void(entity e, float chan, string samp) sound
PF_normalize,		// #9 vector(vector v) normalize
PF_error,			// #10 void(string e) error
PF_objerror,		// #11 void(string e) objerror
PF_vlen,			// #12 float(vector v) vlen
PF_vectoyaw,		// #13 float(vector v) vectoyaw
PF_spawn,			// #14 entity() spawn
PF_remove,		// #15 void(entity e) remove
PF_traceline,		// #16 void(vector v1, vector v2, float tryents) traceline
PF_checkclient,		// #17 entity() checkclient
PF_find,			// #18 entity(entity start, .string fld, string match) find
PF_precache_sound,		// #19 void(string s) precache_sound
PF_precache_model,		// #20 void(string s) precache_model
PF_stuffcmd,		// #21 void(entity client, string s, ...) stuffcmd
PF_findradius,		// #22 entity(vector org, float rad) findradius
PF_bprint,		// #23 void(string s, ...) bprint
PF_sprint,		// #24 void(entity client, string s, ...) sprint
PF_dprint,		// #25 void(string s, ...) dprint
PF_ftos,			// #26 string(float f) ftos
PF_vtos,			// #27 string(vector v) vtos
PF_coredump,		// #28 void() coredump
PF_traceon,		// #29 void() traceon
PF_traceoff,		// #30 void() traceoff
PF_eprint,		// #31 void(entity e) eprint
PF_walkmove,		// #32 float(float yaw, float dist) walkmove
NULL,			// #33 reserved
PF_droptofloor,		// #34 float() droptofloor
PF_lightstyle,		// #35 void(float style, string value) lightstyle
PF_rint,			// #36 float(float v) rint
PF_floor,			// #37 float(float v) floor
PF_ceil,			// #38 float(float v) ceil
NULL,			// #39 reserved
PF_checkbottom,		// #40 float(entity e) checkbottom
PF_pointcontents,		// #41 float(vector v) pointcontents
NULL,			// #42 reserved
PF_fabs,			// #43 float(float f) fabs
PF_aim,			// #44 vector(entity e, float speed) aim
PF_cvar,			// #45 float(string s) cvar
PF_localcmd,		// #46 void(string s) localcmd
PF_nextent,		// #47 entity(entity e) nextent
PF_particle,		// #48 void(vector o, vector d, float color, float count) particle
PF_changeyaw,		// #49 void() ChangeYaw
NULL,			// #50 reserved
PF_vectoangles,		// #51 vector(vector v) vectoangles
PF_WriteByte,		// #52 void(float to, float f) WriteByte
PF_WriteChar,		// #53 void(float to, float f) WriteChar
PF_WriteShort,		// #54 void(float to, float f) WriteShort
PF_WriteLong,		// #55 void(float to, float f) WriteLong
PF_WriteCoord,		// #56 void(float to, float f) WriteCoord
PF_WriteAngle,		// #57 void(float to, float f) WriteAngle
PF_WriteString,		// #58 void(float to, string s) WriteString
PF_WriteEntity,		// #59 void(float to, entity e) WriteEntity
PF_sin,			// #60 float(float f) sin
PF_cos,			// #61 float(float f) cos
PF_sqrt,			// #62 float(float f) sqrt
PF_changepitch,		// #63 void(entity ent) changepitch
PF_tracetoss,		// #64 void(entity ent, entity ignore) tracetoss
PF_etos,			// #65 string(entity ent) etos
NULL,			// #66 reserved
PF_movetogoal,		// #67 void(float step) movetogoal
PF_precache_file,		// #68 string(string s) precache_file
PF_makestatic,		// #69 void(entity e) makestatic
PF_changelevel,		// #70 void(string s) changelevel
NULL,			// #71 reserved
PF_cvar_set,		// #72 void(string var, string val) cvar_set
PF_centerprint,		// #73 void(entity client, strings) centerprint
PF_ambientsound,		// #74 void(vector pos, string samp, float vol, float atten) ambientsound
PF_precache_model,		// #75 string(string s) precache_model2
PF_precache_sound,		// #76 string(string s) precache_sound2
PF_precache_file,		// #77 string(string s) precache_file2
PF_setspawnparms,		// #78 void(entity e) setspawnparms
NULL,			// #79 reserved
NULL,			// #80 reserved
PR_stof,			// #81 float( string s ) stof (FRIK_FILE)
NULL,			// #82 reserved
NULL,			// #83 reserved
NULL,			// #84 reserved
NULL,			// #85 reserved
NULL,			// #86 reserved
NULL,			// #87 reserved
NULL,			// #88 reserved
NULL,			// #89 reserved
PF_tracebox,		// #90 void(vector v1, vector min, vector max, vector v2, float nomonsters, entity forent) tracebox (DP_QC_TRACEBOX)
PF_randomvec,		// #91 vector() randomvec (DP_QC_RANDOMVEC)
PF_getlight,		// #92 vector(vector org) getlight (DP_QC_GETLIGHT)
PF_registercvar,		// #93 float(string name, string value) registercvar (DP_REGISTERCVAR)
PF_min,			// #94 float(float a, floats) min (DP_QC_MINMAXBOUND)
PF_max,			// #95 float(float a, floats) max (DP_QC_MINMAXBOUND)
PF_bound,			// #96 float(float minimum, float val, float maximum) bound (DP_QC_MINMAXBOUND)
PF_pow,			// #97 float(float f, float f) pow (DP_QC_SINCOSSQRTPOW)
PF_findfloat,		// #98 entity(entity start, .float fld, float match) findfloat (DP_QC_FINDFLOAT)
PF_checkextension,		// #99 float(string s) checkextension (the basis of the extension system)
};

/*
==============
PR_InstallBuiltins

install progs builtins
==============
*/
void PR_InstallBuiltins( void )
{
	pr.numbuiltins = ARRAYSIZE( prog_builtin );
	pr.builtins = prog_builtin;
}