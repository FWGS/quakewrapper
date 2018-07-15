/*
pr_exec.cpp - Quake virtual machine wrapper impmenentation
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
#include "client.h"
#include "game.h"
#include "progs.h"

// LordHavoc: optimized
#define OPA ((eval_t *)&pr.globals[(word)st->a])
#define OPB ((eval_t *)&pr.globals[(word)st->b])
#define OPC ((eval_t *)&pr.globals[(word)st->c])

static const char *pr_opnames[] =
{
"^5DONE",

"MUL_F",
"MUL_V", 
"MUL_FV",
"MUL_VF",
 
"DIV",

"ADD_F",
"ADD_V", 
  
"SUB_F",
"SUB_V",

"^2EQ_F",
"^2EQ_V",
"^2EQ_S",
"^2EQ_E",
"^2EQ_FNC",

"^2NE_F",
"^2NE_V",
"^2NE_S",
"^2NE_E",
"^2NE_FNC",

"^2LE",
"^2GE",
"^2LT",
"^2GT",

"^6FIELD_F",
"^6FIELD_V",
"^6FIELD_S",
"^6FIELD_ENT",
"^6FIELD_FLD",
"^6FIELD_FNC",

"^1ADDRESS",

"STORE_F",
"STORE_V",
"STORE_S",
"STORE_ENT",
"STORE_FLD",
"STORE_FNC",

"^1STOREP_F",
"^1STOREP_V",
"^1STOREP_S",
"^1STOREP_ENT",
"^1STOREP_FLD",
"^1STOREP_FNC",

"^5RETURN",

"^2NOT_F",
"^2NOT_V",
"^2NOT_S",
"^2NOT_ENT",
"^2NOT_FNC",
  
"^5IF",
"^5IFNOT",
  
"^3CALL0",
"^3CALL1",
"^3CALL2",
"^3CALL3",
"^3CALL4",
"^3CALL5",
"^3CALL6",
"^3CALL7",
"^3CALL8",
  
"^1STATE",

"^5GOTO",

"^2AND",
"^2OR",

"BITAND",
"BITOR"
};

//=============================================================================
/*
=================
PR_PrintStatement
=================
*/
static void PR_PrintStatement( dstatement_t *s )
{
	int	i;
	
	if((unsigned int)s->op < ARRAYSIZE( pr_opnames ))
	{
		ALERT( at_console, "%s ", pr_opnames[s->op] );
		i = Q_strlen( pr_opnames[s->op] );
		// don't count a preceding color tag when padding the name
		if( pr_opnames[s->op][0] == '^' )
			i -= 2;
		for( ; i < 10; i++ )
			ALERT( at_console, " " );
	}
		
	if( s->op == OP_IF || s->op == OP_IFNOT )
	{
		ALERT( at_console, "%sbranch %i", PR_GlobalString( s->a ), s->b );
	}
	else if( s->op == OP_GOTO )
	{
		ALERT( at_console, "branch %i", s->a );
	}
	else if((unsigned int)(s->op - OP_STORE_F) < 6 )
	{
		ALERT( at_console, "%s", PR_GlobalString( s->a ));
		ALERT( at_console, "%s", PR_GlobalStringNoContents( s->b ));
	}
	else if( s->op == OP_ADDRESS || (unsigned int)( s->op - OP_LOAD_F ) < 6 )
	{
		if( s->a ) ALERT( at_console, "%s", PR_GlobalString( s->a ));
		if( s->b ) ALERT( at_console, "%s", PR_GlobalStringNoContents( s->b ));
		if( s->c ) ALERT( at_console, "%s", PR_GlobalStringNoContents( s->c ));
	}
	else
	{
		if( s->a ) ALERT( at_console, "%s", PR_GlobalString( s->a ));
		if( s->b ) ALERT( at_console, "%s", PR_GlobalString( s->b ));
		if( s->c ) ALERT( at_console, "%s", PR_GlobalStringNoContents( s->c ));
	}

	ALERT( at_console, "\n" );
}

/*
============
PR_StackTrace
============
*/
static void PR_StackTrace( void )
{
	dfunction_t	*f;

	if( pr.depth == 0 )
	{
		ALERT( at_console, "<NO STACK>\n" );
		return;
	}
	
	pr.stack[pr.depth].f = pr.xfunction;

	for( int i = pr.depth; i >= 0; i-- )
	{
		f = pr.stack[i].f;

		if( f ) ALERT( at_console, "%12s : %s\n", STRING( f->s_file ), STRING( f->s_name ));		
		else ALERT( at_console, "<NO FUNCTION>\n" );
	}
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f( void )
{
	dfunction_t	*f, *best;
	int		max, num = 0;

	if( !pr.progs ) return;

	do
	{
		max = 0;
		best = NULL;

		for( int i = 0; i < pr.progs->numfunctions; i++ )
		{
			f = &pr.functions[i];
			if( f->profile > max )
			{
				max = f->profile;
				best = f;
			}
		}

		if( best )
		{
			if( num < 10 )
				ALERT( at_console, "%7i %s\n", best->profile, STRING( best->s_name ));
			best->profile = 0;
			num++;
		}
	} while( best );
}

/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError( const char *error, ... )
{
	va_list		argptr;
	char		string[1024];

	va_start( argptr, error );
	Q_vsnprintf( string, sizeof( string ), error, argptr );
	va_end( argptr );

	PR_PrintStatement( pr.statements + pr.xstatement );
	PR_StackTrace ();
	ALERT( at_console, "%s\n", string );
	
	pr.depth = 0;	// dump the stack so host_error can shutdown functions

	HOST_ERROR( "program error\n" );
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/
/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
int PR_EnterFunction( dfunction_t *f )
{
	int	i, j, c, o;

	pr.stack[pr.depth].s = pr.xstatement;
	pr.stack[pr.depth].f = pr.xfunction;	
	pr.depth++;

	if( pr.depth >= MAX_STACK_DEPTH )
		PR_RunError( "stack overflow" );

	// save off any locals that the new function steps on
	c = f->locals;

	if( pr.localstack_used + c > LOCALSTACK_SIZE )
		PR_RunError( "localstack overflow" );

	for( i = 0; i < c; i++ )
		pr.localstack[pr.localstack_used+i] = ((int *)pr.globals)[f->parm_start + i];
	pr.localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for( i = 0; i < f->numparms; i++ )
	{
		for( j = 0; j < f->parm_size[i]; j++ )
		{
			((int *)pr.globals)[o] = ((int *)pr.globals)[OFS_PARM0+i*3+j];
			o++;
		}
	}

	pr.xfunction = f;

	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int PR_LeaveFunction( void )
{
	int	i, c;

	if( pr.depth <= 0 )
		PR_RunError( "stack underflow" );

	// restore locals from the stack
	c = pr.xfunction->locals;
	pr.localstack_used -= c;
	if( pr.localstack_used < 0 )
		PR_RunError( "localstack underflow" );

	for( i = 0; i < c; i++ )
		((int *)pr.globals)[pr.xfunction->parm_start + i] = pr.localstack[pr.localstack_used+i];

	// up stack
	pr.depth--;
	pr.xfunction = pr.stack[pr.depth].f;
	return pr.stack[pr.depth].s;
}

/*
====================
PR_ExecuteProgram
====================
*/
void PR_ExecuteProgram( func_t fnum )
{
	eval_t		*a, *b, *c;
	dfunction_t	*f, *newf;
	int		exitdepth;
	int		runaway;
	int		i, s;
	eval_t		*ptr;
	dstatement_t	*st;
	pr_entvars_t	*ev;
	edict_t		*ed;

	if( !fnum || fnum >= pr.progs->numfunctions )
	{
		if( pr.global_struct->self )
			ED_Print( INDEXENT( pr.global_struct->self ));
		PR_RunError( "NULL function" );
	}

	f = &pr.functions[fnum];
	runaway = 100000;
	pr.trace = false;

	// make a stack frame
	exitdepth = pr.depth;

	s = PR_EnterFunction( f );
	
	while( 1 )
	{
		s++; // next statement

		st = &pr.statements[s];
		a = (eval_t *)&pr.globals[st->a];
		b = (eval_t *)&pr.globals[st->b];
		c = (eval_t *)&pr.globals[st->c];
	
		if( !--runaway )
			PR_RunError( "runaway loop error" );
		
		pr.xfunction->profile++;
		pr.xstatement = s;
	
		if( pr.trace )
			PR_PrintStatement( st );
		
		switch( st->op )
		{
		case OP_ADD_F:
			c->value = a->value + b->value;
			break;
		case OP_ADD_V:
			c->vector[0] = a->vector[0] + b->vector[0];
			c->vector[1] = a->vector[1] + b->vector[1];
			c->vector[2] = a->vector[2] + b->vector[2];
			break;
		case OP_SUB_F:
			c->value = a->value - b->value;
			break;
		case OP_SUB_V:
			c->vector[0] = a->vector[0] - b->vector[0];
			c->vector[1] = a->vector[1] - b->vector[1];
			c->vector[2] = a->vector[2] - b->vector[2];
			break;
		case OP_MUL_F:
			c->value = a->value * b->value;
			break;
		case OP_MUL_V:
			c->value = a->vector[0] * b->vector[0] + a->vector[1] * b->vector[1] + a->vector[2] * b->vector[2];	// DotProduct
			break;
		case OP_MUL_FV:
			c->vector[0] = a->value * b->vector[0];
			c->vector[1] = a->value * b->vector[1];
			c->vector[2] = a->value * b->vector[2];
			break;
		case OP_MUL_VF:
			c->vector[0] = b->value * a->vector[0];
			c->vector[1] = b->value * a->vector[1];
			c->vector[2] = b->value * a->vector[2];
			break;
		case OP_DIV_F:
			// don't divide by zero
			if( b->value == 0.0f ) c->value = 0.0f;
			else c->value = a->value / b->value;
			break;
		case OP_BITAND:
			c->value = (int)a->value & (int)b->value;
			break;
		case OP_BITOR:
			c->value = (int)a->value | (int)b->value;
			break;
		case OP_GE:
			c->value = a->value >= b->value;
			break;
		case OP_LE:
			c->value = a->value <= b->value;
			break;
		case OP_GT:
			c->value = a->value > b->value;
			break;
		case OP_LT:
			c->value = a->value < b->value;
			break;
		case OP_AND:
			c->value = a->value && b->value;
			break;
		case OP_OR:
			c->value = a->value || b->value;
			break;
		case OP_NOT_F:
			c->value = !a->value;
			break;
		case OP_NOT_V:
			c->value = !a->vector[0] && !a->vector[1] && !a->vector[2];
			break;
		case OP_NOT_S:
			c->value = !a->string || !*STRING( a->string );
			break;
		case OP_NOT_FNC:
			c->value = !a->function;
			break;
		case OP_NOT_ENT:
			c->value = FNullEnt( a->edict ); // 0-th entity is a world. QC never has the NULL pointer to entity
			break;
		case OP_EQ_F:
			c->value = a->value == b->value;
			break;
		case OP_EQ_V:
			c->value = (a->vector[0] == b->vector[0]) && (a->vector[1] == b->vector[1]) && (a->vector[2] == b->vector[2]);
			break;
		case OP_EQ_S:
			c->value = !Q_strcmp( STRING( a->string ), STRING( b->string ));
			break;
		case OP_EQ_E:
			c->value = a->integer == b->integer;
			break;
		case OP_EQ_FNC:
			c->value = a->function == b->function;
			break;
		case OP_NE_F:
			c->value = a->value != b->value;
			break;
		case OP_NE_V:
			c->value = (a->vector[0] != b->vector[0]) || (a->vector[1] != b->vector[1]) || (a->vector[2] != b->vector[2]);
			break;
		case OP_NE_S:
			c->value = Q_strcmp( STRING( a->string ), STRING( b->string ));
			break;
		case OP_NE_E:
			c->value = a->integer != b->integer;
			break;
		case OP_NE_FNC:
			c->value = a->function != b->function;
			break;
//==================
		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:		// integers
		case OP_STORE_S:
		case OP_STORE_FNC:		// pointers
			b->integer = a->integer;
			break;
		case OP_STORE_V:
			b->vector[0] = a->vector[0];
			b->vector[1] = a->vector[1];
			b->vector[2] = a->vector[2];
			break;
		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:		// integers
		case OP_STOREP_S:
		case OP_STOREP_FNC:		// pointers
			ed = INDEXENT( b->integer & 0x0000FFFF );
			if( !ed ) PR_RunError( "entity %i is out of range", b->integer & 0x0000FFFF );
			if( ed->free || ed->pvPrivateData == NULL )
				ED_InitEdict( ed );
			ptr = (eval_t *)((int *)ed->pvPrivateData + ((b->integer & 0xFFFF0000) >> 16));
			ptr->integer = a->integer;
			break;
		case OP_STOREP_V:
			ed = INDEXENT( b->integer & 0x0000FFFF );
			if( !ed ) PR_RunError( "entity %i is out of range", b->integer & 0x0000FFFF );
			if( ed->free || ed->pvPrivateData == NULL )
				ED_InitEdict( ed );
			ptr = (eval_t *)((int *)ed->pvPrivateData + ((b->integer & 0xFFFF0000) >> 16));
			ptr->vector[0] = a->vector[0];
			ptr->vector[1] = a->vector[1];
			ptr->vector[2] = a->vector[2];
			break;
		case OP_ADDRESS:
			ed = INDEXENT( a->edict );
			if( FNullEnt( ed ) && GET_SERVER_STATE() == SERVER_ACTIVE )
				PR_RunError( "address: assignment to null/world entity" );
			// NOTE: we can't store address as single number because userdata is not a part of sv.edicts array
			// and alloced\freed dynamically per each entity. Interpret adress at low and highpart and store entity num
			// in lowpart and localoffset in highpart
			c->integer = (b->integer<<16)|(a->edict);
			break;
		case OP_LOAD_F:
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
			ed = INDEXENT( a->edict );
			if( !ed ) PR_RunError( "entity %i is out of range", a->edict );
			if( ed->free || ed->pvPrivateData == NULL )
				ED_InitEdict( ed );
			a = (eval_t *)((int *)ed->pvPrivateData + b->integer);
			c->integer = a->integer;
			break;
		case OP_LOAD_V:
			ed = INDEXENT( a->edict );
			if( !ed ) PR_RunError( "entity %i is out of range", a->edict );
			if( ed->free || ed->pvPrivateData == NULL )
				ED_InitEdict( ed );
			a = (eval_t *)((int *)ed->pvPrivateData + b->integer);
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;
//==================
		case OP_IFNOT:
			if( !a->integer )
				s += st->b - 1;	// offset the s++
			break;
		case OP_IF:
			if( a->integer )
				s += st->b - 1;	// offset the s++
			break;
		case OP_GOTO:
			s += st->a - 1;		// offset the s++
			break;
		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
			pr.argc = st->op - OP_CALL0;
			if( !a->function )
				PR_RunError( "NULL function" );
			newf = &pr.functions[a->function];

			if( newf->first_statement < 0 )
			{
				// negative statements are built in functions
				i = -newf->first_statement;
				if( i >= pr.numbuiltins || pr.builtins[i] == NULL )
					PR_RunError( "no such builtin #%i", i );
				pr.builtins[i]();
				break;
			}

			s = PR_EnterFunction( newf );
			break;
		case OP_DONE:
		case OP_RETURN:
			pr.globals[OFS_RETURN+0] = pr.globals[st->a+0];
			pr.globals[OFS_RETURN+1] = pr.globals[st->a+1];
			pr.globals[OFS_RETURN+2] = pr.globals[st->a+2];
	
			s = PR_LeaveFunction();
			if( pr.depth == exitdepth )
			{
				PR_SendMessage();
				return; // all done (program leave point)
			}
			break;
		case OP_STATE:
			ed = INDEXENT( pr.global_struct->self );
			ev = (pr_entvars_t *)GET_PRIVATE( ed );
			if( ed->free || !ev )
				PR_RunError( "state: assignment to freed entity" );
			ev->nextthink = pr.global_struct->time + 0.1;
			ed->v.animtime = gpGlobals->time;
			ev->think = b->function;
			ev->frame = a->value;
			break;
		default:
			PR_RunError( "bad opcode %i", st->op );
			break;
		}
	}
}