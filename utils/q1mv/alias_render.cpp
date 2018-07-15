/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/
// updates:
// 1-4-99	fixed file texture load and file read bug

////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gl.h>
#include <GL/glu.h>
#include "AliasModel.h"
#include "GLWindow.h"
#include "ViewerSettings.h"
#include "stringlib.h" 

#include <mx.h>
#include "mdlviewer.h"

vec3_t		g_lightvec;			// light vector in model reference frame
int		g_ambientlight;			// ambient world light
float		g_shadelight;			// direct world light
vec3_t		g_lightcolor;

bool		bUseWeaponOrigin = false;
bool		bUseWeaponLeftHand = false;
extern bool	g_bStopPlaying;

// pre-quantized table normals from Quake1
const float m_bytenormals[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

void AliasModel :: centerView( bool reset )
{
	vec3_t	min, max;

	ExtractBbox( min, max );

	float dx = max[0] - min[0];
	float dy = max[1] - min[1];
	float dz = max[2] - min[2];
	float d = max( dx, max( dy, dz ));

	if( reset )
	{
		g_viewerSettings.trans[0] = 0;
		g_viewerSettings.trans[1] = 0;
		g_viewerSettings.trans[2] = 0;
	}
	else
	{
		g_viewerSettings.trans[0] = 0;
		g_viewerSettings.trans[1] = min[2] + dz / 2.0f;
		g_viewerSettings.trans[2] = d * 1.0f;
	}

	g_viewerSettings.rot[0] = -90.0f;
	g_viewerSettings.rot[1] = -90.0f;
	g_viewerSettings.rot[2] = 0.0f;

	g_viewerSettings.movementScale = max( 1.0f, d * 0.01f );
}

int AliasModel :: setFrame( int newframe, bool nolerp )
{
	if( !m_paliashdr )
		return 0;

	if( newframe == -1 )
		return m_newframe;

	if( newframe < g_GlWindow->getStartFrame( ))
		return m_newframe;

	if( newframe > g_GlWindow->getEndFrame( ))
		return m_newframe;

	if( nolerp )
	{
		m_stepMovement[1] = m_stepMovement[0] = 0.0f;		
		m_flAnimTime[1] = m_flAnimTime[0] = m_flTime;
		m_oldframe = m_newframe = newframe;
	}
	else
	{
		m_stepMovement[1] = m_stepMovement[0];
		m_stepMovement[0] = m_paliashdr->scale_origin[0];
		m_flAnimTime[1] = m_flAnimTime[0];
		m_flAnimTime[0] = m_flTime;
		m_oldframe = m_newframe;
		m_newframe = newframe;
	}

	return newframe;
}
	
/*
================
StudioModel::SetupLighting
	set some global variables based on entity position
inputs:
outputs:
	g_ambientlight
	g_shadelight
================
*/
void AliasModel :: SetupLighting( void )
{
	g_ambientlight = 95;
	g_shadelight = 160;

	g_lightcolor[0] = g_viewerSettings.lColor[0];
	g_lightcolor[1] = g_viewerSettings.lColor[1];
	g_lightcolor[2] = g_viewerSettings.lColor[2];

	Matrix4x4_VectorIRotate( m_protationmatrix, g_viewerSettings.gLightVec, g_lightvec );
	VectorNormalize( g_lightvec );
}

/*
===============
R_AliasLighting

===============
*/
void AliasModel :: Lighting( float *lv, const vec3_t normal )
{
	float 	illum = g_ambientlight;
	float	r = 1.5f, lightcos;

	lightcos = DotProduct( normal, g_lightvec ); // -1 colinear, 1 opposite
#if 1	// QuakeSpasm
	if( lightcos < 0.0 )
		lightcos = 1.0 + lightcos * 0.3f;
	else lightcos = 1.0 + lightcos;

	illum += g_shadelight * lightcos;
#else
	if( lightcos > 1.0f ) lightcos = 1.0f;

	illum += g_shadelight;

	// do modified hemispherical lighting
	if( r <= 1.0f )
	{
		r += 1.0f;
		lightcos = (( r - 1.0f ) - lightcos) / r;
		if( lightcos > 0.0f ) 
			illum += g_shadelight * lightcos; 
	}
	else
	{
		lightcos = (lightcos + ( r - 1.0f )) / r;
		if( lightcos > 0.0f )
			illum -= g_shadelight * lightcos; 
	}
#endif
	illum = max( illum, 0.0f );
	illum = min( illum, 255.0f );
	*lv = illum * (1.0f / 255.0f);
}

/*
=============
GL_DrawAliasFrame
=============
*/
void AliasModel :: DrawAliasFrame( bool bFullbright, bool bWireframe )
{
	float 		lv_tmp;
	trivertex_t	*verts0;
	trivertex_t	*verts1;
	vec3_t		vert, norm;
	vec3_t		v0, v1;
	float		min, max;
	int		*order;
	int		count;
	vec3_t		out;

	verts0 = verts1 = m_pPoseData;
	verts0 += m_oldpose * m_iPoseVerts;
	verts1 += m_newpose * m_iPoseVerts;
	order = m_pCommands;
	min = 255, max = -255;

	if( bWireframe )
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDisable( GL_TEXTURE_2D );
		glColor4f( 1.0f, 0.0f, 0.0f, 0.99f ); 

		glEnable( GL_LINE_SMOOTH );
		glEnable( GL_POLYGON_SMOOTH );
		glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
		glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	}
	else if (g_viewerSettings.transparency < 1.0f)
	{
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask( GL_TRUE );
	}

	if( bWireframe == false )
		glEnable( GL_MULTISAMPLE );

	while( 1 )
	{
		// get the vertex count and primitive type
		count = *order++;
		if( !count ) break; // done

		if( count < 0 )
		{
			glBegin( GL_TRIANGLE_FAN );
			count = -count;
		}
		else
		{
			glBegin( GL_TRIANGLE_STRIP );
                    }

		if( !bFullbright && !bWireframe )
			g_viewerSettings.drawn_polys += (count - 2);

		do
		{
			// texture coordinates come from the draw list
			glTexCoord2f( ((float *)order)[0], ((float *)order)[1] );
			order += 2;

			if( !bFullbright )
			{
				VectorLerp( m_bytenormals[verts0->lightnormalindex], m_flLerpfrac, m_bytenormals[verts1->lightnormalindex], norm );
				VectorNormalize( norm );
				Lighting( &lv_tmp, norm );
				glColor4f( g_lightcolor[0] * lv_tmp, g_lightcolor[1] * lv_tmp, g_lightcolor[2] * lv_tmp, g_viewerSettings.transparency );
			}
			else if( !bWireframe )
			{
				glColor4f( 1.0f, 1.0f, 1.0f, g_viewerSettings.transparency );
			}

			VectorCopy( verts0->v, v0 );
			VectorCopy( verts1->v, v1 );
			VectorLerp( v0, m_flLerpfrac, v1, vert );

			if( vert[2] < 24 )
			{
				if( vert[0] < min ) min = vert[0];
				if( vert[0] > max ) max = vert[0];
			}
			Matrix4x4_VectorTransform( m_protationmatrix, vert, out );

			glVertex3fv( out );
			verts0++, verts1++;
		} while( --count );

		glEnd();
	}

	m_stepMovement[0] = ((min + max) * 0.5f * m_paliashdr->scale[0]) + m_paliashdr->scale_origin[0];

	if( bWireframe )
	{
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glDisable( GL_BLEND );
		glDisable( GL_LINE_SMOOTH );
		glDisable(GL_POLYGON_SMOOTH);
	}
	else
	{
		glDisable( GL_MULTISAMPLE );
	}
}

/*
=============
GL_DrawAliasFrame
=============
*/
void AliasModel :: DrawAliasNormals( void )
{
	trivertex_t	*verts0;
	trivertex_t	*verts1;
	vec3_t		vert, norm;
	vec3_t		v0, v1;
	int		*order;
	int		count;
	vec3_t		av, nv;

	verts0 = verts1 = m_pPoseData;
	verts0 += m_oldpose * m_iPoseVerts;
	verts1 += m_newpose * m_iPoseVerts;
	order = m_pCommands;

	bool texEnabled = glIsEnabled( GL_TEXTURE_2D ) ? true : false;

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	if( texEnabled ) glDisable( GL_TEXTURE_2D );
	glColor4f( 0.3f, 0.4f, 0.5f, 0.99f ); 

	glEnable( GL_LINE_SMOOTH );
	glEnable( GL_POLYGON_SMOOTH );
	glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );

	while( 1 )
	{
		// get the vertex count and primitive type
		count = *order++;
		if( !count ) break; // done

		count = abs( count );

		glBegin( GL_LINES );
		do
		{
			order += 2;

			VectorLerp( m_bytenormals[verts0->lightnormalindex], m_flLerpfrac, m_bytenormals[verts1->lightnormalindex], norm );
			VectorCopy( verts0->v, v0 );
			VectorCopy( verts1->v, v1 );
			VectorLerp( v0, m_flLerpfrac, v1, vert );

			Matrix4x4_VectorTransform( m_protationmatrix, vert, av );
			Matrix4x4_VectorRotate( m_protationmatrix, norm, nv );
			VectorNormalize( nv );

			glVertex3fv( av );
			glVertex3f( av[0] + nv[0] * 2.0f, av[1] + nv[1] * 2.0f, av[2] + nv[2] * 2.0f );
			verts0++, verts1++;
		} while( --count );

		glEnd();
	}

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	if( texEnabled ) glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glDisable( GL_LINE_SMOOTH );
	glDisable(GL_POLYGON_SMOOTH);
}

/*
=============
GL_DrawAliasFrame
=============
*/
void AliasModel :: DrawAliasUVMap( void )
{
	if( !m_paliashdr ) return;

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glDisable( GL_TEXTURE_2D );
	glColor4f( 1.0f, 1.0f, 1.0f, 0.99f ); 

	for( int i = 0; i < m_paliashdr->numtris; i++ )
	{
		dtriangle_t *tri = &m_triangles[i];

		glBegin( GL_TRIANGLES );
		for( int j = 0; j < 3; j++ )
		{
			float x = (float)m_stverts[tri->vertindex[j]].s;
			float y = (float)m_stverts[tri->vertindex[j]].t;
			if( y < 0.0f ) y += m_paliashdr->skinheight; // OpenGL issues

			if( tri->facesfront )
			{
				if( m_stverts[tri->vertindex[j]].onseam )
					x += m_paliashdr->skinwidth * 0.5; // on back side
				glColor3f( 1.0f, 0.0f, 0.0f ); 
			}
			else glColor3f( 1.0f, 1.0f, 1.0f ); 

			x *= g_viewerSettings.textureScale;
			y *= g_viewerSettings.textureScale;

			glVertex2f( offset_x + x, offset_y + y );
		}
		glEnd();
	}

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
}

/*
=================
R_SetupAliasFrame

=================
*/
void AliasModel :: SetupFrame( void )
{
	int	newpose, oldpose;
	int	newframe, oldframe;
	int	numposes, cycle;
	float	interval;

	oldframe = m_oldframe;
	newframe = m_newframe;

	if( newframe < 0 )
	{
		newframe = 0;
	}
	else if( newframe >= m_paliashdr->numframes )
	{
		newframe = m_paliashdr->numframes - 1;
	}

	if(( oldframe >= m_paliashdr->numframes ) || ( oldframe < 0 ))
		oldframe = newframe;

	numposes = m_pFrames[newframe].numposes;

	if( numposes > 1 )
	{
		oldpose = newpose = m_pFrames[newframe].firstpose;
		interval = 1.0f / m_pFrames[newframe].interval;
		cycle = (int)(m_flTime * interval);
		oldpose += (cycle + 0) % numposes; // lerpframe from
		newpose += (cycle + 1) % numposes; // lerpframe to
		m_flLerpfrac = ( m_flTime * interval );
		m_flLerpfrac -= (int)m_flLerpfrac;
	}
	else
	{
		oldpose = m_pFrames[oldframe].firstpose;
		newpose = m_pFrames[newframe].firstpose;
	}

	m_oldpose = oldpose;
	m_newpose = newpose;
}

void AliasModel :: SetupTransform( bool bMirror )
{
	vec3_t	angles = { 0.0f, 0.0f, 0.0f };
	vec3_t	origin = { 0.0f, 0.0f, 0.0f };
	float	signY = 1.0f, signZ = 1.0f;

	// NOTE: this completely over control about angles and don't broke interpolation
	if( FBitSet( m_paliashdr->flags, ALIAS_ROTATE ))
		angles[YAW] = anglemod( 100.0f * m_flTime );

	if( g_viewerSettings.editMode == EDIT_SOURCE )
		VectorCopy( m_editfields[0].origin, origin );

	if( !bUseWeaponOrigin && g_viewerSettings.adjustOrigin )
		origin[2] = ORIGIN_OFFSET;

	if( bUseWeaponOrigin && bUseWeaponLeftHand )
		signY = -1.0f;

	if( bMirror )
		signZ = -1.0f;

	Matrix4x4_CreateFromEntity( m_protationmatrix, angles, origin, 1.0f );
	Matrix4x4_ConcatTranslate( m_protationmatrix, m_paliashdr->scale_origin[0], m_paliashdr->scale_origin[1] * signY, m_paliashdr->scale_origin[2] );
	Matrix4x4_ConcatScale( m_protationmatrix, m_paliashdr->scale[0], m_paliashdr->scale[1] * signY, m_paliashdr->scale[2] * signZ );

}

/*
================
StudioModel::DrawModel
inputs:
	currententity
	r_entorigin
================
*/
void AliasModel :: DrawModel( bool bMirror )
{
	int	anim, skin;

	if( !m_paliashdr ) return;

	if(( m_flTime < m_flAnimTime[0] + 1.0f ) && ( m_flAnimTime[0] != m_flAnimTime[1] ))
		m_flLerpfrac = ( m_flTime - m_flAnimTime[0] ) / ( m_flAnimTime[0] - m_flAnimTime[1] );
	else m_flLerpfrac = 0.0f;

	m_flLerpfrac = bound( 0.0f, m_flLerpfrac, 1.0f );

	SetupTransform( bMirror );

	SetupLighting();

	anim = (int)(m_flTime * 10) & 3;
	skin = bound( 0, m_skinnum, m_paliashdr->numskins - 1 );

	SetupFrame();

	bool isInEditMode = (g_MDLViewer->getTableIndex() == TAB_MODELEDITOR) ? true : false;
	bool drawEyePos = g_viewerSettings.showEyePosition;

	if( isInEditMode && m_pedit )
		drawEyePos = (m_pedit->type == TYPE_EYEPOSITION) ? true : false;

	glEnable( GL_MULTISAMPLE );
	glBindTexture( GL_TEXTURE_2D, gl_texturenum[skin][anim] );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	DrawAliasFrame( false );

	// render fullbrights
	if( fb_texturenum[skin][anim] && g_viewerSettings.renderMode == RM_TEXTURED && g_viewerSettings.renderLuma )
	{
		glEnable( GL_BLEND );
		glDepthMask( GL_FALSE );
		glDepthFunc( GL_EQUAL );
		glBlendFunc( GL_ONE, GL_SRC_ALPHA );
		glBindTexture( GL_TEXTURE_2D, fb_texturenum[skin][anim] );
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		DrawAliasFrame( true );
		glDepthFunc( GL_LEQUAL );
		glDepthMask( GL_TRUE );
		glDisable( GL_BLEND );
	}
	glDisable( GL_MULTISAMPLE );

	if( g_viewerSettings.showWireframeOverlay && g_viewerSettings.renderMode != RM_WIREFRAME )
		DrawAliasFrame( true, true );

	if( g_viewerSettings.showNormals )
		DrawAliasNormals();

	if( drawEyePos )
	{
		bool texEnabled = glIsEnabled( GL_TEXTURE_2D ) ? true : false;

		if( texEnabled )	
			glDisable( GL_TEXTURE_2D );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );

		glPointSize( 7.0f );
		glColor3f( 1.0f, 0.5f, 1.0f );

		glBegin( GL_POINTS );
			if( isInEditMode && m_pedit && g_viewerSettings.editMode == EDIT_SOURCE )
				glVertex3fv( m_pedit->origin );
			else glVertex3fv( m_paliashdr->eyeposition );
		glEnd();

		if( texEnabled )	
			glEnable( GL_TEXTURE_2D );
		glEnable( GL_DEPTH_TEST );
		if( g_viewerSettings.transparency < 1.0f )
			glEnable( GL_BLEND );
		glPointSize( 1.0f );
	}
}