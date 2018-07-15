/*
mathlib.h - base math functions
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MATHLIB_H
#define MATHLIB_H

#include <math.h>

#define NUMVERTEXNORMALS	162

// euler angle order
#define PITCH		0
#define YAW		1
#define ROLL		2

#ifndef M_PI
#define M_PI		(float)3.14159265358979323846
#endif

#ifndef M_PI2
#define M_PI2		(float)6.28318530717958647692
#endif

#define M_PI_F		((float)(M_PI))
#define M_PI2_F		((float)(M_PI2))

#define RAD2DEG( x )	((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD( x )	((float)(x) * (float)(M_PI / 180.f))
#define Q_rint(x)		((x) < 0 ? ((int)((x)-0.5f)) : ((int)((x)+0.5f)))

typedef float	vec_t;
typedef vec_t	vec2_t[2];	// x,y
typedef vec_t	vec3_t[3];	// x,y,z
typedef vec_t	vec4_t[4];	// x,y,z,w
typedef vec_t	matrix4x4[4][4];

#define VectorCopy(a,b) ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorSet(v, x, y, z) ((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))
#define VectorAdd(a,b,c) ((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define DotProduct(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorClear(x) ((x)[0]=(x)[1]=(x)[2]=0)
#define VectorMA(a, scale, b, c) ((c)[0] = (a)[0] + (scale) * (b)[0],(c)[1] = (a)[1] + (scale) * (b)[1],(c)[2] = (a)[2] + (scale) * (b)[2])
#define VectorLerp( v1, lerp, v2, c ) ((c)[0] = (v1)[0] + (lerp) * ((v2)[0] - (v1)[0]), (c)[1] = (v1)[1] + (lerp) * ((v2)[1] - (v1)[1]), (c)[2] = (v1)[2] + (lerp) * ((v2)[2] - (v1)[2]))
#define VectorNormalize( v ) { float ilength = (float)sqrt(DotProduct(v, v));if (ilength) ilength = 1.0f / ilength;v[0] *= ilength;v[1] *= ilength;v[2] *= ilength; }
#define bound( min, num, max ) ((num) >= (min) ? ((num) < (max) ? (num) : (max)) : (min))

float anglemod( float a );
void SinCos( float radians, float *sine, float *cosine );

#define Matrix4x4_LoadIdentity( mat )	Matrix4x4_Copy( mat, matrix4x4_identity )
#define Matrix4x4_Copy( out, in )	memcpy( out, in, sizeof( matrix4x4 ))

void Matrix4x4_CreateFromEntity( matrix4x4 out, const float angles[3], const float origin[3], float scale );
void Matrix4x4_Concat( matrix4x4 out, const matrix4x4 in1, const matrix4x4 in2 );
void Matrix4x4_VectorTransform( const matrix4x4 in, const float v[3], float out[3] );
void Matrix4x4_VectorITransform( const matrix4x4 in, const float v[3], float out[3] );
void Matrix4x4_VectorRotate( const matrix4x4 in, const float v[3], float out[3] );
void Matrix4x4_VectorIRotate( const matrix4x4 in, const float v[3], float out[3] );
void Matrix4x4_ConcatTranslate( matrix4x4 out, float x, float y, float z );
void Matrix4x4_CreateTranslate( matrix4x4 out, float x, float y, float z );
void Matrix4x4_ConcatScale( matrix4x4 out, float x, float y, float z );
void Matrix4x4_CreateScale( matrix4x4 out, float x, float y, float z );

extern vec3_t		vec3_origin;
extern const matrix4x4	matrix4x4_identity;

#endif