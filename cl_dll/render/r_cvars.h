#pragma once
#ifndef R_CVARS_H
#define R_CVARS_H

#ifndef NO_EXTERN
#define EXTERN extern
#else
#define EXTERN
#endif

// engine cvars
EXTERN cvar_t *r_norefresh;
EXTERN cvar_t *r_speeds;
EXTERN cvar_t *gl_finish;

// renderer cvars
EXTERN cvar_t *gl_renderer;
EXTERN cvar_t *r_stereo;

#endif // R_CVARS_H
