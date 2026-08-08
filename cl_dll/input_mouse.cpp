#include "input_mouse.h"
#include "exportdef.h"
#include "hud.h"
#include "cl_util.h"

// shared between backends
Vector dead_viewangles(0, 0, 0);
cvar_t      *sensitivity;
cvar_t  *in_joystick;

FWGSInput fwgsInput;
AbstractInput* currentInput = &fwgsInput;

extern "C"  void DLLEXPORT IN_ClientMoveEvent( float forwardmove, float sidemove )
{
	currentInput->IN_ClientMoveEvent(forwardmove, sidemove);
}

extern "C" void DLLEXPORT IN_ClientLookEvent( float relyaw, float relpitch )
{
	currentInput->IN_ClientLookEvent(relyaw, relpitch);
}

void IN_Move( float frametime, usercmd_t *cmd )
{
	currentInput->IN_Move(frametime, cmd);
}

extern "C" void DLLEXPORT IN_MouseEvent( int mstate )
{
	currentInput->IN_MouseEvent(mstate);
}

extern "C" void DLLEXPORT IN_ClearStates( void )
{
	currentInput->IN_ClearStates();
}

extern "C" void DLLEXPORT IN_ActivateMouse( void )
{
	currentInput->IN_ActivateMouse();
}

extern "C" void DLLEXPORT IN_DeactivateMouse( void )
{
	currentInput->IN_DeactivateMouse();
}

extern "C" void DLLEXPORT IN_Accumulate( void )
{
	currentInput->IN_Accumulate();
}

void IN_Commands( void )
{
	currentInput->IN_Commands();
}

void IN_Shutdown( void )
{
	currentInput->IN_Shutdown();
}

void IN_Init( void )
{
	currentInput = &fwgsInput;
	currentInput->IN_Init();
}
