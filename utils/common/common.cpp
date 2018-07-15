/*
common.cpp - common game routines
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stringlib.h>
#include <fcntl.h>
#include <io.h>

/*
============
COM_FileBase

Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
============
*/
void COM_FileBase( const char *in, char *out )
{
	int len, start, end;

	len = Q_strlen( in );
	if( !len ) return;
	
	// scan backward for '.'
	end = len - 1;

	while( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if( in[end] != '.' )
		end = len-1; // no '.', copy to end
	else end--; // found ',', copy to left of '.'


	// scan backward for '/'
	start = len - 1;

	while( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if( start < 0 || ( in[start] != '/' && in[start] != '\\' ))
		start = 0;
	else start++;

	// length of new sting
	len = end - start + 1;

	// Copy partial string
	Q_strncpy( out, &in[start], len + 1 );
	out[len] = 0;
}

/*
============
COM_ExtractFilePath
============
*/
void COM_ExtractFilePath( const char *path, char *dest )
{
	const char	*src;
	src = path + Q_strlen( path ) - 1;

	// back up until a \ or the start
	while( src != path && !(*(src - 1) == '\\' || *(src - 1) == '/' ))
		src--;

	if( src != path )
	{
		memcpy( dest, path, src - path );
		dest[src - path - 1] = 0; // cutoff backslash
	}
	else Q_strcpy( dest, "" ); // file without path
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( char *path )
{
	size_t	length;

	length = Q_strlen( path ) - 1;
	while( length > 0 && path[length] != '.' )
	{
		length--;
		if( path[length] == '/' || path[length] == '\\' || path[length] == ':' )
			return; // no extension
	}
	if( length ) path[length] = 0;
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, const char *extension )
{
	const char *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + Q_strlen( path ) - 1;

	while( *src != '/' && src != path )
	{
		// it has an extension
		if( *src == '.' ) return;                 
		src--;
	}
	Q_strcat( path, extension );
}

/*
============
COM_FixSlashes

Changes all '/' characters into '\' characters, in place.
============
*/
void COM_FixSlashes( char *pname )
{
	while( *pname )
	{
		if( *pname == '\\' )
			*pname = '/';
		pname++;
	}
}

/*
============
COM_FileExtension
============
*/
const char *COM_FileExtension( const char *in )
{
	const char *separator, *backslash, *colon, *dot;

	separator = Q_strrchr( in, '/' );
	backslash = Q_strrchr( in, '\\' );

	if( !separator || separator < backslash )
		separator = backslash;

	colon = Q_strrchr( in, ':' );

	if( !separator || separator < colon )
		separator = colon;

	dot = Q_strrchr( in, '.' );

	if( dot == NULL || ( separator && ( dot < separator )))
		return "";

	return dot + 1;
}

/*
==================
COM_FileExists
==================
*/
bool COM_FileExists( const char *path )
{
	int desc;
     
	if(( desc = open( path, O_RDONLY|O_BINARY )) < 0 )
		return false;

	close( desc );
	return true;
}