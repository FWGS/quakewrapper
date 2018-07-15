//
//                 MDL Viewer (c) 1999 by Mete Ciragan
//
// file:           md2viewer.h
// last modified:  Apr 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.4
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#ifndef INCLUDED_MDLVIEWER
#define INCLUDED_MDLVIEWER



#ifndef INCLUDED_MXWINDOW
#include <mx/mxWindow.h>
#endif

#define IDC_FILE_LOADMODEL			1001
#define IDC_FILE_SAVEMODEL			1002
#define IDC_FILE_LOADBACKGROUNDTEX		1003
#define IDC_FILE_LOADGROUNDTEX		1004
#define IDC_FILE_UNLOADGROUNDTEX		1005
#define IDC_FILE_OPENPAKFILE			1006
#define IDC_FILE_OPENPAKFILE2			1007
#define IDC_FILE_CLOSEPAKFILE			1008
#define IDC_FILE_RECENTMODELS1		1009
#define IDC_FILE_RECENTMODELS2		1010
#define IDC_FILE_RECENTMODELS3		1011
#define IDC_FILE_RECENTMODELS4		1012
#define IDC_FILE_RECENTPAKFILES1		1013
#define IDC_FILE_RECENTPAKFILES2		1014
#define IDC_FILE_RECENTPAKFILES3		1015
#define IDC_FILE_RECENTPAKFILES4		1016
#define IDC_FILE_EXIT			1017

#define IDC_OPTIONS_COLORBACKGROUND		1101
#define IDC_OPTIONS_COLORGROUND		1102
#define IDC_OPTIONS_COLORLIGHT		1103
#define IDC_OPTIONS_CENTERVIEW		1104
#define IDC_OPTIONS_RESETVIEW			1105
#define IDC_OPTIONS_MAKESCREENSHOT		1106
#define IDC_OPTIONS_WEAPONORIGIN		1107
#define IDC_OPTIONS_LEFTHAND			1108
#define IDC_OPTIONS_AUTOPLAY			1109
#define IDC_OPTIONS_DUMP			1110

#define IDC_VIEW_FILEASSOCIATIONS		1201

#define IDC_HELP_GOTOHOMEPAGE			1301
#define IDC_HELP_ABOUT			1302

// control panel
#define TAB_MODELDISPLAY			0	// render options, change rendermode, show info
#define TAB_TEXTURES			1	// texture browser
#define TAB_SEQUENCES			2	// sequence browser
#define TAB_MISC				3	// model flags, remapping
#define TAB_MODELEDITOR			4	// built-in model editor

#define IDC_TAB				1901
#define IDC_RENDERMODE			2001
#define IDC_TRANSPARENCY			2002
#define IDC_GROUND				2003
#define IDC_MIRROR				2004
#define IDC_BACKGROUND			2005
#define IDC_FULLBRIGHTS			2006
#define IDC_ADJUST_ORIGIN			2007
#define IDC_EYEPOSITION			2008
#define IDC_NORMALS				2009
#define IDC_WIREFRAME			2010

#define IDC_BODYPART			3001
#define IDC_SUBMODEL			3002
#define IDC_CONTROLLER			3003
#define IDC_CONTROLLERVALUE			3004
#define IDC_SKINS				3005

#define IDC_TEXTURES			4001
#define IDC_EXPORTTEXTURE			4002
#define IDC_IMPORTTEXTURE			4003
#define IDC_EXPORT_UVMAP			4004
#define IDC_TEXTURESCALE			4005
#define IDC_TEXANIM				4006

#define IDC_SHOW_UV_MAP			4020
#define IDC_OVERLAY_UV_MAP			4021
#define IDC_ANTI_ALIAS_LINES			4022

#define IDC_ANIMATION			5001
#define IDC_SPEEDSCALE			5002
#define IDC_STOP				5003
#define IDC_PREVFRAME			5004
#define IDC_FRAME				5005
#define IDC_NEXTFRAME			5006

#define IDC_MISC				6001
#define IDC_ALIAS_ROCKET			6002	// leave a trail
#define IDC_ALIAS_GRENADE			6003	// leave a trail
#define IDC_ALIAS_GIB			6004	// leave a trail
#define IDC_ALIAS_ROTATE			6005	// rotate (bonus items)
#define IDC_ALIAS_TRACER			6006	// green split trail
#define IDC_ALIAS_ZOMGIB			6007	// small blood trail
#define IDC_ALIAS_TRACER2			6008	// orange split trail + rotate
#define IDC_ALIAS_TRACER3			6009	// purple trail
#define IDC_ALIAS_AMBIENT_LIGHT		6010	// dynamically get lighting from floor or ceil (flying monsters)
#define IDC_ALIAS_TRACE_HITBOX		6011	// always use hitbox trace instead of bbox
#define IDC_ALIAS_FORCE_SKYLIGHT		6012	// always grab lightvalues from the sky settings (even if sky is invisible)
#define IDC_TOPCOLOR			6013
#define IDC_BOTTOMCOLOR			6014

#define IDC_EDITOR				7001
#define IDC_MOVE_PX				7002
#define IDC_MOVE_NX				7003
#define IDC_MOVE_PY				7004
#define IDC_MOVE_NY				7005
#define IDC_MOVE_PZ				7006
#define IDC_MOVE_NZ				7007
#define IDC_EDIT_TYPE			7008
#define IDC_EDIT_MODE			7009
#define IDC_EDIT_STEP			7010
#define IDC_EDIT_SIZE			7011

class mxTab;
class mxMenuBar;
class mxButton;
class mxLineEdit;
class mxLabel;
class mxChoice;
class mxCheckBox;
class mxSlider;
class GlWindow;
class PAKViewer;



class MDLViewer : public mxWindow
{
	mxMenuBar *mb;
	mxTab *tab;
	GlWindow *d_GlWindow;
	mxChoice *cRenderMode;
	mxLabel *lOpacityValue;
	mxSlider *slTransparency;
	mxCheckBox *cbGround, *cbMirror, *cbBackground;

	mxChoice *cAnim;
	mxSlider *slSpeedScale;
	mxButton *tbStop;
	mxButton *bPrevFrame, *bNextFrame;
	mxLabel *lSequenceInfo;
	mxLineEdit *leFrame;

	mxChoice *cBodypart, *cController, *cSubmodel;
	mxLabel *BodyPartLabel;
	mxSlider *slController;
	mxChoice *cSkin;
	mxLabel *lModelInfo1;
	mxChoice *cTextures;
	mxChoice *cTexAnim;
	mxCheckBox *cbShowUVMap;
	mxCheckBox *cbOverlayUVMap;
	mxCheckBox *cbAntiAliasLines;
	mxLabel *lTexSize;
	mxLabel *lTexScale;
	mxCheckBox *cbFlagRocket;
	mxCheckBox *cbFlagGrenade;
	mxCheckBox *cbFlagGib;
	mxCheckBox *cbFlagRotate;
	mxCheckBox *cbFlagTracer;
	mxCheckBox *cbFlagZomgib;
	mxCheckBox *cbFlagTracer2;
	mxCheckBox *cbFlagTracer3;
	mxSlider *slTopColor;
	mxSlider *slBottomColor;
	mxChoice *cEditMode;
	mxChoice *cEditType;
	mxLineEdit *leEditStep;
	mxLineEdit *leEditString;

	mxButton *tbMovePosX;
	mxButton *tbMoveNegX;
	mxButton *tbMovePosY;
	mxButton *tbMoveNegY;
	mxButton *tbMovePosZ;
	mxButton *tbMoveNegZ;

	PAKViewer *pakViewer;

	void loadRecentFiles ();
	void saveRecentFiles ();
	void initRecentFiles ();

	void setModelInfo ();
	void initAnimation (int animation);
	void initTextures( void );
	void initTexAnim( int anim );
	void fullscreen ();

public:
	friend PAKViewer;

	// CREATORS
	MDLViewer ();
	~MDLViewer ();

	// MANIPULATORS
	virtual int handleEvent (mxEvent *event);
	void checkboxSet( int id, bool bState );
	void redraw ();
	void makeScreenShot (const char *filename);
	void setRenderMode (int index);
	void setShowGround (bool b);
	void setMirror (bool b);
	void setShowBackground (bool b);
	int getTableIndex();
	void addEditType ( const char *name, int type, int id = -1 );
	bool loadModel( const char *ptr, bool centering = true );
	void centerModel ();

	// ACCESSORS
	mxMenuBar *getMenuBar () const { return mb; }
};



extern MDLViewer *g_MDLViewer;
extern char g_appTitle[];


#endif // INCLUDED_MDLVIEWER