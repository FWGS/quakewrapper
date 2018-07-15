/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#ifndef INCLUDED_ALIASMODEL
#define INCLUDED_ALIASMODEL

#include <basetypes.h>
#include <mathlib.h>

/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/
#define MAXALIASVERTS	2048
#define MAXALIASFRAMES	256
#define MAXALIASTRIS	4096
#define MAX_SKINS		32

#define SHIRT_HUE_START	16
#define SHIRT_HUE_END	32
#define PANTS_HUE_START	96
#define PANTS_HUE_END	112

#define CROSS_LENGTH	18.0f

#define ORIGIN_OFFSET	24.5f

#define TYPE_ORIGIN		0
#define TYPE_EYEPOSITION	1
#define MAX_EDITFIELDS	2

// This mirrors trivert_t in trilib.h, is present so Quake knows how to
// load this data
typedef struct
{
	byte		v[3];
	byte		lightnormalindex;
} trivertex_t;

#include "alias.h"

typedef struct
{
	int		firstpose;
	int		numposes;
	trivertex_t	bboxmin;
	trivertex_t	bboxmax;
	float		interval;
	char		name[16];
} maliasframedesc_t;

typedef struct
{
	int		type;
	int		id;		// #id for mutilple count
	vec3_t		origin;
} edit_field_t;

class AliasModel
{
public:
	daliashdr_t	*getAliasHeader () const { return m_paliashdr; }
	void		updateTimings( float flTime, float flFrametime ) { m_flTime = flTime; m_flFrameTime = flFrametime; }
	float		getCurrentTime( void ) { return m_flTime; }
	void		UploadTexture( byte *data, int width, int height, byte *palette, int name, bool luma = false );
	void		PaletteHueReplace( byte *palSrc, int top, int bottom );
	void		setAnimation( int animation ) { m_animation = animation; }
	void		SetOffset2D( float x, float y ) { offset_x = x, offset_y = y; }
	byte		*GetSkinData( int index ) { return m_skindata[index]; }
	int		GetTextureSkin( int skinnum, int anim = 0 ) { return gl_texturenum[skinnum][anim]; }
	int		GetNumPoses( int index ) { return m_pFrames ? m_pFrames[index].numposes : 0; }
	void		DrawAliasNormals( void );
	void		DrawAliasUVMap( void );
	void		RemapTextures( void );
	void		FreeModel ();
	daliashdr_t	*LoadModel( const char *modelname );
	bool		PostLoadModel ( char *modelname );
	bool		SaveModel ( const char *modelname );
	void		DrawModel( bool bMirror = false );
	void		DrawModelUVMap();
	bool		AdvanceFrame( float dt );
	int		SetSkin( int iValue );
	int		setFrame( int newframe, bool nolerp = false );

	int		getAnimationCount( void );
	const char	*getAnimationName( int animation );
	void		getAnimationFrames( int animation, int *startFrame, int *endFrame );
	void		ExtractBbox( vec3_t mins, vec3_t maxs );
	void		GetMovement( vec3_t delta );

	bool		SetEditType( int iType );
	bool		SetEditMode( int iMode );
	bool		AddEditField( int type, int id );
	void		ReadEditField( daliashdr_t *phdr, edit_field_t *ed );
	void		WriteEditField( daliashdr_t *phdr, edit_field_t *ed );
	void		UpdateEditFields( bool write_to_model );

	void		SetTopColor( int color );
	void		SetBottomColor( int color );

	void		centerView( bool reset );
	void		updateModel( void );
	void		editPosition( float step, int type );
	edit_field_t	*editField( void ) { return m_pedit; }
	const char	*getQCcode( void );
	float		getFloor() { return floorZ; }
private:
	// global settings
	float		m_flTime;
	float		m_flFrameTime;
	float		m_flAnimTime[2];
	float		m_stepMovement[2];

	// entity settings
	int		m_oldpose;	// shadow used
	int		m_newpose;	// shadow used
	float		m_flLerpfrac;	// lerp frames
	int		m_animation;	// sequence index
	float		m_dt;
	int		m_skinnum;	// skin group selection
	int		m_newframe;
	int		m_oldframe;
	float		offset_x, offset_y;
	bool		remap_textures;
	int		update_model;	// set to true if model was edited
	float		floorZ;

	matrix4x4		m_protationmatrix;

	// internal data
	daliashdr_t	*m_paliashdr;	// pointer to souremodel
	int		m_iFileSize;	// real size of model footprint

	int		m_iNumSkins;
	int		m_iNumPoses;
	int		m_iPoseVerts;
	trivertex_t	*m_pPoseData;	// numposes * poseverts trivert_t
	int		*m_pCommands;	// gl command list with embedded s/t
	maliasframedesc_t	*m_pFrames;	// variable sized
	unsigned short	gl_texturenum[MAX_SKINS][4];
	unsigned short	fb_texturenum[MAX_SKINS][4];
	byte		*m_skindata[MAX_SKINS];
	vec3_t		m_aliasmins;
	vec3_t		m_aliasmaxs;

	edit_field_t	m_editfields[MAX_EDITFIELDS];
	int		m_numeditfields;
	edit_field_t	*m_pedit;

	void 		SetupTransform( bool bMirror = false );
	void		Lighting( float *lv, const vec3_t normal );
	void		SetupLighting( void );
	void		SetupFrame( void );
	void		DrawAliasFrame( bool bFullbright, bool bWireframe = false );

	// loading stuff
	void		*LoadSingleSkin( daliasskintype_t *pskintype, int skinnum, int size );
	void		*LoadGroupSkin( daliasskintype_t *pskintype, int skinnum, int size );
	daliasskintype_t	*LoadAllSkins( int numskins, daliasskintype_t *pskintype );
	void		*LoadAliasFrame( void *pin, maliasframedesc_t *frame );
	void		*LoadAliasGroup( void *pin, maliasframedesc_t *frame );
	void		CalcAliasBounds( void );

	// building stuff
	int		StripLength( int starttri, int startv );
	int		FanLength( int starttri, int startv );
	void		BuildTris( void );
	void		MakeDisplayLists( void );

	// intermediate arrays who used for build tri-strips
	trivertex_t	*m_poseverts[MAXALIASFRAMES];
	dtriangle_t	m_triangles[MAXALIASTRIS];
	stvert_t		m_stverts[MAXALIASVERTS];
	int		m_used[8192];

	// a pose is a single set of vertexes. a frame may be
	// an animating sequence of poses
	int		m_posenum;

	// the command list holds counts and s/t values that are valid for
	// every frame
	int		m_commands[8192];
	int		m_numcommands;

	// all frames will have their vertexes rearranged and expanded
	// so they are in the order expected by the command list
	int		m_vertexorder[8192];
	int		m_numorder;

	int		m_stripverts[128];
	int		m_striptris[128];
	int		m_stripcount;
};

extern AliasModel	g_aliasModel;
extern byte	palette_q1[];

#endif