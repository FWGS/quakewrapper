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

AliasModel g_aliasModel;
extern bool bUseWeaponOrigin;
extern bool bUseWeaponLeftHand;

////////////////////////////////////////////////////////////////////////

// Quake1 always has same palette
byte palette_q1[768] =
{
0,0,0,15,15,15,31,31,31,47,47,47,63,63,63,75,75,75,91,91,91,107,107,107,123,123,123,139,139,139,155,155,155,171,
171,171,187,187,187,203,203,203,219,219,219,235,235,235,15,11,7,23,15,11,31,23,11,39,27,15,47,35,19,55,43,23,63,
47,23,75,55,27,83,59,27,91,67,31,99,75,31,107,83,31,115,87,31,123,95,35,131,103,35,143,111,35,11,11,15,19,19,27,
27,27,39,39,39,51,47,47,63,55,55,75,63,63,87,71,71,103,79,79,115,91,91,127,99,99,139,107,107,151,115,115,163,123,
123,175,131,131,187,139,139,203,0,0,0,7,7,0,11,11,0,19,19,0,27,27,0,35,35,0,43,43,7,47,47,7,55,55,7,63,63,7,71,71,
7,75,75,11,83,83,11,91,91,11,99,99,11,107,107,15,7,0,0,15,0,0,23,0,0,31,0,0,39,0,0,47,0,0,55,0,0,63,0,0,71,0,0,79,
0,0,87,0,0,95,0,0,103,0,0,111,0,0,119,0,0,127,0,0,19,19,0,27,27,0,35,35,0,47,43,0,55,47,0,67,55,0,75,59,7,87,67,7,
95,71,7,107,75,11,119,83,15,131,87,19,139,91,19,151,95,27,163,99,31,175,103,35,35,19,7,47,23,11,59,31,15,75,35,19,
87,43,23,99,47,31,115,55,35,127,59,43,143,67,51,159,79,51,175,99,47,191,119,47,207,143,43,223,171,39,239,203,31,255,
243,27,11,7,0,27,19,0,43,35,15,55,43,19,71,51,27,83,55,35,99,63,43,111,71,51,127,83,63,139,95,71,155,107,83,167,123,
95,183,135,107,195,147,123,211,163,139,227,179,151,171,139,163,159,127,151,147,115,135,139,103,123,127,91,111,119,
83,99,107,75,87,95,63,75,87,55,67,75,47,55,67,39,47,55,31,35,43,23,27,35,19,19,23,11,11,15,7,7,187,115,159,175,107,
143,163,95,131,151,87,119,139,79,107,127,75,95,115,67,83,107,59,75,95,51,63,83,43,55,71,35,43,59,31,35,47,23,27,35,
19,19,23,11,11,15,7,7,219,195,187,203,179,167,191,163,155,175,151,139,163,135,123,151,123,111,135,111,95,123,99,83,
107,87,71,95,75,59,83,63,51,67,51,39,55,43,31,39,31,23,27,19,15,15,11,7,111,131,123,103,123,111,95,115,103,87,107,
95,79,99,87,71,91,79,63,83,71,55,75,63,47,67,55,43,59,47,35,51,39,31,43,31,23,35,23,15,27,19,11,19,11,7,11,7,255,
243,27,239,223,23,219,203,19,203,183,15,187,167,15,171,151,11,155,131,7,139,115,7,123,99,7,107,83,0,91,71,0,75,55,
0,59,43,0,43,31,0,27,15,0,11,7,0,0,0,255,11,11,239,19,19,223,27,27,207,35,35,191,43,43,175,47,47,159,47,47,143,47,
47,127,47,47,111,47,47,95,43,43,79,35,35,63,27,27,47,19,19,31,11,11,15,43,0,0,59,0,0,75,7,0,95,7,0,111,15,0,127,23,
7,147,31,7,163,39,11,183,51,15,195,75,27,207,99,43,219,127,59,227,151,79,231,171,95,239,191,119,247,211,139,167,123,
59,183,155,55,199,195,55,231,227,87,127,191,255,171,231,255,215,255,255,103,0,0,139,0,0,179,0,0,215,0,0,255,0,0,255,
243,147,255,247,199,255,255,255,159,91,83
};

////////////////////////////////////////////////////////////////////////
static int g_tex_base = TEXTURE_COUNT;

//Mugsy - upped the maximum texture size to 512. All changes are the replacement of '256'
//with this define, MAX_TEXTURE_DIMS
#define MAX_TEXTURE_DIMS 1024	

void AliasModel :: UploadTexture( byte *data, int width, int height, byte *srcpal, int name, bool luma )
{
	int	row1[MAX_TEXTURE_DIMS], row2[MAX_TEXTURE_DIMS], col1[MAX_TEXTURE_DIMS], col2[MAX_TEXTURE_DIMS];
	byte	*pix1, *pix2, *pix3, *pix4;
	byte	*tex, *in, *out, pal[768];
	bool	has_alpha = false;
	int	i, j;

	// convert texture to power of 2
	int outwidth;
	for (outwidth = 1; outwidth < width; outwidth <<= 1);

	if (outwidth > MAX_TEXTURE_DIMS)
		outwidth = MAX_TEXTURE_DIMS;

	int outheight;
	for (outheight = 1; outheight < height; outheight <<= 1);

	if (outheight > MAX_TEXTURE_DIMS)
		outheight = MAX_TEXTURE_DIMS;

	in = (byte *)malloc(width * height * 4);
	if (!in) return;

	memcpy( pal, srcpal, 768 );

	// only player supposed to have remaps!
	if( !luma && Q_stristr( g_viewerSettings.modelFile, "player.mdl" ))
		PaletteHueReplace( pal, g_viewerSettings.topcolor * 16, g_viewerSettings.bottomcolor * 16 );

	// expand pixels to rgba
	for (i=0 ; i < width * height; i++)
	{
		if( data[i] == 255 )
		{
			has_alpha = true;
			in[i*4+0] = 0x00;
			in[i*4+1] = 0x00;
			in[i*4+2] = 0x00;
			in[i*4+3] = 0x00;
		}
		else
		{
			if( !luma || data[i] >= 224 )
			{
				in[i*4+0] = pal[data[i]*3+0];
				in[i*4+1] = pal[data[i]*3+1];
				in[i*4+2] = pal[data[i]*3+2];
			}
			else
			{
				in[i*4+0] = in[i*4+1] = in[i*4+2] = 0x0;
			}
			in[i*4+3] = 0xFF;
		}
	}

	tex = out = (byte *)malloc( outwidth * outheight * 4);
	if (!out)
	{
		return;
	}

	for (i = 0; i < outwidth; i++)
	{
		col1[i] = (int) ((i + 0.25) * (width / (float)outwidth));
		col2[i] = (int) ((i + 0.75) * (width / (float)outwidth));
	}

	for (i = 0; i < outheight; i++)
	{
		row1[i] = (int) ((i + 0.25) * (height / (float)outheight)) * width;
		row2[i] = (int) ((i + 0.75) * (height / (float)outheight)) * width;
	}

	// scale down and convert to 32bit RGB
	for (i=0 ; i<outheight ; i++)
	{
		for (j=0 ; j<outwidth ; j++, out += 4)
		{
			pix1 = &in[(row1[i] + col1[j]) * 4];
			pix2 = &in[(row1[i] + col2[j]) * 4];
			pix3 = &in[(row2[i] + col1[j]) * 4];
			pix4 = &in[(row2[i] + col2[j]) * 4];

			out[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			out[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			out[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			out[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}

	GLint outFormat = (has_alpha) ? GL_RGBA : GL_RGB;

	glBindTexture( GL_TEXTURE_2D, name );		
	glHint( GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST );
	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
	glTexImage2D( GL_TEXTURE_2D, 0, outFormat, outwidth, outheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	const char *extensions = (const char *)glGetString( GL_EXTENSIONS );

	// check for anisotropy support
	if( Q_strstr( extensions, "GL_EXT_texture_filter_anisotropic" ))
	{
		float	anisotropy = 1.0f;
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy );
	}

	free( tex );
	free( in );
}

void AliasModel :: PaletteHueReplace( byte *palSrc, int top, int bottom )
{
	byte	dst[256], src[256];
	int	i;

	for( i = 0; i < 256; i++ )
		src[i] = i;
	memcpy( dst, src, 256 );

	if( top < 128 )
	{
		// the artists made some backwards ranges. sigh.
		memcpy( dst + SHIRT_HUE_START, src + top, 16 );
	}
	else
	{
		for( i = 0; i < 16; i++ )
			dst[SHIRT_HUE_START+i] = src[top + 15 - i];
	}

	if( bottom < 128 )
	{
		memcpy( dst + PANTS_HUE_START, src + bottom, 16 );
	}
	else
	{
		for( i = 0; i < 16; i++ )
			dst[PANTS_HUE_START + i] = src[bottom + 15 - i];
	}

	// last color isn't changed because it's alpha-pixel
	for( i = 0; i < 255; i++ )
	{
		palSrc[i*3+0] = palette_q1[dst[i]*3+0];
		palSrc[i*3+1] = palette_q1[dst[i]*3+1];
		palSrc[i*3+2] = palette_q1[dst[i]*3+2];
	}
}

void AliasModel :: RemapTextures( void )
{
	daliasskintype_t	*pskintype;

	if( !remap_textures ) return;

	m_iNumSkins = 0;	// ouch! ouch! ouch!

	// reload the skins
	pskintype = (daliasskintype_t *)&m_paliashdr[1];
	pskintype = LoadAllSkins( m_paliashdr->numskins, pskintype );

	remap_textures = false;
}

/*
================
StripLength
================
*/
int AliasModel :: StripLength( int starttri, int startv )
{
	int		m1, m2, j, k;
	dtriangle_t	*last, *check;

	m_used[starttri] = 2;

	last = &m_triangles[starttri];

	m_stripverts[0] = last->vertindex[(startv+0) % 3];
	m_stripverts[1] = last->vertindex[(startv+1) % 3];
	m_stripverts[2] = last->vertindex[(startv+2) % 3];

	m_striptris[0] = starttri;
	m_stripcount = 1;

	m1 = last->vertindex[(startv+2)%3];
	m2 = last->vertindex[(startv+1)%3];
nexttri:
	// look for a matching triangle
	for( j = starttri + 1, check = &m_triangles[starttri + 1]; j < m_paliashdr->numtris; j++, check++ )
	{
		if( check->facesfront != last->facesfront )
			continue;

		for( k = 0; k < 3; k++ )
		{
			if( check->vertindex[k] != m1 )
				continue;
			if( check->vertindex[(k+1) % 3] != m2 )
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if( m_used[j] ) goto done;

			// the new edge
			if( m_stripcount & 1 )
				m2 = check->vertindex[(k+2) % 3];
			else m1 = check->vertindex[(k+2) % 3];

			m_stripverts[m_stripcount+2] = check->vertindex[(k+2) % 3];
			m_striptris[m_stripcount] = j;
			m_stripcount++;

			m_used[j] = 2;
			goto nexttri;
		}
	}
done:
	// clear the temp used flags
	for( j = starttri + 1; j < m_paliashdr->numtris; j++ )
	{
		if( m_used[j] == 2 )
			m_used[j] = 0;
	}

	return m_stripcount;
}

/*
===========
FanLength
===========
*/
int AliasModel :: FanLength( int starttri, int startv )
{
	int		m1, m2, j, k;
	dtriangle_t	*last, *check;

	m_used[starttri] = 2;

	last = &m_triangles[starttri];

	m_stripverts[0] = last->vertindex[(startv+0) % 3];
	m_stripverts[1] = last->vertindex[(startv+1) % 3];
	m_stripverts[2] = last->vertindex[(startv+2) % 3];

	m_striptris[0] = starttri;
	m_stripcount = 1;

	m1 = last->vertindex[(startv+0) % 3];
	m2 = last->vertindex[(startv+2) % 3];

nexttri:
	// look for a matching triangle
	for( j = starttri + 1, check = &m_triangles[starttri + 1]; j < m_paliashdr->numtris; j++, check++ )
	{
		if( check->facesfront != last->facesfront )
			continue;

		for( k = 0; k < 3; k++ )
		{
			if( check->vertindex[k] != m1 )
				continue;
			if( check->vertindex[(k+1) % 3] != m2 )
				continue;

			// this is the next part of the fan
			// if we can't use this triangle, this tristrip is done
			if( m_used[j] ) goto done;

			// the new edge
			m2 = check->vertindex[(k+2) % 3];

			m_stripverts[m_stripcount + 2] = m2;
			m_striptris[m_stripcount] = j;
			m_stripcount++;

			m_used[j] = 2;
			goto nexttri;
		}
	}
done:
	// clear the temp used flags
	for( j = starttri + 1; j < m_paliashdr->numtris; j++ )
	{
		if( m_used[j] == 2 )
			m_used[j] = 0;
	}

	return m_stripcount;
}

/*
================
BuildTris

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void AliasModel :: BuildTris( void )
{
	int	len, bestlen, besttype;
	int	bestverts[1024];
	int	besttris[1024];
	int	type, startv;
	int	i, j, k;
	float	s, t;

	//
	// build tristrips
	//
	memset( m_used, 0, sizeof( m_used ));
	m_numcommands = 0;
	m_numorder = 0;

	for( i = 0; i < m_paliashdr->numtris; i++ )
	{
		// pick an unused triangle and start the trifan
		if( m_used[i] ) continue;

		bestlen = 0;
		for( type = 0; type < 2; type++ )
		{
			for( startv = 0; startv < 3; startv++ )
			{
				if( type == 1 ) len = StripLength( i, startv );
				else len = FanLength( i, startv );

				if( len > bestlen )
				{
					besttype = type;
					bestlen = len;

					for( j = 0; j < bestlen + 2; j++ )
						bestverts[j] = m_stripverts[j];

					for( j = 0; j < bestlen; j++ )
						besttris[j] = m_striptris[j];
				}
			}
		}

		// mark the tris on the best strip as used
		for( j = 0; j < bestlen; j++ )
			m_used[besttris[j]] = 1;

		if( besttype == 1 )
			m_commands[m_numcommands++] = (bestlen + 2);
		else m_commands[m_numcommands++] = -(bestlen + 2);

		for( j = 0; j < bestlen + 2; j++ )
		{
			// emit a vertex into the reorder buffer
			k = bestverts[j];
			m_vertexorder[m_numorder++] = k;

			// emit s/t coords into the commands stream
			s = m_stverts[k].s;
			t = m_stverts[k].t;

			if( !m_triangles[besttris[0]].facesfront && m_stverts[k].onseam )
				s += m_paliashdr->skinwidth / 2;	// on back side
			s = (s + 0.5f) / m_paliashdr->skinwidth;
			t = (t + 0.5f) / m_paliashdr->skinheight;

			// Carmack use floats and Valve use shorts here...
			*(float *)&m_commands[m_numcommands++] = s;
			*(float *)&m_commands[m_numcommands++] = t;
		}
	}

	m_commands[m_numcommands++] = 0; // end of list marker
}

/*
================
GL_MakeAliasModelDisplayLists
================
*/
void AliasModel :: MakeDisplayLists( void )
{
	trivertex_t	*verts;
	int		i, j;

	BuildTris( );

	// save the data out
	m_iPoseVerts = m_numorder;

	m_pCommands = (int *)malloc( m_numcommands * 4 );
	memcpy( m_pCommands, m_commands, m_numcommands * 4 );

	verts = m_pPoseData = (trivertex_t *)malloc( m_iNumPoses * m_iPoseVerts * sizeof( trivertex_t ));

	for( i = 0; i < m_iNumPoses; i++ )
	{
		for( j = 0; j < m_numorder; j++ )
			*verts++ = m_poseverts[i][m_vertexorder[j]];
	}
}

void *AliasModel :: LoadSingleSkin( daliasskintype_t *pskintype, int skinnum, int size )
{
	byte	*data = (byte *)(pskintype + 1);
	bool	skin_has_luma = false;

	UploadTexture( data, m_paliashdr->skinwidth, m_paliashdr->skinheight, palette_q1, g_tex_base + m_iNumSkins );
	gl_texturenum[skinnum][0] =
	gl_texturenum[skinnum][1] =
	gl_texturenum[skinnum][2] =
	gl_texturenum[skinnum][3] = g_tex_base + m_iNumSkins;
	m_iNumSkins++;

	m_skindata[skinnum] = data; // set pointer to skindata

	for( int i = 0; i < size; i++ )
	{
		if( data[i] > 224 && data[i] != 255 )
		{
			skin_has_luma = true;
			break;
		}
	}

	if( skin_has_luma )
	{
		UploadTexture( data, m_paliashdr->skinwidth, m_paliashdr->skinheight, palette_q1, g_tex_base + m_iNumSkins, true );
		fb_texturenum[skinnum][0] =
		fb_texturenum[skinnum][1] =
		fb_texturenum[skinnum][2] =
		fb_texturenum[skinnum][3] = g_tex_base + m_iNumSkins;
		m_iNumSkins++;
	}

	return ((byte *)(pskintype + 1) + size);
}

void *AliasModel :: LoadGroupSkin( daliasskintype_t *pskintype, int skinnum, int size )
{
	daliasskininterval_t	*pinskinintervals;
	daliasskingroup_t		*pinskingroup;
	int			i, j;

	// animating skin group.  yuck.
	pskintype++;
	pinskingroup = (daliasskingroup_t *)pskintype;
	pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);
	pskintype = (daliasskintype_t *)(pinskinintervals + pinskingroup->numskins);

	for( i = 0; i < pinskingroup->numskins; i++ )
	{
		byte	*data = (byte *)(pskintype + 1);
		bool	skin_has_luma = false;

		UploadTexture( data, m_paliashdr->skinwidth, m_paliashdr->skinheight, palette_q1, g_tex_base + m_iNumSkins );
		gl_texturenum[skinnum][i & 3] = g_tex_base + m_iNumSkins;
		if( i == 0 ) m_skindata[skinnum] = data; // set pointer to skindata. FIXME this is handle only first skin from group
		m_iNumSkins++;

		for( j = 0; j < size; j++ )
		{
			if( data[j] > 224 && data[j] != 255 )
			{
				skin_has_luma = true;
				break;
			}
		}

		if( skin_has_luma )
		{
			UploadTexture( data, m_paliashdr->skinwidth, m_paliashdr->skinheight, palette_q1, g_tex_base + m_iNumSkins, true );
			fb_texturenum[skinnum][i & 3] = g_tex_base + m_iNumSkins;
			m_iNumSkins++;
		}

		pskintype = (daliasskintype_t *)((byte *)(pskintype) + size);
	}

	for( j = i; i < 4; i++ )
	{
		gl_texturenum[skinnum][i & 3] = gl_texturenum[skinnum][i - j]; 
		fb_texturenum[skinnum][i & 3] = fb_texturenum[skinnum][i - j]; 
	}

	return pskintype;
}

/*
===============
Mod_LoadAllSkins
===============
*/
daliasskintype_t *AliasModel :: LoadAllSkins( int numskins, daliasskintype_t *pskintype )
{
	int size = m_paliashdr->skinwidth * m_paliashdr->skinheight;

	for( int i = 0; i < numskins; i++ )
	{
		if( pskintype->type == ALIAS_SKIN_SINGLE )
		{
			pskintype = (daliasskintype_t *)LoadSingleSkin( pskintype, i, size );
		}
		else
		{
			pskintype = (daliasskintype_t *)LoadGroupSkin( pskintype, i, size );
		}
	}

	return pskintype;
}

/*
=================
Mod_LoadAliasFrame
=================
*/
void *AliasModel :: LoadAliasFrame( void *pin, maliasframedesc_t *frame )
{
	daliasframe_t	*pdaliasframe;
	trivertex_t	*pinframe;
	int		i;

	pdaliasframe = (daliasframe_t *)pin;

	Q_strncpy( frame->name, pdaliasframe->name, sizeof( frame->name ));
	int frameNum = atoi( frame->name );

	// if frames just enumarated
	if( frameNum != 0 )
		Q_snprintf( frame->name, sizeof( frame->name ), "enum%d", frameNum );
	frame->firstpose = m_posenum;
	frame->numposes = 1;

	for( i = 0; i < 3; i++ )
	{
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmax.v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertex_t *)(pdaliasframe + 1);

	m_poseverts[m_posenum] = (trivertex_t *)pinframe;
	pinframe += m_paliashdr->numverts;
	m_posenum++;

	return (void *)pinframe;
}

/*
=================
Mod_LoadAliasGroup
=================
*/
void *AliasModel :: LoadAliasGroup( void *pin, maliasframedesc_t *frame )
{
	daliasgroup_t	*pingroup;
	daliasframe_t	*pdaliasframe;
	int		i, numframes;
	daliasinterval_t	*pin_intervals;
	void		*ptemp;

	pingroup = (daliasgroup_t *)pin;
	numframes = pingroup->numframes;

	frame->firstpose = m_posenum;
	frame->numposes = numframes;

	for( i = 0; i < 3; i++ )
	{
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
	}

	pin_intervals = (daliasinterval_t *)(pingroup + 1);

	// all the intervals are always equal 0.1 so we don't care about them
	frame->interval = pin_intervals->interval;
	pin_intervals += numframes;
	ptemp = (void *)pin_intervals;

	pdaliasframe = (daliasframe_t *)ptemp;
	Q_strncpy( frame->name, pdaliasframe->name, sizeof( frame->name ));

	for( i = 0; i < numframes; i++ )
	{
		m_poseverts[m_posenum] = (trivertex_t *)((daliasframe_t *)ptemp + 1);
		ptemp = (trivertex_t *)((daliasframe_t *)ptemp + 1) + m_paliashdr->numverts;
		m_posenum++;
	}

	return ptemp;
}

/*
=================
Mod_CalcAliasBounds
=================
*/
void AliasModel :: CalcAliasBounds( void )
{
	int	i, j, k;
	vec3_t	v;

	// make bogus range
	m_aliasmins[0] = m_aliasmins[1] = m_aliasmins[2] =  999999.0f;
	m_aliasmaxs[0] = m_aliasmaxs[1] = m_aliasmaxs[2] = -999999.0f;
	floorZ = 99999.0f;

	// process verts
	for( i = 0; i < m_iNumPoses; i++ )
	{
		for( j = 0; j < m_paliashdr->numverts; j++ )
		{
			for( k = 0; k < 3; k++ )
			{
				v[k] = m_poseverts[i][j].v[k] * m_paliashdr->scale[k] + m_paliashdr->scale_origin[k];
				if( v[k] < m_aliasmins[k] ) m_aliasmins[k] = v[k];
				if( v[k] > m_aliasmaxs[k] ) m_aliasmaxs[k] = v[k];
				if( k == 2 ) floorZ = min( v[k], floorZ );
			}
		}
	}
}

daliashdr_t *AliasModel :: LoadModel( const char *modelname )
{
	FILE *fp;
	void *buffer;

	if (!modelname)
		return 0;

	// load the model
	if( (fp = fopen( modelname, "rb" )) == NULL)
		return 0;

	fseek( fp, 0, SEEK_END );
	m_iFileSize = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	buffer = malloc( m_iFileSize );
	if (!buffer)
	{
		m_iFileSize = 0;
		fclose (fp);
		return 0;
	}

	fread( buffer, m_iFileSize, 1, fp );
	fclose( fp );

	daliashdr_t	*phdr;
	stvert_t		*pinstverts;
	dtriangle_t	*pintriangles;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	int		i, j;

	phdr = (daliashdr_t *)buffer;

	if (strncmp ((const char *) buffer, "IDPO", 4 ))
	{
		if( !strncmp ((const char *) buffer, "IDST", 4) || !strncmp ((const char *) buffer, "IDSQ", 4))
			mxMessageBox( g_GlWindow, "Half-Life models doesn't supported.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		else mxMessageBox( g_GlWindow, "Unknown file format.", g_appTitle, MX_MB_OK | MX_MB_ERROR );

		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	if( phdr->version != ALIAS_VERSION )
	{
		mxMessageBox( g_GlWindow, "Unsupported alias version.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	if( phdr->numverts <= 0 )
	{
		mxMessageBox( g_GlWindow, "model has no vertices.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	if( phdr->numverts > MAXALIASVERTS )
	{
		mxMessageBox( g_GlWindow, "model has too many vertices.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	if( phdr->numtris <= 0 )
	{
		mxMessageBox( g_GlWindow, "model has no triangles.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	if( phdr->numframes < 1 )
	{
		mxMessageBox( g_GlWindow, "model has no animation frames.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		m_iFileSize = 0;
		free (buffer);
		return 0;
	}

	Q_strncpy (g_viewerSettings.modelFile, modelname, sizeof( g_viewerSettings.modelFile ));
	Q_strncpy( g_viewerSettings.modelPath, modelname, sizeof( g_viewerSettings.modelPath ));

	// set header, alloc the frames
	m_paliashdr = (daliashdr_t *)buffer;
	m_pFrames = (maliasframedesc_t *)malloc( phdr->numframes * sizeof( maliasframedesc_t ));
	m_iNumSkins = 0;

	// load the skins
	pskintype = (daliasskintype_t *)&phdr[1];
	pskintype = LoadAllSkins( phdr->numskins, pskintype );

	// load base s and t vertices
	pinstverts = (stvert_t *)pskintype;

	for( i = 0; i < phdr->numverts; i++ )
	{
		m_stverts[i].onseam = pinstverts[i].onseam;
		m_stverts[i].s = pinstverts[i].s;
		m_stverts[i].t = pinstverts[i].t;
	}

	// load triangle lists
	pintriangles = (dtriangle_t *)&pinstverts[phdr->numverts];

	for( i = 0; i < phdr->numtris; i++ )
	{
		m_triangles[i].facesfront = pintriangles[i].facesfront;

		for( j = 0; j < 3; j++ )
			m_triangles[i].vertindex[j] = pintriangles[i].vertindex[j];
	}

	// load the frames
	pframetype = (daliasframetype_t *)&pintriangles[phdr->numtris];
	m_posenum = 0;

	for( i = 0; i < phdr->numframes; i++ )
	{
		aliasframetype_t	frametype = pframetype->type;

		if( frametype == ALIAS_SINGLE )
			pframetype = (daliasframetype_t *)LoadAliasFrame( pframetype + 1, &m_pFrames[i] );
		else pframetype = (daliasframetype_t *)LoadAliasGroup( pframetype + 1, &m_pFrames[i] );
	}

	m_iNumPoses = m_posenum;
	CalcAliasBounds();

	// build the draw lists
	MakeDisplayLists();

	char	basename[64];

	COM_FileBase( modelname, basename );

	if( !Q_strnicmp( basename, "v_", 2 ))
	{
		g_MDLViewer->checkboxSet( IDC_OPTIONS_WEAPONORIGIN, true );
		bUseWeaponOrigin = true;
	}
	else
	{
		g_MDLViewer->checkboxSet( IDC_OPTIONS_WEAPONORIGIN, false );
		bUseWeaponOrigin = false;
	}

	// reset all the changes
	g_viewerSettings.numModelChanges = 0;
	g_viewerSettings.numSourceChanges = 0;
	m_numeditfields = 0;

	return (daliashdr_t *)buffer;
}

void AliasModel :: FreeModel( void )
{
	int	i;

	if( g_viewerSettings.numModelChanges )
	{
		if( !mxMessageBox( g_GlWindow, "Model has changes. Do you wish to save them?", g_appTitle, MX_MB_YESNO | MX_MB_QUESTION ))
		{
			char *ptr = (char *)mxGetSaveFileName( g_GlWindow , g_viewerSettings.modelPath, "*.mdl", g_viewerSettings.modelPath );
			if( ptr )
			{
				char filename[256];
				char ext[16];

				strcpy( filename, ptr );
				strcpy( ext, mx_getextension( filename ));
				if( mx_strcasecmp( ext, ".mdl" ))
					strcat( filename, ".mdl" );

				if( !SaveModel( filename ))
					mxMessageBox( g_GlWindow, "Error saving model.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
			}
		}

		g_viewerSettings.numModelChanges = 0;	// all the settings are handled or invalidated
	}

	if( m_paliashdr )
	{
		// deleting textures
		int textures[MAX_SKINS];
		for (i = 0; i < m_iNumSkins; i++)
			textures[i] = g_tex_base + i;

		glDeleteTextures (m_iNumSkins, (const GLuint *)textures);
		m_iNumSkins = 0;

		memset( gl_texturenum, 0, sizeof( gl_texturenum ));
		memset( fb_texturenum, 0, sizeof( fb_texturenum ));
		memset( m_skindata, 0, sizeof( m_skindata ));
	}

	if (m_paliashdr)
		free (m_paliashdr);

	if (m_pPoseData)
		free (m_pPoseData);

	if (m_pCommands)
		free (m_pCommands);

	if (m_pFrames)
		free (m_pFrames);

	g_viewerSettings.numSourceChanges = 0;
	m_newpose = m_oldpose = 0;
	m_newframe = m_oldframe = 0;
	m_paliashdr = 0;
	m_pPoseData = 0;
	m_pCommands = 0;
	m_pFrames = 0;
	remap_textures = false;
	m_numeditfields = 0;
	m_iFileSize = 0;
}

bool AliasModel :: SaveModel( const char *modelname )
{
	if (!modelname)
		return false;

	if (!m_paliashdr)
		return false;

	FILE *file;
	
	file = fopen (modelname, "wb");
	if (!file)
		return false;

	fwrite (m_paliashdr, sizeof (byte), m_iFileSize, file);
	fclose (file);
	return true;
}

int AliasModel :: SetSkin( int iValue )
{
	if (!m_paliashdr)
		return 0;

	if (iValue >= m_paliashdr->numskins)
		return m_skinnum;
	m_skinnum = iValue;

	return iValue;
}

void AliasModel :: ExtractBbox( vec3_t mins, vec3_t maxs )
{
	if( !m_paliashdr )
	{
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}

	VectorCopy( m_aliasmins, mins );
	VectorCopy( m_aliasmaxs, maxs );
	maxs[2] += ORIGIN_OFFSET;	// yes here a little trick
}

int AliasModel :: getAnimationCount( void )
{
	int i, j, pos;
	int count;
	int lastId;
	char name[16], last[16];

	if( !m_paliashdr || !m_pFrames )
		return 0;

	Q_strcpy( last, m_pFrames[0].name );
	pos = Q_strlen( last ) - 1;
	j = 0;
	while( isdigit( last[pos] ) && j < 3 )
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;

	for( i = 0; i <= m_paliashdr->numframes; i++ )
	{
		if( i == m_paliashdr->numframes )
			Q_strcpy (name, ""); // some kind of a sentinel
		else Q_strcpy( name, m_pFrames[i].name );
		pos = Q_strlen( name ) - 1;
		j = 0;
		while( isdigit( name[pos] ) && j < 3 )
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if( Q_strcmp( last, name ))
		{
			Q_strcpy( last, name );
			count++;
		}
	}

	return count;
}

const char *AliasModel :: getAnimationName( int animation )
{
	int i, j, pos;
	int count;
	int lastId;
	static char last[32];
	char name[32];

	if( !m_paliashdr || !m_pFrames )
		return "";

	Q_strcpy( last, m_pFrames[0].name );
	pos = Q_strlen( last ) - 1;
	j = 0;
	while( isdigit( last[pos] ) && j < 3 )
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;

	for( i = 0; i <= m_paliashdr->numframes; i++ )
	{
		if( i == m_paliashdr->numframes )
			Q_strcpy( name, "" ); // some kind of a sentinel
		else Q_strcpy( name, m_pFrames[i].name );
		pos = Q_strlen( name ) - 1;
		j = 0;
		while( isdigit( name[pos] ) && j < 3 )
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if( Q_strcmp( last, name ))
		{
			if( count == animation )
				return last;

			Q_strcpy( last, name );
			count++;
		}
	}

	return "";
}

void AliasModel :: getAnimationFrames( int animation, int *startFrame, int *endFrame )
{
	int i, j, pos;
	int count, numFrames, frameCount;
	int lastId;
	char name[16], last[16];

	if( !m_paliashdr || !m_pFrames )
	{
		*startFrame = *endFrame = 0;
		return;
	}

	Q_strcpy( last, m_pFrames[0].name );
	pos = Q_strlen( last ) - 1;
	j = 0;
	while( isdigit( last[pos] ) && j < 3 )
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;
	numFrames = 0;
	frameCount = 0;

	for( i = 0; i <= m_paliashdr->numframes; i++ )
	{
		if( i == m_paliashdr->numframes )
			Q_strcpy (name, ""); // some kind of a sentinel
		else Q_strcpy( name, m_pFrames[i].name );
		pos = Q_strlen( name ) - 1;
		j = 0;
		while( isdigit( name[pos] ) && j < 3 )
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if( Q_strcmp( last, name ))
		{
			Q_strcpy( last, name );

			if( count == animation )
			{
				*startFrame = frameCount - numFrames;
				*endFrame = frameCount - 1;
				return;
			}

			count++;
			numFrames = 0;
		}
		frameCount++;
		numFrames++;
	}


	*startFrame = *endFrame = 0;
}

void AliasModel:: GetMovement( vec3_t delta )
{
	delta[0] = 0.0f;//fabs( m_stepMovement[0] - m_stepMovement[1] );
	delta[1] = delta[2] = 0.0f;
}

void AliasModel::SetTopColor( int color )
{
	if( g_viewerSettings.topcolor != color )
		remap_textures = true;
	g_viewerSettings.topcolor = color;
}

void AliasModel::SetBottomColor( int color )
{
	if( g_viewerSettings.bottomcolor != color )
		remap_textures = true;
	g_viewerSettings.bottomcolor = color;
}

bool AliasModel::SetEditType( int type )
{
	if( type < 0 || type >= m_numeditfields )
	{
		m_pedit = NULL;
		return false;
	}

	m_pedit = &m_editfields[type];

	// in alias we can't change any bboxes...
	return false;
}

bool AliasModel::SetEditMode( int mode )
{
	char	str[256];

	if( mode == EDIT_MODEL && g_viewerSettings.editMode == EDIT_SOURCE && g_viewerSettings.numSourceChanges > 0 )
	{
		Q_strcpy( str, "we have some virtual changes for QC-code.\nApply them to real model or all the changes will be lost?" );
		int ret = mxMessageBox( g_GlWindow, str, g_appTitle, MX_MB_YESNOCANCEL | MX_MB_QUESTION );

		if( ret == 2 ) return false;	// cancelled
	
		if( ret == 0 ) UpdateEditFields( true );
		else UpdateEditFields( false );
	}

	g_viewerSettings.editMode = mode;
	return true;
}

void AliasModel::ReadEditField( daliashdr_t *phdr, edit_field_t *ed )
{
	if( !ed || !phdr ) return;

	// get initial values
	switch( ed->type )
	{
	case TYPE_ORIGIN:
		VectorClear( ed->origin ); // !!!
		break;
	case TYPE_EYEPOSITION:
		VectorCopy( phdr->eyeposition, ed->origin );
		break;
	}
}

void AliasModel::WriteEditField( daliashdr_t *phdr, edit_field_t *ed )
{
	if( !ed || !phdr ) return;

	// get initial values
	switch( ed->type )
	{
	case TYPE_ORIGIN:
		VectorAdd( phdr->scale_origin, ed->origin, phdr->scale_origin );
		VectorClear( ed->origin );
		break;
	case TYPE_EYEPOSITION:
		VectorCopy( ed->origin, phdr->eyeposition );
		break;
	default:	return;
	}

	g_viewerSettings.numModelChanges++;
}

void AliasModel::UpdateEditFields( bool write_to_model )
{
	daliashdr_t *phdr = g_aliasModel.getAliasHeader ();

	for( int i = 0; i < m_numeditfields; i++ )
	{
		edit_field_t *ed = &m_editfields[i];

		if( write_to_model )
			WriteEditField( phdr, ed );
		else ReadEditField( phdr, ed );
	}

	if( write_to_model && phdr )
		g_viewerSettings.numSourceChanges = 0;
}

bool AliasModel::AddEditField( int type, int id )
{
	if( m_numeditfields >= MAX_EDITFIELDS )
	{
		mxMessageBox( g_GlWindow, "Edit fields limit exceeded.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
		return false;
	}

	daliashdr_t *phdr = g_aliasModel.getAliasHeader ();

	if( phdr )
	{
		edit_field_t *ed = &m_editfields[m_numeditfields++];

		// initialize fields
		VectorClear( ed->origin );
		ed->type = type;
		ed->id = id;

		// read initial values from studiomodel
		ReadEditField( phdr, ed );

		return true;
	}

	return false;
}

const char *AliasModel::getQCcode( void )
{
	daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
	static char str[256];

	if( !m_pedit ) return "";

	edit_field_t *ed = m_pedit;

	Q_strncpy( str, "not available", sizeof( str ));
	if( !phdr ) return str;

	// get initial values
	switch( ed->type )
	{
	case TYPE_ORIGIN:
		if( g_viewerSettings.editMode == EDIT_SOURCE )
			Q_snprintf( str, sizeof( str ), "$origin %g %g %g", -ed->origin[1], ed->origin[0], -ed->origin[2] );
		break;
	case TYPE_EYEPOSITION:
		Q_snprintf( str, sizeof( str ), "$eyeposition %g %g %g", ed->origin[1], ed->origin[0], ed->origin[2] );
		break;
	}

	return str;
}

void AliasModel::updateModel( void )
{
	if( !update_model ) return;
	WriteEditField( m_paliashdr, m_pedit );
	update_model = false;
}

void AliasModel::editPosition( float step, int type )
{
	if( !m_pedit ) return;

	switch( type )
	{
	case IDC_MOVE_PX: m_pedit->origin[0] += step; break;
	case IDC_MOVE_NX: m_pedit->origin[0] -= step; break;
	case IDC_MOVE_PY: m_pedit->origin[1] += step; break;
	case IDC_MOVE_NY: m_pedit->origin[1] -= step; break;
	case IDC_MOVE_PZ: m_pedit->origin[2] += step; break;
	case IDC_MOVE_NZ: m_pedit->origin[2] -= step; break;
	}

	if( g_viewerSettings.editMode == EDIT_MODEL )
		update_model = true;
	else g_viewerSettings.numSourceChanges++;
}