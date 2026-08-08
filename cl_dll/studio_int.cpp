/*
studio_int.cpp - capture-only studio model interface
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

#include <string.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "r_studioint.h"

engine_studio_api_t IEngineStudio;

extern "C" int DLLEXPORT HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio )
{
	if( version != STUDIO_INTERFACE_VERSION )
		return 0;

	// grab the engine model accessors...
	memcpy( &IEngineStudio, pstudio, sizeof( IEngineStudio ));

	// ...but decline to provide a renderer, the engine will use its own
	return 0;
}
