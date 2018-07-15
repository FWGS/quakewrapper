/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "stringlib.h"
#include "ref_params.h"
#include "triangleapi.h"
#include "wadfile.h"

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up stupid hardware / drivers

=============================================================================
*/
// returns a texture number and the position inside it
int CHud :: ScrapAllocBlock( int w, int h, int *x, int *y )
{
	int	i, j, texnum;
	int	best, best2;

	for( texnum = 0; texnum < MAX_SCRAPS; texnum++ )
	{
		best = BLOCK_HEIGHT;

		for( i = 0; i < BLOCK_WIDTH - w; i++ )
		{
			best2 = 0;

			for( j = 0; j < w; j++ )
			{
				if( scrap_allocated[texnum][i+j] >= best )
					break;
				if( scrap_allocated[texnum][i+j] > best2 )
					best2 = scrap_allocated[texnum][i+j];
			}

			if( j == w )
			{
				// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if( best + h > BLOCK_HEIGHT )
			continue;

		for( i = 0; i < w; i++ )
			scrap_allocated[texnum][*x + i] = best + h;
		return texnum;
	}

	return -1;
}

void CHud :: ScrapUpload( void )
{
	char	scrap_name[16];
	size_t	img_size = sizeof( lmp_t ) + BLOCK_WIDTH * BLOCK_HEIGHT;
	byte	*buffer = (byte *)malloc( img_size );
	byte	*p = buffer;
	lmp_t	*hdr = (lmp_t *)p;

	hdr->width = BLOCK_WIDTH;
	hdr->height = BLOCK_HEIGHT;
	p += sizeof( lmp_t ); 

	for( int texnum = 0; texnum < MAX_SCRAPS; texnum++ )
	{
		sprintf( scrap_name, "#scrap%i.lmp", texnum );
		memcpy( p, scrap_texels[texnum], BLOCK_WIDTH * BLOCK_HEIGHT );
		scrap_texnums[texnum] = gRenderfuncs.GL_LoadTexture( scrap_name, buffer, img_size, TF_CLAMP|TF_NOMIPMAP ); 
	}

	scrap_dirty = false;
	free( buffer );
}

glpic_t *CHud :: DrawPicFromWad( const char *name, bool fullpath )
{
	char	path[64];
	glpic_t	*gl;
	lmp_t	*p;

	// check if already loaded
	for( int i = 0; i < num_cache_pics; i++ )
	{
		gl = &cache_pics[i];
		if( !Q_strcmp( gl->name, name ))
			return gl;
	}

	if( fullpath ) Q_strncpy( path, name, sizeof( path ));
	else sprintf( path, "gfx/%s.lmp", name );

	p = (lmp_t *)gEngfuncs.COM_LoadFile( path, 5, NULL );

	if( !p ) return NULL; // image was missed?

	gl = &cache_pics[num_cache_pics];

	Q_strncpy( gl->name, name, sizeof( gl->name ));
	gl->width = p->width;
	gl->height = p->height;

	// load little ones into the scrap
	if( p->width < 64 && p->height < 64 )
	{
		int		x, y;
		unsigned int	i, j, k;
		int		texnum;
		byte		*data = (byte *)(p + 1);

		if(( texnum = ScrapAllocBlock( p->width, p->height, &x, &y )) == -1 )
		{
			gEngfuncs.COM_FreeFile( p );
			return NULL;
		}

		scrap_dirty = true;

		// accumulate pixels into the atlas
		for( i = k = 0; i < p->height; i++ )
			for( j = 0; j < p->width; j++, k++ )
				scrap_texels[texnum][(y + i) * BLOCK_WIDTH + x + j] = data[k];

		gl->sl = (x + 0.01f) / (float)BLOCK_WIDTH;
		gl->sh = (x + p->width - 0.01f) / (float)BLOCK_WIDTH;
		gl->tl = (y + 0.01) / (float)BLOCK_WIDTH;
		gl->th = (y + p->height - 0.01f) / (float)BLOCK_WIDTH;
		gl->texnum = texnum;
		gl->scrap = true;
	}
	else
	{
		gl->texnum = gRenderfuncs.GL_LoadTexture( path, NULL, 0, TF_CLAMP|TF_NOMIPMAP );
		gl->scrap = false;
		gl->sl = 0.0f;
		gl->sh = 1.0f;
		gl->tl = 0.0f;
		gl->th = 1.0f;
	}

	gEngfuncs.COM_FreeFile( p );

	if( num_cache_pics < MAX_CACHE_PICS )
	{
		num_cache_pics++;
		return gl;
	}

	gEngfuncs.Con_Printf( "^3Error:^7 DrawPicFromWad: MAX_CACHE_PICS exceeds\n" );
	return NULL;
}

/*
=============
Draw_PicGeneric
=============
*/
void CHud :: DrawPicGeneric( int x, int y, glpic_t *gl )
{
	if( !gl ) return;

	if( scrap_dirty )
		ScrapUpload();

	gEngfuncs.pTriAPI->Color4f( 1.0f, 1.0f, 1.0f, 1.0f );

	if( gl->scrap )
		gRenderfuncs.GL_Bind( 0, scrap_texnums[gl->texnum] );
	else gRenderfuncs.GL_Bind( 0, gl->texnum );

	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	gEngfuncs.pTriAPI->TexCoord2f( gl->sl, gl->tl );
	gEngfuncs.pTriAPI->Vertex3f( x, y, 0.0f );
	gEngfuncs.pTriAPI->TexCoord2f( gl->sh, gl->tl );
	gEngfuncs.pTriAPI->Vertex3f( x + gl->width, y, 0.0f );
	gEngfuncs.pTriAPI->TexCoord2f( gl->sh, gl->th );
	gEngfuncs.pTriAPI->Vertex3f( x + gl->width, y + gl->height, 0.0f );
	gEngfuncs.pTriAPI->TexCoord2f( gl->sl, gl->th );
	gEngfuncs.pTriAPI->Vertex3f( x, y + gl->height, 0.0f );
	gEngfuncs.pTriAPI->End ();
}

/*
=============
Draw_Pic
=============
*/
void CHud :: DrawPic( int x, int y, glpic_t *pic )
{
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	DrawPicGeneric( x, y, pic );
}

/*
=============
Draw_TransPic
=============
*/
void CHud :: DrawTransPic( int x, int y, glpic_t *pic )
{
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
	DrawPicGeneric( x, y, pic );
}