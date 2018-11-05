/*
r_main.c -- renderer main
Copyright (C) 2018 a1batross
*/

#include <string.h>
#include "cl_dll.h"
#include "cl_util.h"
#include "const.h"
#include "render_api.h"
#include "exportdef.h"
#include "ref_params.h"

#define NO_EXTERN
#include "r_cvars.h"

qboolean g_fRenderInitialized = FALSE;

/*
================
GL_RenderFrame

passed through R_RenderFrame (0 - use engine renderer, 1 - use custom client renderer)
================
*/
int GL_RenderFrame( const ref_viewpass_t *rvp )
{
	double time1, time2;

	return 0;
}

/*
================
GL_BuildLightmaps

build all the lightmaps on new level or when gamma is changed
================
*/
void GL_BuildLightmaps( void )
{

}

/*
================
GL_BuildLightmaps

setup map bounds for ortho-projection when we in dev_overview mode
================
*/
void GL_OrthoBounds( const float *mins, const float *maxs )
{

}

/*
================
R_CreateStudioDecalList

prepare studio decals for save
================
*/
int R_CreateStudioDecalList( decallist_t *pList, int count )
{

}


/*
================
R_ClearStudioDecals

clear decals by engine request (e.g. for demo recording or vid_restart)
================
*/
void R_ClearStudioDecals( void )
{

}
/*
================
R_SpeedsMessage

grab r_speeds message
================
*/
qboolean R_SpeedsMessage( char *out, size_t size )
{
#if 0
	switch( (int)r_speeds->value )
	{
	case 2:
		snprintf(out, size, "%3i ms  %4i/%4i wpoly %4i/%4i epoly %3i lmap %4i/%4i sky %1.1f mtex\n",
			(int)((time2-time1)*1000),
			rs_brushpolys,
			rs_brushpasses,
			rs_aliaspolys,
			rs_aliaspasses,
			rs_dynamiclightmaps,
			rs_skypolys,
			rs_skypasses,
			TexMgr_FrameUsage ());
		return true;
	default:
		snprintf( out, size, "%3i ms  %4i wpoly %4i epoly %3i lmap\n",
			(int)((time2-time1)*1000),
			rs_brushpolys,
			rs_aliaspolys,
			rs_dynamiclightmaps);
		return true;
	}
#endif

	return false;
}

/*
================
Mod_ProcessUserData

alloc or destroy model custom data
================
*/
void Mod_ProcessUserData( struct model_s *mod, qboolean create, const byte *buffer )
{

}

/*
================
R_ProcessEntData

alloc or destroy entity custom data
================
*/
void R_ProcessEntData( qboolean allocate )
{

}

/*
================
Mod_GetCurrentVis

get visdata for current frame from custom renderer
================
*/
byte* Mod_GetCurrentVis( void )
{

}

/*
================
R_NewMap

tell the renderer what new map is started
================
*/
void R_NewMap( void )
{

}

/*
================
R_ClearScene

clear the render entities before each frame
================
*/
void R_ClearScene( void )
{

}

void R_Init( void )
{
	// engine cvars
	gl_finish = gEngfuncs.pfnGetCvarPointer( "gl_finish" );
	r_norefresh = gEngfuncs.pfnGetCvarPointer( "r_norefresh" );
	r_speeds = gEngfuncs.pfnGetCvarPointer( "r_speeds" );

	// renderer cvars
	gl_renderer = CVAR_CREATE( "gl_renderer", "0", FCVAR_ARCHIVE );
	r_stereo = CVAR_CREATE( "r_stereo", "0", FCVAR_ARCHIVE );
}

void R_Shutdown( void )
{

}

//
// Xash3D render interface
//
static render_interface_t gRenderInterface =
{
	CL_RENDER_INTERFACE_VERSION,
	GL_RenderFrame,
	GL_BuildLightmaps,
	GL_OrthoBounds,
	R_CreateStudioDecalList,
	R_ClearStudioDecals,
	R_SpeedsMessage,
	Mod_ProcessUserData,
	R_ProcessEntData,
	Mod_GetCurrentVis,
	R_NewMap,
	R_ClearScene
};

extern "C" int DLLEXPORT HUD_GetRenderInterface( int version, render_api_t *renderfuncs, render_interface_t *callback )
{
	if ( !callback || !renderfuncs || version != CL_RENDER_INTERFACE_VERSION )
	{
		return FALSE;
	}

	size_t iImportSize = sizeof( render_interface_t );
	size_t iExportSize = sizeof( render_api_t );

	// copy new physics interface
	memcpy( &gRenderfuncs, renderfuncs, iExportSize );

	// fill engine callbacks
	memcpy( callback, &gRenderInterface, iImportSize );

	g_fRenderInitialized = TRUE;

	return TRUE;
}
