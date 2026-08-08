/*
pr_localize.cpp - Quake 2021 rerelease localization strings support
Copyright (C) 2026 a1batross

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
#include "progs.h"

// since 2026 update localization strings are shipped as plain text files inside pak0.pak
// (localization/loc_<language>.txt), so we can read them through the engine filesystem directly
#define LOC_BASE_PATH    "localization/loc_%s.txt"
#define LOC_MOD_PATH     "localization/loc_%s_mod.txt"
#define LOC_DEFAULT_LANG "english"

typedef struct
{
	char *key;
	char *value;
} locstring_t;

static locstring_t *loc_strings = NULL;
static int          loc_numstrings = 0;
static int          loc_maxstrings = 0;

/*
=================
PR_LocalizeFindIndex

linear search, only used while parsing to override duplicate keys
=================
*/
static int PR_LocalizeFindIndex( const char *key )
{
	for( int i = 0; i < loc_numstrings; i++ )
	{
		if( !Q_strcmp( loc_strings[i].key, key ))
			return i;
	}

	return -1;
}

/*
=================
PR_LocalizeAddString

adds or overrides a single key-value pair
=================
*/
static void PR_LocalizeAddString( const char *key, const char *value )
{
	int i = PR_LocalizeFindIndex( key );

	if( i != -1 )
	{
		free( loc_strings[i].value );
		loc_strings[i].value = strdup( value );
		return;
	}

	if( loc_numstrings == loc_maxstrings )
	{
		loc_maxstrings = loc_maxstrings ? loc_maxstrings * 2 : 2048;
		loc_strings = (locstring_t *)realloc( loc_strings, loc_maxstrings * sizeof( locstring_t ));
	}

	loc_strings[loc_numstrings].key = strdup( key );
	loc_strings[loc_numstrings].value = strdup( value );
	loc_numstrings++;
}

/*
=================
PR_LocalizeUnescape

processes \n \t \" \\ escape sequences in place
=================
*/
static void PR_LocalizeUnescape( char *str )
{
	char	*src = str, *dst = str;

	while( *src )
	{
		char	c = *src++;

		if( c == '\\' )
		{
			switch( *src )
			{
			case 'n': c = '\n'; src++; break;
			case 't': c = '\t'; src++; break;
			case '\"': c = '\"'; src++; break;
			case '\\': c = '\\'; src++; break;
			}
		}

		*dst++ = c;
	}

	*dst = '\0';
}

/*
=================
PR_LocalizeParseFile

parses rerelease localization file format:

// comment
key = "value"
key <platform list> = "value"	(platform-specific, ignored)

values are one line long, with \n \t \" \\ escape sequences
=================
*/
static void PR_LocalizeParseFile( const char *filename )
{
	char *file = (char *)LOAD_FILE( filename, NULL );
	char *pfile, *pnext;
	char key[256];
	char token[256];
	static char value[MAX_VAR_STRING];

	if( !file )
		return;

	pfile = file;

	// skip BOMж
	if( !Q_strnicmp( pfile, "\xef\xbb\xbf", 3 ))
		pfile += 3;

	while(( pfile = COM_ParseFileExt( pfile, key, sizeof( key ), true )) != NULL )
	{
		char	*eq = strchr( key, '=' );

		// `key= "..."` or `key="..."`: no space before '=', so the
		// tokenizer glued it (and possibly the value) to the key.
		// word tokens are verbatim copies, step pfile back to the '='
		if( eq != NULL )
		{
			pfile -= Q_strlen( eq );
			*eq = '\0';

			if( !key[0] )
				continue;	// stray '=' without a key
		}

		// expect '=' on the same line. scanned by hand rather than
		// tokenized because there may be no space after it either
		// (`key ="..."`, the shipped files do contain such typos)
		while( *pfile == ' ' || *pfile == '\t' || *pfile == '\r' )
			pfile++;

		if( *pfile != '=' )
		{
			// platform-qualified line (key <ps4 switch> = "...") or
			// junk, skip everything up to the end of the line
			while(( pnext = COM_ParseFileExt( pfile, token, sizeof( token ), false )) != NULL && token[0] )
				pfile = pnext;

			continue;
		}

		pfile++; // skip '='

		// the value must be a quoted string on the same line. checked by
		// hand because COM_ParseLine returns an empty token both for a
		// line break and for a legitimate empty "" value
		while( *pfile == ' ' || *pfile == '\t' || *pfile == '\r' )
			pfile++;

		if( *pfile != '\"' )
			continue;

		pfile = COM_ParseFileExt( pfile, value, sizeof( value ), false );

		// COM_ParseFile doesn't know about \" escapes: if the quote that
		// terminated the token was escaped, the token ends with an odd
		// number of backslashes. stitch the rest of the value back by hand
		for( ;; )
		{
			int len = Q_strlen( value );
			int slashes = 0;

			while( len - slashes > 0 && value[len - 1 - slashes] == '\\' )
				slashes++;

			if( !( slashes & 1 ))
				break;

			value[len - 1] = '\"';

			while( *pfile && *pfile != '\"' && *pfile != '\n' && len < (int)sizeof( value ) - 1 )
				value[len++] = *pfile++;

			value[len] = '\0';

			if( *pfile == '\"' )
				pfile++;
			else
				break;	// unterminated value, don't loop forever
		}

		PR_LocalizeUnescape( value );
		PR_LocalizeAddString( key, value );
	}

	ALERT( at_console, "%s: %i strings after parsing %s\n", __func__, loc_numstrings, filename );

	FREE_FILE( file );
}

/*
=================
PR_LocalizeInit

loads localization strings for the current language,
the language comes from engine's ui_language cvar
=================
*/
void PR_LocalizeInit( void )
{
	const char *lang = CVAR_GET_STRING( "ui_language" );
	char path[64];

	if( !lang || !lang[0] )
		lang = LOC_DEFAULT_LANG;

	Q_snprintf( path, sizeof( path ), LOC_BASE_PATH, lang );
	PR_LocalizeParseFile( path );

	// no such language? fall back to english
	if( !loc_numstrings && Q_stricmp( lang, LOC_DEFAULT_LANG ))
	{
		Q_snprintf( path, sizeof( path ), LOC_BASE_PATH, LOC_DEFAULT_LANG );
		PR_LocalizeParseFile( path );
	}

	// mod override file, keys here replace the base ones
	Q_snprintf( path, sizeof( path ), LOC_MOD_PATH, lang );
	PR_LocalizeParseFile( path );
}

/*
=================
PR_LocalizeString

resolves $key references coming from rerelease progs,
returns the string unchanged if it's not a key or the key is unknown
=================
*/
const char *PR_LocalizeString( const char *str )
{
	if( !str || str[0] != '$' )
		return str;

	int i = PR_LocalizeFindIndex( str + 1 );

	if( i == -1 )
	{
		ALERT( at_aiconsole, "%s: unknown key %s\n", __func__, str );
		return str;
	}

	return loc_strings[i].value;
}
