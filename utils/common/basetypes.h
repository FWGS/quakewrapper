/*
basetypes.h - aliases for some base types for convenience
Copyright (C) 2012 Uncle Mike

This file is part of XashNT source code.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef BASETYPES_H
#define BASETYPES_H

//#define HLFX_BUILD

#ifdef _WIN32
#pragma warning( disable : 4244 )	// MIPS
#pragma warning( disable : 4018 )	// signed/unsigned mismatch
#pragma warning( disable : 4305 )	// truncation from const double to float
#endif

typedef unsigned char	byte;
typedef unsigned short	word;
typedef unsigned long	dword;
typedef unsigned int	uint;
typedef unsigned long	ulong;

#undef true
#undef false

#ifndef __cplusplus
typedef enum { false, true }	qboolean;
#else 
typedef int qboolean;
#endif

// a simple string implementation
#define MAX_STRING		256
typedef char		string[MAX_STRING];

#define SetBits( iBitVector, bits )	((iBitVector) = (iBitVector) | (bits))
#define ClearBits( iBitVector, bits )	((iBitVector) = (iBitVector) & ~(bits))
#define FBitSet( iBitVector, bit )	((iBitVector) & (bit))

#endif//BASETYPES_H