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
#include "extdll.h"
#include "util.h"
#include "saverestore.h"
#include "com_model.h"
#include "client.h"
#include "game.h"

extern "C" void PM_Move ( struct playermove_s *ppmove, int server );
extern "C" void PM_Init ( struct playermove_s *ppmove  );
extern "C" char PM_FindTextureType( char *name );

// Holds engine functionality callbacks
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
server_physics_api_t g_physfuncs;

static DLL_FUNCTIONS gFunctionTable = 
{
	GameDLLInit,		// pfnGameInit
	DispatchSpawn,		// pfnSpawn
	DispatchThink,		// pfnThink
	DispatchUse,		// pfnUse
	DispatchTouch,		// pfnTouch
	DispatchBlocked,		// pfnBlocked
	DispatchKeyValue,		// pfnKeyValue
	DispatchSave,		// pfnSave
	DispatchRestore,		// pfnRestore
	DispatchObjectCollisionBox,	// pfnAbsBox

	SaveWriteFields,		// pfnSaveWriteFields
	SaveReadFields,		// pfnSaveReadFields

	SaveGlobalState,		// pfnSaveGlobalState
	RestoreGlobalState,		// pfnRestoreGlobalState
	ResetGlobalState,		// pfnResetGlobalState

	ClientConnect,		// pfnClientConnect
	ClientDisconnect,		// pfnClientDisconnect
	ClientKill,		// pfnClientKill
	ClientPutInServer,		// pfnClientPutInServer
	ClientCommand,		// pfnClientCommand
	ClientUserInfoChanged,	// pfnClientUserInfoChanged
	ServerActivate,		// pfnServerActivate
	ServerDeactivate,		// pfnServerDeactivate

	PlayerPreThink,		// pfnPlayerPreThink
	PlayerPostThink,		// pfnPlayerPostThink

	StartFrame,		// pfnStartFrame
	ParmsNewLevel,		// pfnParmsNewLevel
	ParmsChangeLevel,		// pfnParmsChangeLevel

	GetGameDescription,		// pfnGetGameDescription	Returns string describing current .dll game.
	PlayerCustomization,	// pfnPlayerCustomization	Notifies .dll of new customization for player.

	SpectatorConnect,		// pfnSpectatorConnect	Called when spectator joins server
	SpectatorDisconnect,	// pfnSpectatorDisconnect	Called when spectator leaves the server
	SpectatorThink,		// pfnSpectatorThink	Called when spectator sends a command packet (usercmd_t)

	Sys_Error,		// pfnSys_Error		Called when engine has encountered an error

	PM_Move,			// pfnPM_Move
	PM_Init,			// pfnPM_Init		Server version of player movement initialization
	PM_FindTextureType,		// pfnPM_FindTextureType
	
	SetupVisibility,		// pfnSetupVisibility	Set up PVS and PAS for networking for this client
	UpdateClientData,		// pfnUpdateClientData	Set up data sent only to specific client
	AddToFullPack,		// pfnAddToFullPack
	CreateBaseline,		// pfnCreateBaseline	Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,		// pfnRegisterEncoders	Callbacks for network encoding
	GetWeaponData,		// pfnGetWeaponData
	CmdStart,			// pfnCmdStart
	CmdEnd,			// pfnCmdEnd
	ConnectionlessPacket,	// pfnConnectionlessPacket
	GetHullBounds,		// pfnGetHullBounds
	CreateInstancedBaselines,	// pfnCreateInstancedBaselines
	InconsistentFile,		// pfnInconsistentFile
	AllowLagCompensation,	// pfnAllowLagCompensation
};

NEW_DLL_FUNCTIONS gNewDLLFunctions =
{
	OnFreeEntPrivateData,	// pfnOnFreeEntPrivateData
	GameDLLShutdown,		// pfnGameShutdown
	ShouldCollide,		// pfnShouldCollide
};

int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
	if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
	{
		return FALSE;
	}

	if( !CVAR_GET_POINTER( "host_gameloaded" ))
		return FALSE; // not a Xash3D
	
	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
	return TRUE;
}

int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if ( !pFunctionTable || *interfaceVersion != INTERFACE_VERSION )
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return FALSE;
	}

	if( !CVAR_GET_POINTER( "host_gameloaded" ))
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return FALSE; // not a Xash3D
	}	
	
	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
	return TRUE;
}

int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if( !pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION )
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return FALSE;
	}

	if( !CVAR_GET_POINTER( "host_gameloaded" ))
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return FALSE; // not a Xash3D
	}

	memcpy( pFunctionTable, &gNewDLLFunctions, sizeof( gNewDLLFunctions ));
	return TRUE;
}

int DispatchSpawn( edict_t *pent )
{
	HOST_ERROR( "pfnSpawn: %s\n", STRING( pent->v.classname ));
	return -1; // return that this entity should be deleted
}

void DispatchKeyValue( edict_t *pent, KeyValueData *pkvd )
{
	HOST_ERROR( "pfnKeyValue: %s\n", STRING( pent->v.classname ));
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	HOST_ERROR( "pfnTouch: %s\n", STRING( pentTouched->v.classname ));
	HOST_ERROR( "pfnTouch: %s\n", STRING( pentOther->v.classname ));
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
	HOST_ERROR( "pfnUse: %s\n", STRING( pentUsed->v.classname ));
	HOST_ERROR( "pfnUse: %s\n", STRING( pentOther->v.classname ));
}

void DispatchThink( edict_t *pent )
{
	HOST_ERROR( "pfnThink: %s\n", STRING( pent->v.classname ));
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
	HOST_ERROR( "pfnBlocked: %s\n", STRING( pentBlocked->v.classname ));
	HOST_ERROR( "pfnBlocked: %s\n", STRING( pentOther->v.classname ));
}

void OnFreeEntPrivateData( edict_s *pEdict )
{
}

// Required DLL entry point
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;

	g_iXashEngineBuildNumber = CVAR_GET_FLOAT( "build" ); // 0 for old builds or GoldSrc
	if( g_iXashEngineBuildNumber <= 0 )
		g_iXashEngineBuildNumber = (int)CVAR_GET_FLOAT( "buildnum" );
}