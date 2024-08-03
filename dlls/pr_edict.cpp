/*
pr_edict.cpp - Quake virtual machine wrapper impmenentation
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
#include "skill.h"
#include "game.h"
#include "progs.h"

const int		gProgSizes[8] = { 1, 1, 1, 3, 1, 1, 1, 1 };
prog_state_t	pr;

/*
============
ED_GlobalAtOfs
============
*/
ddef_t *ED_GlobalAtOfs( int ofs )
{
	ddef_t	*def;

	if( !pr.progs || !pr.globaldefs )
		return NULL;
	
	for( int i = 0; i < pr.progs->numglobaldefs; i++ )
	{
		def = &pr.globaldefs[i];

		if( def->ofs == ofs )
			return def;
	}

	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
ddef_t *ED_FieldAtOfs( int ofs )
{
	ddef_t	*def;

	if( !pr.progs || !pr.fielddefs )
		return NULL;
	
	for( int i = 0; i < pr.progs->numfielddefs; i++ )
	{
		def = &pr.fielddefs[i];

		if( def->ofs == ofs )
			return def;
	}

	return NULL;
}

/*
============
ED_FindField
============
*/
ddef_t *ED_FindField( const char *name )
{
	ddef_t	*def;

	if( !pr.progs || !pr.fielddefs )
		return NULL;

	for( int i = 0; i < pr.progs->numfielddefs ; i++)
	{
		def = &pr.fielddefs[i];

		if( !Q_strcmp( STRING( def->s_name ), name ))
			return def;
	}

	return NULL;
}

/*
============
ED_FindGlobal
============
*/
ddef_t *ED_FindGlobal( const char *name )
{
	ddef_t	*def;

	if( !pr.progs || !pr.globaldefs )
		return NULL;
	
	for( int i = 0; i < pr.progs->numglobaldefs; i++ )
	{
		def = &pr.globaldefs[i];

		if( !Q_strcmp( STRING( def->s_name ), name ))
			return def;
	}

	return NULL;
}

/*
============
ED_FindFunction
============
*/
dfunction_t *ED_FindFunction( const char *name )
{
	dfunction_t	*func;

	if( !pr.progs || !pr.functions )
		return NULL;

	for( int i = 0; i < pr.progs->numfunctions; i++ )
	{
		func = &pr.functions[i];

		if( !Q_strcmp( STRING( func->s_name ), name ))
			return func;
	}

	return NULL;
}

/*
============
ED_FindFieldOffset
============
*/
int ED_FindFieldOffset( const char *field )
{
	ddef_t	*d;

	if(( d = ED_FindField( field )) == NULL )
		return 0;

	return d->ofs * 4;
}

/*
============
ED_FindGlobalOffset
============
*/
int ED_FindGlobalOffset( const char *field )
{
	ddef_t	*d;

	if(( d = ED_FindGlobal( field )) == NULL )
		return 0;

	return d->ofs * 4;
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
const char *PR_ValueString( int type, eval_t *val )
{
	static char	line[256];
	ddef_t		*def;
	dfunction_t	*f;
	
	type &= ~DEF_SAVEGLOBAL;
	line[0] = '\0';

	switch( (etype_t)type )
	{
	case ev_string:
		Q_snprintf( line, sizeof( line ), "%s", STRING( val->string ));
		break;
	case ev_entity:	
		Q_snprintf( line, sizeof( line ), "entity %i", ENTINDEX( INDEXENT( val->edict )));
		break;
	case ev_function:
		f = pr.functions + val->function;
		Q_snprintf( line, sizeof( line ), "%s()", STRING( f->s_name ));
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->integer );
		Q_snprintf( line, sizeof( line ), ".%s", STRING( def->s_name ));
		break;
	case ev_void:
		Q_snprintf( line, sizeof( line ), "void" );
		break;
	case ev_float:
		Q_snprintf( line, sizeof( line ), "%5.1f", val->value );
		break;
	case ev_vector:
		Q_snprintf( line, sizeof( line ), "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2] );
		break;
	case ev_pointer:
		Q_snprintf( line, sizeof( line ), "pointer" );
		break;
	default:
		Q_snprintf( line, sizeof( line ), "bad type %i", type );
		break;
	}
	
	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
const char *PR_GlobalString( int ofs )
{
	static char	line[256];
	
	eval_t *val = (eval_t *)&pr.globals[ofs];
	ddef_t *def = ED_GlobalAtOfs( ofs );

	if( !def )
	{
		Q_snprintf( line, sizeof( line ), "%i(???)", ofs );
	}
	else
	{
		const char *s = PR_ValueString( def->type, val );
		Q_snprintf( line, sizeof( line ), "%i(%s)%s", ofs, STRING( def->s_name ), s );
	}
	
	for( int i = Q_strlen( line ); i < 20; i++ )
		Q_strncat( line, " ", sizeof( line ));
	Q_strncat( line, " ", sizeof( line ));
		
	return line;
}

const char *PR_GlobalStringNoContents( int ofs )
{
	static char	line[256];
	
	ddef_t *def = ED_GlobalAtOfs( ofs );
	if( !def ) Q_snprintf( line, sizeof( line ), "%i(???)", ofs );
	else Q_snprintf( line, sizeof( line ), "%i(%s)", ofs, STRING( def->s_name ));
	
	for( int i = Q_strlen( line ); i < 20; i++ )
		Q_strncat( line, " ", sizeof( line ));
	Q_strncat( line, " ", sizeof( line ));
		
	return line;
}

/*
=============
ED_UpdateEdictFields

Important stuff to share prog fields
with standard engine entvars
to allow engine lookup and modify them
=============
*/
bool ED_UpdateEdictFields( edict_t *e )
{
	pr_entvars_t	*src;
	entvars_t		*dst;
	eval_t		*val;

	if( !e || e->free || !e->pvPrivateData )
		return false;

	src = (pr_entvars_t *)GET_PRIVATE( e );
	dst = &e->v;

	// HACKHACK: allow lerping everywhere
	if( !FBitSet( src->flags, FL_CLIENT ))
	{
		if( dst->frame != src->frame )
			dst->animtime = gpGlobals->time;
	}
	else
	{
		val = GETEDICTFIELDVALUE( e, pr.eval_gravity );

		if( val && val->value )
			dst->gravity = val->value;
		else dst->gravity = 1.0f;
	}

	dst->classname = src->classname;
	dst->absmin = src->absmin;
	dst->absmax = src->absmax;
	dst->origin = src->origin;
	dst->angles = src->angles;
	dst->movedir = src->movedir;
	dst->velocity = src->velocity;	
	dst->avelocity = src->avelocity;
	dst->punchangle = src->punchangle;
	dst->v_angle = src->v_angle;
	dst->modelindex = src->modelindex;
	dst->model = src->model;
	dst->absmin = src->absmin;
	dst->absmax = src->absmax;
	dst->mins = src->mins;
	dst->maxs = src->maxs;
	dst->size = src->size;
	dst->ltime = src->ltime;
	dst->nextthink = src->nextthink;
	dst->colormap = src->colormap;
	dst->movetype = src->movetype;
	dst->solid = src->solid;
	dst->skin = src->skin;
	dst->frame = src->frame;
	dst->effects = src->effects;
	dst->v_angle = src->v_angle;
	dst->fixangle = src->fixangle;
	dst->health = src->health;
	dst->frags = src->frags;
	dst->takedamage = src->takedamage;
	dst->deadflag = src->deadflag;
	dst->view_ofs = src->view_ofs;
	dst->impulse = src->impulse;
	dst->spawnflags = src->spawnflags;
	dst->flags = src->flags;
	dst->team = src->team;
	dst->max_health = src->max_health;
//	dst->teleport_time = src->teleport_time;
	dst->armortype = src->armortype;
	dst->armorvalue = src->armorvalue;
	dst->watertype = src->watertype;
	dst->waterlevel = src->waterlevel;
	dst->targetname = src->targetname;
	dst->target = src->target;
	dst->message = src->message;
	dst->netname = src->netname;
	dst->dmg_take = src->dmg_take;
	dst->dmg_save = src->dmg_save;
	dst->ideal_yaw = src->ideal_yaw;
	dst->yaw_speed = src->yaw_speed;
	dst->idealpitch = src->idealpitch;
	dst->weaponmodel = src->weaponmodel;

	if( src->aiment != 0 )
		dst->aiment = INDEXENT( src->aiment );
	else dst->aiment = NULL;

	if( src->dmg_inflictor != 0 )
		dst->dmg_inflictor = INDEXENT( src->dmg_inflictor );
	else dst->dmg_inflictor = NULL;

	if( src->owner != 0 )
		dst->owner = INDEXENT( src->owner );
	else dst->owner = NULL;

	if( src->enemy != 0 )
		dst->enemy = INDEXENT( src->enemy );
	else dst->enemy = NULL;

	dst->groundentity = INDEXENT( src->groundentity );

	// NEHAHRA transparency
	val = GETEDICTFIELDVALUE( e, pr.eval_alpha );
	if( val && val->value )
	{
		dst->rendermode = kRenderTransTexture;
		dst->renderamt = val->value * 255.0f;
	}
	else if( dst->rendermode != kRenderTransAlpha )
		dst->rendermode = kRenderNormal;

	// NEHAHRA fullbright
	val = GETEDICTFIELDVALUE( e, pr.eval_fullbright );
	if( val && val->value )
		SetBits( dst->effects, EF_FULLBRIGHT );
	else ClearBits( dst->effects, EF_FULLBRIGHT );

	return true;
}

/*
=============
ED_UpdateProgsFields

Important stuff to share edict entvars
with prog fields after call some engfuncs
e.g. WalkMove etc
=============
*/
bool ED_UpdateProgsFields( edict_t *e )
{
	pr_entvars_t	*dst;
	entvars_t		*src;

	if( !e || e->free || !e->pvPrivateData )
		return false;

	dst = (pr_entvars_t *)GET_PRIVATE( e );
	src = &e->v;

	dst->classname = src->classname;
	dst->absmin = src->absmin;
	dst->absmax = src->absmax;
	dst->origin = src->origin;
	dst->angles = src->angles;
	dst->movedir = src->movedir;
	dst->velocity = src->velocity;	
	dst->avelocity = src->avelocity;
	dst->punchangle = src->punchangle;
	dst->v_angle = src->v_angle;
	dst->modelindex = src->modelindex;
	dst->model = src->model;
	dst->absmin = src->absmin;
	dst->absmax = src->absmax;
	dst->mins = src->mins;
	dst->maxs = src->maxs;
	dst->size = src->size;
	dst->ltime = src->ltime;
	dst->nextthink = src->nextthink;
	dst->colormap = src->colormap;
	dst->movetype = src->movetype;
	dst->solid = src->solid;
	dst->skin = src->skin;
	dst->frame = src->frame;
	dst->effects = src->effects;
	dst->v_angle = src->v_angle;
	dst->fixangle = src->fixangle;
	dst->health = src->health;
	dst->frags = src->frags;
	dst->takedamage = src->takedamage;
	dst->deadflag = src->deadflag;
	dst->view_ofs = src->view_ofs;
	dst->impulse = src->impulse;
	dst->spawnflags = src->spawnflags;
	dst->flags = src->flags;
	dst->team = src->team;
	dst->max_health = src->max_health;
//	dst->teleport_time = src->teleport_time;
	dst->armortype = src->armortype;
	dst->armorvalue = src->armorvalue;
	dst->watertype = src->watertype;
	dst->waterlevel = src->waterlevel;
	dst->targetname = src->targetname;
	dst->target = src->target;
	dst->message = src->message;
	dst->netname = src->netname;
	dst->dmg_take = src->dmg_take;
	dst->dmg_save = src->dmg_save;
	dst->ideal_yaw = src->ideal_yaw;
	dst->yaw_speed = src->yaw_speed;
	dst->idealpitch = src->idealpitch;
	dst->weaponmodel = src->weaponmodel;

	if( src->aiment != NULL )
		dst->aiment = ENTINDEX( src->aiment );
	else dst->aiment = 0;

	if( src->dmg_inflictor != NULL )
		dst->dmg_inflictor = ENTINDEX( src->dmg_inflictor );
	else dst->dmg_inflictor = 0;

	if( src->owner != NULL )
		dst->owner = ENTINDEX( src->owner );
	else dst->owner = 0;

	if( src->enemy != NULL )
		dst->enemy = ENTINDEX( src->enemy );
	else dst->enemy = 0;

	if( src->groundentity != NULL )
		dst->groundentity = ENTINDEX( src->groundentity );
	else dst->groundentity = 0;

	return true;
}

/*
=============
ED_Print

For debugging
=============
*/
void ED_Print( edict_t *ed )
{
	if( ed->free || !ed->pvPrivateData || !pr.progs )
	{
		ALERT( at_console, "FREE\n" );
		return;
	}

	ALERT( at_console, "\nEDICT %i:\n", ENTINDEX( ed ));

	for( int i = 1; i < pr.progs->numfielddefs; i++ )
	{
		ddef_t *d = &pr.fielddefs[i];
		const char *name = STRING( d->s_name );

		if( name[Q_strlen( name ) - 2] == '_' )
			continue; // skip _x, _y, _z vars
			
		int *v = (int *)((char *)ed->pvPrivateData + d->ofs * 4);

		// if the value is still all 0, skip the field
		int type = d->type & ~DEF_SAVEGLOBAL;
		
		int j;
		for( j = 0; j < gProgSizes[type]; j++ )
			if( v[j] ) break;

		if( j == gProgSizes[type] )
			continue;
	
		ALERT( at_console, "%s", name );
		int l = Q_strlen( name );
		while( l++ < 15 )
			ALERT( at_console, " " );

		ALERT( at_console, "%s\n", PR_ValueString( d->type, (eval_t *)v ));		
	}
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts( void )
{
	ALERT( at_console, "%i entities\n", NUMBER_OF_ENTITIES( ));
	for( int i = 0; i < gpGlobals->maxEntities; i++ )
		ED_Print( INDEXENT( i ));
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f( void )
{
	int	i;
	
	i = atoi( CMD_ARGV( 1 ));
	if( INDEXENT( i ) == NULL )
	{
		ALERT( at_console, "Bad edict number\n" );
		return;
	}

	ED_Print( INDEXENT( i ));
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count( void )
{
	int	active, models, solid, step;
	edict_t	*ent;

	active = models = solid = step = 0;
	for( int i = 0; i < gpGlobals->maxEntities; i++ )
	{
		ent = INDEXENT( i );
		if( !ent ) break;	// end of list

		if( ent->free || !ent->pvPrivateData )
			continue;

		pr_entvars_t *pev = (pr_entvars_t *)GET_PRIVATE( ent );

		active++;
		if( pev->solid )
			solid++;
		if( pev->model )
			models++;
		if( pev->movetype == MOVETYPE_STEP )
			step++;
	}

	ALERT( at_console, "num_edicts:%3i\n", NUMBER_OF_ENTITIES( ));
	ALERT( at_console, "active    :%3i\n", active );
	ALERT( at_console, "view      :%3i\n", models );
	ALERT( at_console, "touch     :%3i\n", solid );
	ALERT( at_console, "step      :%3i\n", step );

}

//============================================================================
/*
=============
ED_ParseEpair

Can parse either fields or globals
returns false if error
=============
*/
static bool ED_ParseEpair( void *base, ddef_t *key, const char *s )
{
	ddef_t		*def;
	dfunction_t	*func;
	void		*d;

	if( !base ) return false; // force an error

	d = (void *)((int *)base + key->ofs);
	
	switch( key->type & ~DEF_SAVEGLOBAL )
	{
	case ev_string:
		*(string_t *)d = ALLOC_STRING( s );
		break;
	case ev_float:
		*(float *)d = Q_atof( s );
		break;
	case ev_vector:
		Q_atov((float *)d, s, 3 );
		break;
	case ev_entity:
		*(int *)d = ENTINDEX( INDEXENT( Q_atoi( s ))); // checking bounds because this may be unsafe
		break;
	case ev_field:
		def = ED_FindField (s);
		if( !def )
		{
			ALERT( at_warning, "can't find field %s\n", s );
			return false;
		}
		*(int *)d = G_INT( def->ofs );
		break;
	case ev_function:
		if(( func = ED_FindFunction( s )) == NULL )
		{
			ALERT( at_warning, "can't find function %s\n", s );
			return false;
		}
		*(func_t *)d = func - pr.functions;
		break;
	default:
		break;
	}

	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
static char *ED_ParseEdict( char *data, edict_t *ent )
{
	char	keyname[256];
	char	token[2048];
	char	temp[32];
	bool	anglehack;
	bool	init = false;
	ddef_t	*key;
	int	n;

	// go through all the dictionary pairs
	while( 1 )
	{	
		// parse key
		if(( data = COM_ParseFile( data, token )) == NULL )
			HOST_ERROR( "ED_ParseEdict: EOF without closing brace\n" );
		if( token[0] == '}' ) break;	// end of desc

		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if( !Q_strcmp( token, "angle" ))
		{
			Q_strncpy( token, "angles", sizeof( token ));
			anglehack = true;
		}
		else anglehack = false;

		// FIXME: change light to _light to get rid of this hack
		if( !Q_strcmp( token, "light" ))
			Q_strncpy( token, "light_lev", sizeof( token )); // hack for single light def

		Q_strncpy( keyname, token, sizeof( keyname ));

		// another hack to fix keynames with trailing spaces
		n = Q_strlen( keyname );
		while( n && keyname[n-1] == ' ' )
		{
			keyname[n-1] = 0;
			n--;
		}

		// parse value	
		if(( data = COM_ParseFile( data, token )) == NULL ) 
			HOST_ERROR( "ED_ParseEdict: EOF without closing brace\n" );

		if( token[0] == '}' )
			HOST_ERROR( "ED_ParseEdict: closing brace without data\n" );

		init = true;	

		// HANDLE some engine features here
		if( !Q_strcmp( keyname, "sky" ))
		{
			CVAR_SET_STRING( "sv_skyname", token );
			continue;
		}

		if( !Q_strcmp( keyname, "fog" ) || !Q_strcmp( keyname, "fog_" ))
		{
			float fog_settings[4];
			int packed_fog[4];
			UTIL_StringToFloatArray( fog_settings, 4, token );

			// NOTE: NEHAHRA and Xash3D fog divider is 100, Arcane Dimensions fog divider is 64
			// premultiply AD density to match value 
			fog_settings[0] *= 1.5625f;

			for( int i = 0; i < 4; i++)
				packed_fog[i] = fog_settings[i] * 255;

			// temporare place for store fog settings
			ent->v.impulse = (packed_fog[1]<<24)|(packed_fog[2]<<16)|(packed_fog[3]<<8)|packed_fog[0];
			continue;
		}

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if( keyname[0] == '_' ) continue;
		
		if(( key = ED_FindField( keyname )) == NULL )
		{
			ALERT( at_error, "'%s' is not a field\n", keyname );
			continue;
		}

		if( anglehack )
		{
			Q_strncpy( temp, token, sizeof( temp ));
			Q_snprintf( token, sizeof( token ), "0 %s 0", temp );
		}

		if( !ED_ParseEpair( ent->pvPrivateData, key, token ))
			HOST_ERROR( "ED_ParseEdict: parse error" );
	}

	// quake doesn't using FL_KILLME
	if( !init ) REMOVE_ENTITY( ent );

	return data;
}

void ED_SpawnEdict( edict_t *ent, pr_entvars_t *pev )
{
	int		old_self = pr.global_struct->self;
	dfunction_t	*func;

	// look for the spawn function
	func = ED_FindFunction( STRING( pev->classname ));
	ent->v.classname = pev->classname;

	if( !func )
	{
		ALERT( at_error, "no spawn function for:\n" );
		ED_Print( ent );
		REMOVE_ENTITY( ent );
		return;
	}

	pr.global_struct->self = ENTINDEX( ent );
	PR_ExecuteProgram( func - pr.functions );
	pr.global_struct->self = old_self;
}

void ED_PrecacheEdict( edict_t *ent, pr_entvars_t *pev )
{
	int		old_self = pr.global_struct->self;
	int		old_monsters, old_secrets;
	dfunction_t	*func;

	// look for the spawn function
	func = ED_FindFunction( STRING( pev->classname ));

	old_monsters = pr.global_struct->total_monsters;
	old_secrets = pr.global_struct->total_secrets;

	// check for collision between classnames and builtins
	if( func && func->first_statement > 0 )
	{
		// save restored state
		memcpy( pr.temp_entvars, pev, pr.edict_size );
		pr.precache = true;
		pr.num_moved = 0;

		// HACKHACK: trying to call spawn function
		// to allow precache models, sounds etc
		pr.global_struct->self = ENTINDEX( ent );
		PR_ExecuteProgram( func - pr.functions );
		pr.global_struct->self = old_self;

		// back actual state
		if( !FNullEnt( ent ))
		{
			for( int i = 0; i < pr.num_moved; i++ )
				REMOVE_ENTITY( pr.moved_edict[i] );
			memcpy( pev, pr.temp_entvars, pr.edict_size );
		}
		pr.precache = false;
		pr.num_moved = 0;
	}

	pr.global_struct->total_monsters = old_monsters;
	pr.global_struct->total_secrets = old_secrets;

	// need for update modelindex
	if ( pev->modelindex != 0 && !FStringNull( pev->model ))
		pev->modelindex = ent->v.modelindex = PRECACHE_MODEL( STRING( pev->model ));

	model_t *mod = (model_t *)MODEL_HANDLE( pev->modelindex );

	if( mod )
	{
		switch( mod->type )
		{
		case mod_sprite:
			// FIXME: kRenderTransAlpha doesn't have lerping between frames
			// but looks better than kRenderTransAdd. What i should choose?
			if( UTIL_CheckSpriteFullBright( mod ))
				ent->v.rendermode = kRenderTransAlpha;
			else ent->v.rendermode = kRenderNormal;
			ent->v.renderamt = 255;

			break;
		case mod_brush:
			break;
		}
	}
}

/*
===============
ED_InitEdict

in some cases VM try acessing to released entity. In Quake this cause no problems
because all entities was created as solid array, but in Xash we need to 'hot' initialize
this entity and alloc pvPrivateData (original VM code completely does not checking for ed->free state)
===============
*/
void ED_InitEdict( edict_t *ent )
{
	ALERT( at_aiconsole, "PR_ExecuteProgram: caused to access to freed entity %i\n", ENTINDEX( ent ));

	if( !ent->pvPrivateData )
		ALLOC_PRIVATE( ent, pr.progs->entityfields * 4 );

	memset( &ent->v, 0, sizeof( entvars_t ));
	ent->v.pContainingEntity = ent; // re-link

	ent->free = false;
}

/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
int ED_LoadFromFile( const char *mapname, char *entities )
{	
	char	token[2048];
	edict_t	*ent = NULL;
	int	inhibit = 0;
	char	*data = entities;
	bool	world = false;

	if( !data ) return 0; // probably this never happens

	pr.global_struct->time = gpGlobals->time;
	pr.global_struct->mapname = gpGlobals->mapname;
	pr.global_struct->deathmatch = gpGlobals->deathmatch;
	pr.global_struct->coop = gpGlobals->coop;
	pr.global_struct->teamplay = gpGlobals->teamplay;
	pr.global_struct->total_secrets = 0;
	pr.global_struct->found_secrets = 0;
	pr.global_struct->total_monsters = 0;
	pr.global_struct->killed_monsters = 0;
	pr.global_struct->serverflags = pr.serverflags;
	pr.serverflags = 0;

	CVAR_SET_STRING( "sv_skyname", "" );
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE( );

	g_iSkillLevel = (int)CVAR_GET_FLOAT( "skill" );
	
	// parse ents
	while(( data = COM_ParseFile( data, token )) != NULL )
	{
		if( token[0] != '{' )
			HOST_ERROR( "ED_LoadFromFile: found %s when expecting {\n", token );
		world = false;

		if( !ent )
		{
			ent = INDEXENT( 0 );
			world = true;
		}
		else ent = CREATE_ENTITY();

		// progs have constant class size for each entity
		pr_entvars_t *pev = (pr_entvars_t *)ALLOC_PRIVATE( ent, pr.progs->entityfields * 4 );

		ent->v.pContainingEntity = ent; // re-link

		// share world settings
		if( world ) ED_UpdateProgsFields( ent );

		data = ED_ParseEdict( data, ent );

		if( world ) pev->angles = g_vecZero;	// fixup issues on e3m3

		// remove things from different skill levels or deathmatch
		if( gpGlobals->deathmatch )
		{
			if( FBitSet( pev->spawnflags, SF_NOT_DEATHMATCH ))
			{
				REMOVE_ENTITY( ent );	
				inhibit++;
				continue;
			}
		}
		else if(( g_iSkillLevel == SKILL_EASY && FBitSet( pev->spawnflags, SF_NOT_EASY ))
		      || (g_iSkillLevel == SKILL_MEDIUM && FBitSet( pev->spawnflags, SF_NOT_MEDIUM ))
		      || (g_iSkillLevel >= SKILL_HARD && FBitSet( pev->spawnflags, SF_NOT_HARD )))
		{
			REMOVE_ENTITY( ent );
			inhibit++;
			continue;
		}

		//
		// immediately call spawn function
		//
		if( !pev->classname )
		{
			ALERT( at_error, "no classname for:\n" );
			ED_Print( ent );
			REMOVE_ENTITY( ent );
			continue;
		}

		ED_SpawnEdict( ent, pev );

		// map have the fog
		if( world )
		{
			if( ent->v.impulse != 0 )
			{
				UPDATE_PACKED_FOG( ent->v.impulse );
				// NOTE: store packed fog as integer
				*(int *)&pev->impulse = ent->v.impulse;
			}

			// tell the engine about soundtrack
			gpGlobals->cdAudioTrack = pev->sounds;
		}
	}	

	ALERT( at_console, "%i entities inhibited\n", inhibit );

	return 1;	// we done
}

/*
===============
PR_ResetGlobalState
===============
*/
void PR_ResetGlobalState( void )
{
	if( !pr.progs || !pr.source_globals || !pr.global_struct )
		return;

	memcpy( pr.global_struct, pr.source_globals, pr.global_size );
}

/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs( const char *progname )
{
	int		i, filesize;
	dfunction_t	*f;

	CRC_Init( &pr.crc );

	pr.progs = (dprograms_t *)LOAD_FILE( progname, &filesize );

	if( !pr.progs )
		HOST_ERROR( "PR_LoadProgs: couldn't load %s\n", progname );

	ALERT( at_console, "Programs occupy %iK.\n", filesize / 1024 );

	for( i = 0; i < filesize; i++ )
		CRC_ProcessByte( &pr.crc, ((byte *)pr.progs)[i] );

	if( pr.progs->version != PROG_VERSION )
		HOST_ERROR( "%s has wrong version number (%i should be %i)", progname, pr.progs->version, PROG_VERSION );

	if( pr.progs->crc != PROGHEADER_CRC )
		HOST_ERROR( "%s system vars have been modified, progdefs.h is out of date\n", progname );

	pr.functions = (dfunction_t *)((byte *)pr.progs + pr.progs->ofs_functions);
	gpGlobals->pStringBase = (const char *)pr.progs + pr.progs->ofs_strings;
	pr.globaldefs = (ddef_t *)((byte *)pr.progs + pr.progs->ofs_globaldefs);
	pr.fielddefs = (ddef_t *)((byte *)pr.progs + pr.progs->ofs_fielddefs);
	pr.statements = (dstatement_t *)((byte *)pr.progs + pr.progs->ofs_statements);
	pr.global_struct = (pr_globalvars_t *)((byte *)pr.progs + pr.progs->ofs_globals);
	pr.globals = (float *)pr.global_struct;

	pr.edict_size = pr.progs->entityfields * 4; // pvPrivateData size
	pr.global_size = pr.progs->numglobals * 4;

	for( i = 0; i < pr.progs->numfielddefs; i++ )
	{
		if( pr.fielddefs[i].type & DEF_SAVEGLOBAL )
			HOST_ERROR( "PR_LoadProgs: pr.fielddefs[i].type & DEF_SAVEGLOBAL\n" );
	}

	pr.temp_entvars = (pr_entvars_t *)calloc( pr.progs->entityfields, 4 );
	pr.source_globals = (pr_globalvars_t *)calloc( pr.progs->numglobals, 4 );

	// archive globals state to avoid reloading VM every map load\changelevel
	memcpy( pr.source_globals, pr.global_struct, pr.global_size );

	// setup offsets to user fields
	pr.eval_gravity = ED_FindFieldOffset( "gravity" );
	pr.eval_items2 = ED_FindFieldOffset( "items2" );
	pr.eval_alpha = ED_FindFieldOffset( "alpha" );
	pr.eval_fullbright = ED_FindFieldOffset( "fullbright" );
	pr.eval_idealpitch = ED_FindFieldOffset( "idealpitch" );
	pr.eval_pitch_speed = ED_FindFieldOffset( "pitch_speed" );
	pr.eval_classtype = ED_FindFieldOffset( "classtype" );
	pr.glob_framecount = ED_FindGlobalOffset( "framecount" );

	if(( f = ED_FindFunction( "RestoreGame" )) != NULL )
		pr.pfnRestoreGame = (func_t)(f - pr.functions);

	// install builtins for progs
	PR_InstallBuiltins();

	GAME_ADD_COMMAND ("edict", ED_PrintEdict_f);
	GAME_ADD_COMMAND ("edicts", ED_PrintEdicts);
	GAME_ADD_COMMAND ("edictcount", ED_Count);
	GAME_ADD_COMMAND ("profile", PR_Profile_f);
}

/*
===============
PR_UnloadProgs
===============
*/
void PR_UnloadProgs( void )
{
	if( !pr.progs ) return;

	FREE_FILE( pr.progs );
	free( pr.source_globals );
	free( pr.temp_entvars );
	memset( &pr, 0, sizeof( prog_state_t ));
}
