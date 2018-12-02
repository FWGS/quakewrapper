//
//                 MDL Viewer (c) 1999 by Mete Ciragan
//
// file:           md2viewer.cpp
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
#include <stdio.h>
#include <stdlib.h>
//#include <ostream.h>
#include <mx.h>
#include "mxtk/gl.h"
#include <mxBmp.h>
#include "mdlviewer.h"
#include "GlWindow.h"
#include "pakviewer.h"
#include "FileAssociation.h"
#include "stringlib.h"
#include "AliasModel.h"

MDLViewer *g_MDLViewer = 0;
char g_appTitle[] = "Quake 1 Model Viewer v0.60 stable";
static char recentFiles[8][256] = { "", "", "", "", "", "", "", "" };
extern bool bUseWeaponOrigin;
extern bool bUseWeaponLeftHand;
bool g_bStopPlaying = false;
bool g_bEndOfSequence = false;
static int g_nCurrFrame = 0;

void MDLViewer::initRecentFiles ()
{
	for (int i = 0; i < 8; i++)
	{
		if (strlen (recentFiles[i]))
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, recentFiles[i]);
		}
		else
		{
			mb->modify (IDC_FILE_RECENTMODELS1 + i, IDC_FILE_RECENTMODELS1 + i, "(empty)");
			mb->setEnabled (IDC_FILE_RECENTMODELS1 + i, false);
		}
	}
}

void MDLViewer::loadRecentFiles( void )
{
	char	str[256];

	for( int i = 0; i < 8; i++ )
	{
		mx_snprintf( str, sizeof( str ), "RecentFile%i", i );
		if( !LoadString( str, recentFiles[i] ))
			break;
	}
}

void MDLViewer::saveRecentFiles( void )
{
	char	str[256];

	if( !InitRegistry( ))
		return;

	for( int i = 0; i < 8; i++ )
	{
		mx_snprintf( str, sizeof( str ), "RecentFile%i", i );
		if( !SaveString( str, recentFiles[i] ))
			break;
	}
}

MDLViewer :: MDLViewer() : mxWindow( 0, 0, 0, 0, 0, g_appTitle, mxWindow::Normal )
{
	// create menu stuff
	mb = new mxMenuBar (this);
	mxMenu *menuFile = new mxMenu ();
	mxMenu *menuOptions = new mxMenu ();
	mxMenu *menuView = new mxMenu ();
	mxMenu *menuHelp = new mxMenu ();

	mb->addMenu ("File", menuFile);
	mb->addMenu ("Options", menuOptions);
	mb->addMenu ("Tools", menuView);
	mb->addMenu ("Help", menuHelp);

	mxMenu *menuRecentModels = new mxMenu ();
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS1);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS2);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS3);
	menuRecentModels->add ("(empty)", IDC_FILE_RECENTMODELS4);

	mxMenu *menuRecentPakFiles = new mxMenu ();
	menuRecentPakFiles->add ("(empty)", IDC_FILE_RECENTPAKFILES1);
	menuRecentPakFiles->add ("(empty)", IDC_FILE_RECENTPAKFILES2);
	menuRecentPakFiles->add ("(empty)", IDC_FILE_RECENTPAKFILES3);
	menuRecentPakFiles->add ("(empty)", IDC_FILE_RECENTPAKFILES4);

	menuFile->add ("Load Model...", IDC_FILE_LOADMODEL);
	menuFile->add ("Save Model...", IDC_FILE_SAVEMODEL);
	menuFile->addSeparator ();
	menuFile->add ("Load Background Texture...", IDC_FILE_LOADBACKGROUNDTEX);
	menuFile->add ("Load Ground Texture...", IDC_FILE_LOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->add ("Unload Ground Texture", IDC_FILE_UNLOADGROUNDTEX);
	menuFile->addSeparator ();
	menuFile->add ("Open PAK file...", IDC_FILE_OPENPAKFILE);
	menuFile->add ("Close PAK file", IDC_FILE_CLOSEPAKFILE);
	menuFile->addSeparator ();
	menuFile->addMenu ("Recent Models", menuRecentModels);
	menuFile->addMenu ("Recent PAK files", menuRecentPakFiles);
	menuFile->addSeparator ();
	menuFile->add ("Exit", IDC_FILE_EXIT);

	menuOptions->add ("Background Color...", IDC_OPTIONS_COLORBACKGROUND);
	menuOptions->add ("Ground Color...", IDC_OPTIONS_COLORGROUND);
	menuOptions->add ("Light Color...", IDC_OPTIONS_COLORLIGHT);
	menuOptions->addSeparator ();
	menuOptions->add( "Sequence AutoPlay", IDC_OPTIONS_AUTOPLAY );
	menuOptions->addSeparator ();
	menuOptions->add( "Use weapon origin", IDC_OPTIONS_WEAPONORIGIN );
	menuOptions->add( "Weapon left-handed", IDC_OPTIONS_LEFTHAND );
	menuOptions->addSeparator ();
	menuOptions->add ("Center View", IDC_OPTIONS_CENTERVIEW);
	menuOptions->add ("Reset View", IDC_OPTIONS_RESETVIEW);
#ifdef WIN32
	menuOptions->addSeparator ();
	menuOptions->add ("Make Screenshot...", IDC_OPTIONS_MAKESCREENSHOT);
	//menuOptions->add ("Dump Model Info", IDC_OPTIONS_DUMP);
#endif
	menuView->add ("File Associations...", IDC_VIEW_FILEASSOCIATIONS);

#ifdef WIN32
	menuHelp->add ("Goto Homepage...", IDC_HELP_GOTOHOMEPAGE);
	menuHelp->addSeparator ();
#endif
	menuHelp->add ("About", IDC_HELP_ABOUT);

	mb->setChecked( IDC_OPTIONS_WEAPONORIGIN, bUseWeaponOrigin );
	mb->setChecked( IDC_OPTIONS_LEFTHAND, bUseWeaponLeftHand );
	mb->setChecked( IDC_OPTIONS_AUTOPLAY, g_viewerSettings.sequence_autoplay ? true : false );

	// create tabcontrol with subdialog windows
	tab = new mxTab (this, 0, 0, 0, 0, IDC_TAB);
#ifdef WIN32
	SetWindowLong ((HWND) tab->getHandle (), GWL_EXSTYLE, WS_EX_CLIENTEDGE);
#endif
	mxWindow *wRender = new mxWindow (this, 0, 0, 0, 0);
	tab->add (wRender, "Model Display");
	mxLabel *RenderLabel = new mxLabel (wRender, 5, 3, 120, 20, "Render Mode");
	cRenderMode = new mxChoice (wRender, 5, 17, 112, 22, IDC_RENDERMODE);
	cRenderMode->add ("Wireframe");
	cRenderMode->add ("Flat Shaded");
	cRenderMode->add ("Smooth Shaded");
	cRenderMode->add ("Texture Shaded");
	cRenderMode->select (3);
	mxToolTip::add (cRenderMode, "Select Render Mode");
	lOpacityValue = new mxLabel (wRender, 5, 45, 100, 18, "Opacity: 100%");
	slTransparency = new mxSlider (wRender, 0, 62, 120, 18, IDC_TRANSPARENCY);
	slTransparency->setValue (100);
	mxToolTip::add (slTransparency, "Model Transparency");
	mxCheckBox *cbDrawLuma = new mxCheckBox (wRender, 140, 5, 120, 20, "Draw Luma", IDC_FULLBRIGHTS);
	mxCheckBox *cbAdjustOrigin = new mxCheckBox (wRender, 140, 25, 120, 20, "Adjust Z", IDC_ADJUST_ORIGIN);
	mxCheckBox *cbEyePosition = new mxCheckBox (wRender, 140, 45, 120, 20, "Show Eye Position", IDC_EYEPOSITION );
	mxCheckBox *cbNormals = new mxCheckBox (wRender, 140, 65, 120, 20, "Show Normals", IDC_NORMALS);
	g_viewerSettings.renderLuma = true;
	cbDrawLuma->setChecked( true );

	cbGround = new mxCheckBox (wRender, 260, 5, 130, 20, "Show Ground", IDC_GROUND);
	cbGround->setChecked( g_viewerSettings.showGround ? true : false );

	cbMirror = new mxCheckBox (wRender, 260, 25, 130, 20, "Mirror Model On Ground", IDC_MIRROR);
	cbBackground = new mxCheckBox (wRender, 260, 45, 130, 20, "Show Background", IDC_BACKGROUND);
	mxCheckBox *cbWireframe = new mxCheckBox (wRender, 260, 65, 130, 20, "Wireframe Overlay", IDC_WIREFRAME);

	cSkin = new mxChoice (wRender, 430, 5, 112, 22, IDC_SKINS);
	mxToolTip::add (cSkin, "Choose a skin family");
	lModelInfo1 = new mxLabel (wRender, 430, 30, 120, 100, "" );

	mxWindow *wTexture = new mxWindow (this, 0, 0, 0, 0);
	tab->add (wTexture, "Textures");
	cTextures = new mxChoice (wTexture, 5, 18, 150, 22, IDC_TEXTURES);
	mxToolTip::add (cTextures, "Choose a texture");
	cTexAnim = new mxChoice (wTexture, 175, 18, 150, 22, IDC_TEXANIM);
	mxToolTip::add (cTexAnim, "Choose a animtaion frame");
	cTexAnim->setEnabled (false);

	new mxButton (wTexture, 510, 5, 80, 18, "Import Texture", IDC_IMPORTTEXTURE);
	new mxButton (wTexture, 510, 25, 80, 18, "Export Texture", IDC_EXPORTTEXTURE);
	new mxButton (wTexture, 510, 45, 80, 18, "Export UV Map", IDC_EXPORT_UVMAP);
	lTexSize = new mxLabel (wTexture, 5, 3, 140, 14, "Texture");
	cbShowUVMap = new mxCheckBox (wTexture, 400, 3, 100, 22, "Show UV Map", IDC_SHOW_UV_MAP);
	cbOverlayUVMap = new mxCheckBox (wTexture, 400, 23, 100, 22, "Overlay UV Map", IDC_OVERLAY_UV_MAP);
	cbAntiAliasLines = new mxCheckBox (wTexture, 400, 43, 100, 22, "Anti-Alias Lines", IDC_ANTI_ALIAS_LINES);

	mxToolTip::add (new mxSlider (wTexture, 0, 60, 160, 18, IDC_TEXTURESCALE), "Scale texture size");
	lTexScale = new mxLabel (wTexture, 5, 47, 140, 14, "Scale Texture View (1x)");

	mxWindow *wAnim = new mxWindow (this, 0, 0, 0, 0);

	// and add them to the tabcontrol
	tab->add (wAnim, "Animation");

	// Create widgets for the Animation Tab

	mxLabel *AnimSequence = new mxLabel (wAnim, 5, 3, 120, 18, "Animation Sequence");
	cAnim = new mxChoice (wAnim, 5, 18, 200, 22, IDC_ANIMATION );	
	mxToolTip::add (cAnim, "Select Animation");
	tbStop = new mxButton (wAnim, 5, 46, 60, 18, "Stop", IDC_STOP);
	mxToolTip::add (tbStop, "Stop Playing");
	bPrevFrame = new mxButton (wAnim, 84, 46, 30, 18, "<<", IDC_PREVFRAME);
	bPrevFrame->setEnabled (false);
	mxToolTip::add (bPrevFrame, "Prev Frame");
	leFrame = new mxLineEdit (wAnim, 119, 46, 50, 18, "", IDC_FRAME); 
	leFrame->setEnabled (false);
	mxToolTip::add (leFrame, "Set Frame");
	bNextFrame = new mxButton (wAnim, 174, 46, 30, 18, ">>", IDC_NEXTFRAME);
	bNextFrame->setEnabled (false);
	mxToolTip::add (bNextFrame, "Next Frame");	

	lSequenceInfo = new mxLabel (wAnim, 228, 12, 90, 100, "");

	mxLabel *SpdLabel = new mxLabel (wAnim, 170, 70, 35, 18, "Speed");
	slSpeedScale = new mxSlider (wAnim, 0, 70, 165, 18, IDC_SPEEDSCALE);
	slSpeedScale->setRange (0, 200);
	slSpeedScale->setValue (40);
	mxToolTip::add (slSpeedScale, "Speed Scale");

	mxWindow *wMisc = new mxWindow (this, 0, 0, 0, 0);
	tab->add (wMisc, "Misc");

	mxLabel *FlagLabel = new mxLabel (wMisc, 5, 3, 120, 20, "Global model flags");
	cbFlagRocket = new mxCheckBox (wMisc, 10, 23, 95, 22, "Rocket Trail", IDC_ALIAS_ROCKET );
	mxToolTip::add (cbFlagRocket, "leave red-orange particle trail + dynamic light at model origin");
	cbFlagGrenade = new mxCheckBox (wMisc, 10, 43, 95, 22, "Grenade Smoke", IDC_ALIAS_GRENADE );
	mxToolTip::add (cbFlagGrenade, "leave gray-black particle trail");
	cbFlagGib = new mxCheckBox (wMisc, 10, 63, 95, 22, "Gib Blood", IDC_ALIAS_GIB );
	mxToolTip::add (cbFlagGib, "leave dark red particle trail that obey gravity");

	cbFlagRotate = new mxCheckBox (wMisc, 110, 3, 95, 22, "Model Rotate", IDC_ALIAS_ROTATE );
	mxToolTip::add (cbFlagRotate, "model will auto-rotate by yaw axis. Useable for items (will be working only in Xash3D)");
	cbFlagTracer = new mxCheckBox (wMisc, 110, 23, 95, 22, "Green Trail", IDC_ALIAS_TRACER );
	mxToolTip::add (cbFlagTracer, "green split trail. e.g. monster_wizard from Quake");
	cbFlagZomgib = new mxCheckBox (wMisc, 110, 43, 95, 22, "Zombie Blood", IDC_ALIAS_ZOMGIB );
	mxToolTip::add (cbFlagZomgib, "small blood trail from zombie gibs");
	cbFlagTracer2 = new mxCheckBox (wMisc, 110, 63, 95, 22, "Orange Trail", IDC_ALIAS_TRACER2 );
	mxToolTip::add (cbFlagTracer2, "orange split trail + rotate");
	cbFlagTracer3 = new mxCheckBox (wMisc, 210, 3, 95, 22, "Purple Trail", IDC_ALIAS_TRACER3 );
	mxToolTip::add (cbFlagTracer3, "purple signle trail");

	mxLabel *RemapLabel = new mxLabel (wMisc, 325, 3, 120, 20, "Remap colors");
	slTopColor = new mxSlider( wMisc, 320, 20, 145, 18, IDC_TOPCOLOR );
	slBottomColor = new mxSlider( wMisc, 320, 40, 145, 18, IDC_BOTTOMCOLOR );
	new mxLabel( wMisc, 467, 20, 55, 18, "Top Color" );
	new mxLabel( wMisc, 467, 40, 75, 18, "Bottom Color" );
	slTopColor->setRange( 0, 13 );	// 16 * 13 + 16 = 224 (luma pixels) 
	slBottomColor->setRange( 0, 13 );
	slTopColor->setValue( g_viewerSettings.topcolor );
	slBottomColor->setValue( g_viewerSettings.bottomcolor );

	mxWindow *wEdit = new mxWindow (this, 0, 0, 0, 0);
	tab->add (wEdit, "Editor");

	mxLabel *EidtLabel1 = new mxLabel (wEdit, 5, 3, 120, 20, "Edit Type");
	cEditType = new mxChoice (wEdit, 5, 17, 112, 22, IDC_EDIT_TYPE);
	mxToolTip::add (cEditType, "Select item to editing");

	mxLabel *EidtLabel2 = new mxLabel (wEdit, 5, 42, 120, 20, "Edit Mode");
	cEditMode = new mxChoice (wEdit, 5, 58, 112, 22, IDC_EDIT_MODE);
	mxToolTip::add (cEditMode, "Select editor mode (change current model or grab values to put them into QC source)");
	cEditMode->add ("QC Source");
	cEditMode->add ("Real Model");
	cEditMode->select (g_viewerSettings.editMode);

	mxLabel *EidtLabel3 = new mxLabel (wEdit, 125, 3, 120, 20, "Step Size");
	leEditStep = new mxLineEdit (wEdit, 125, 19, 50, 18, va( "%g", g_viewerSettings.editStep ));
	mxToolTip::add (leEditStep, "Editor movement step size");

	mxLabel *EidtLabel4 = new mxLabel (wEdit, 185, 3, 170, 20, "QC source code:");
	leEditString = new mxLineEdit (wEdit, 185, 19, 290, 18, "" );

	if( g_viewerSettings.editMode == EDIT_SOURCE )
		leEditString->setEnabled( true );
	else leEditString->setEnabled( false );

	tbMovePosX = new mxButton (wEdit, 185, 43, 55, 20, "Move +X", IDC_MOVE_PX );
	tbMoveNegX = new mxButton (wEdit, 185, 64, 55, 20, "Move -X", IDC_MOVE_NX );
	tbMovePosY = new mxButton (wEdit, 245, 43, 55, 20, "Move +Y", IDC_MOVE_PY );
	tbMoveNegY = new mxButton (wEdit, 245, 64, 55, 20, "Move -Y", IDC_MOVE_NY );
	tbMovePosZ = new mxButton (wEdit, 305, 43, 55, 20, "Move +Z", IDC_MOVE_PZ );
	tbMoveNegZ = new mxButton (wEdit, 305, 64, 55, 20, "Move -Z", IDC_MOVE_NZ );

	// create the OpenGL window
	d_GlWindow = new GlWindow (this, 0, 0, 0, 0, "", mxWindow::Normal);
#ifdef WIN32
	SetWindowLong ((HWND) d_GlWindow->getHandle (), GWL_EXSTYLE, WS_EX_CLIENTEDGE);
#endif
	g_GlWindow = d_GlWindow;

	// finally create the pakviewer window
	pakViewer = new PAKViewer (this);

	g_FileAssociation = new FileAssociation ();

	loadRecentFiles ();
	initRecentFiles ();

	setBounds (20, 20, 690, 550);
	setVisible (true);

	if( g_viewerSettings.showGround )
		setShowGround (true);

	if( g_viewerSettings.groundTexFile[0] )
		d_GlWindow->loadTexture( g_viewerSettings.groundTexFile, TEXTURE_GROUND );
	else d_GlWindow->loadTexture( NULL, TEXTURE_GROUND );
}

MDLViewer::~MDLViewer ()
{
	g_viewerSettings.showMaximized = isMaximized();
	saveRecentFiles ();
	SaveViewerSettings ();
}

void MDLViewer :: checkboxSet( int id, bool bState )
{
	mb->setChecked( id, bState );
}

int
MDLViewer::handleEvent (mxEvent *event)
{
	if (event->event == mxEvent::Size)
	{
		int w = event->width;
		int h = event->height;
		int y = mb->getHeight ();
#ifdef WIN32
#define HEIGHT 120
#else
#define HEIGHT 140
		h -= 40;
#endif

		if (pakViewer->isVisible ())
		{
			w -= 170;
			pakViewer->setBounds (w, y, 170, h);
		}

		d_GlWindow->setBounds (0, y, w, h - HEIGHT);
		tab->setBounds (0, y + h - HEIGHT, w, HEIGHT);
		return 1;
	}

	if ( event->event == mxEvent::KeyDown )
	{
		redraw();
		switch( (char)event->key )
		{
		case 27:
			if( !getParent( )) // fullscreen mode ?
				mx::quit();
			break;
		case 37:
			if( g_viewerSettings.numModelPathes > 0 )
				loadModel( LoadPrevModel( ));
			break;
		case 39:
			if( g_viewerSettings.numModelPathes > 0 )
				loadModel( LoadNextModel( ));
			break;
		case VK_F5:
		{
			bool oldUseWeaponOrigin = bUseWeaponOrigin;
			loadModel( g_viewerSettings.modelFile, false );
			bUseWeaponOrigin = oldUseWeaponOrigin;
			break;
		}
		case 'v':
		case 'ì':
			bUseWeaponOrigin = !mb->isChecked( IDC_OPTIONS_WEAPONORIGIN );
			mb->setChecked( IDC_OPTIONS_WEAPONORIGIN, bUseWeaponOrigin );
			break;
		case 'l':
		case 'ä':
			bUseWeaponLeftHand = !mb->isChecked( IDC_OPTIONS_LEFTHAND );
			mb->setChecked( IDC_OPTIONS_LEFTHAND, bUseWeaponLeftHand );
			break;
		case '5':
			g_viewerSettings.transparency -= 0.05f;
			if( g_viewerSettings.transparency < 0.0f )
				g_viewerSettings.transparency = 0.0f;
			break;
		case '6':
			g_viewerSettings.transparency += 0.05f;
			if( g_viewerSettings.transparency > 1.0f )
				g_viewerSettings.transparency = 1.0f;
			break;
		}
		return 1;
	}

	switch (event->action)
	{
		case IDC_TAB:
		{
			g_viewerSettings.showTexture = (tab->getSelectedIndex () == TAB_TEXTURES);
		}
		break;

		case IDC_FILE_LOADMODEL:
		{
			const char *ptr = mxGetOpenFileName (this, 0, "*.mdl");
			if (ptr)
			{
				if (!loadModel (ptr ))
				{
					char str[256];

					sprintf (str, "Error reading model: %s", ptr);
					mxMessageBox (this, str, "ERROR", MX_MB_OK | MX_MB_ERROR);
					break;
				}

				// now update recent files list

				int i;
				char path[256];

				if (event->action == IDC_FILE_LOADMODEL)
					strcpy (path, "[m] ");
				else
					strcpy (path, "[w] ");

				strcat (path, ptr);

				for (i = 0; i < 4; i++)
				{
					if (!mx_strcasecmp (recentFiles[i], path))
						break;
				}

				// swap existing recent file
				if (i < 4)
				{
					char tmp[256];
					strcpy (tmp, recentFiles[0]);
					strcpy (recentFiles[0], recentFiles[i]);
					strcpy (recentFiles[i], tmp);
				}

				// insert recent file
				else
				{
					for (i = 3; i > 0; i--)
						strcpy (recentFiles[i], recentFiles[i - 1]);

					strcpy (recentFiles[0], path);
				}

				initRecentFiles ();
			}
		}
		break;
		case IDC_FILE_SAVEMODEL:
		{
			char *ptr = (char *) mxGetSaveFileName (this, g_viewerSettings.modelPath, "*.mdl", g_viewerSettings.modelPath);
			if (!ptr)
				break;

			char filename[256];
			char ext[16];

			strcpy( filename, ptr );
			strcpy( ext, mx_getextension( filename ));
			if( mx_strcasecmp( ext, ".mdl" ))
				strcat( filename, ".mdl" );

			if( !g_aliasModel.SaveModel( filename ))
			{
				mxMessageBox( this, "Error saving model.", g_appTitle, MX_MB_OK | MX_MB_ERROR );
			}
			else
			{
				strcpy( g_viewerSettings.modelFile, filename );
				g_viewerSettings.numModelChanges = 0;	// all the settings are handled
			}
		}
		break;
		case IDC_FILE_LOADBACKGROUNDTEX:
		case IDC_FILE_LOADGROUNDTEX:
		{
			const char *ptr = mxGetOpenFileName (this, 0, "*.bmp;*.tga;*.pcx");
			if (ptr)
			{
				int name = TEXTURE_UNUSED;

				if( event->action == IDC_FILE_LOADBACKGROUNDTEX )
					name = TEXTURE_BACKGROUND;
				else if( event->action == IDC_FILE_LOADGROUNDTEX )					
					name = TEXTURE_GROUND;

				if (d_GlWindow->loadTexture( ptr, name ))
				{
					if (event->action == IDC_FILE_LOADBACKGROUNDTEX)
						setShowBackground (true);
					else
						setShowGround (true);

				}
				else
					mxMessageBox (this, "Error loading texture.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
			}
		}
		break;

		case IDC_FILE_UNLOADGROUNDTEX:
		{
			d_GlWindow->loadTexture( NULL, TEXTURE_GROUND );
			setShowGround (false);
		}
		break;
		case IDC_FILE_OPENPAKFILE:
		{
			const char *ptr = mxGetOpenFileName (this, "\\Quake\\id1\\", "*.pak");
			if (ptr)
			{
				int i;

				pakViewer->openPAKFile (ptr);

				for (i = 4; i < 8; i++)
				{
					if (!mx_strcasecmp (recentFiles[i], ptr))
						break;
				}

				// swap existing recent file
				if (i < 8)
				{
					char tmp[256];
					strcpy (tmp, recentFiles[4]);
					strcpy (recentFiles[4], recentFiles[i]);
					strcpy (recentFiles[i], tmp);
				}

				// insert recent file
				else
				{
					for (i = 7; i > 4; i--)
						strcpy (recentFiles[i], recentFiles[i - 1]);

					strcpy (recentFiles[4], ptr);
				}

				initRecentFiles ();

				redraw ();
			}
		}
		break;

		case IDC_FILE_CLOSEPAKFILE:
		{
			pakViewer->closePAKFile ();
			redraw ();
		}
		break;

		case IDC_FILE_RECENTMODELS1:
		case IDC_FILE_RECENTMODELS2:
		case IDC_FILE_RECENTMODELS3:
		case IDC_FILE_RECENTMODELS4:
		{
			int i = event->action - IDC_FILE_RECENTMODELS1;
			bool isModel = recentFiles[i][1] == 'm';
			char *ptr = &recentFiles[i][4];

			if (!loadModel (ptr ))
			{
				char str[256];

				sprintf (str, "Error reading model: %s", ptr);
				mxMessageBox (this, str, "ERROR", MX_MB_OK | MX_MB_ERROR);
				break;
			}

			// update recent model list

			char tmp[256];			
			strcpy (tmp, recentFiles[0]);
			strcpy (recentFiles[0], recentFiles[i]);
			strcpy (recentFiles[i], tmp);

			initRecentFiles ();
		}
		break;

		case IDC_FILE_RECENTPAKFILES1:
		case IDC_FILE_RECENTPAKFILES2:
		case IDC_FILE_RECENTPAKFILES3:
		case IDC_FILE_RECENTPAKFILES4:
		{
			int i = event->action - IDC_FILE_RECENTPAKFILES1 + 4;
			pakViewer->openPAKFile (recentFiles[i]);

			char tmp[256];			
			strcpy (tmp, recentFiles[4]);
			strcpy (recentFiles[4], recentFiles[i]);
			strcpy (recentFiles[i], tmp);

			initRecentFiles ();

			redraw ();
		}
		break;

		case IDC_FILE_EXIT:
			pakViewer->closePAKFile ();
			redraw ();
			mx::quit ();
			break;

		case IDC_OPTIONS_COLORBACKGROUND:
		case IDC_OPTIONS_COLORGROUND:
		case IDC_OPTIONS_COLORLIGHT:
		{
			float *cols[3] = { g_viewerSettings.bgColor, g_viewerSettings.gColor, g_viewerSettings.lColor };
			float *col = cols[event->action - IDC_OPTIONS_COLORBACKGROUND];
			int r = (int) (col[0] * 255.0f);
			int g = (int) (col[1] * 255.0f);
			int b = (int) (col[2] * 255.0f);
			if (mxChooseColor (this, &r, &g, &b))
			{
				col[0] = (float) r / 255.0f;
				col[1] = (float) g / 255.0f;
				col[2] = (float) b / 255.0f;
			}
		}
		break;
#ifdef WIN32
		case IDC_OPTIONS_MAKESCREENSHOT:
		{
			char *ptr = (char *)mxGetSaveFileName (this, 0, "*.bmp");
			if (ptr)
			{
				if( !strstr( ptr, ".bmp" ))
					strcat( ptr, ".bmp" );
				makeScreenShot (ptr);
			}
		}
		break;
#endif
		case IDC_OPTIONS_WEAPONORIGIN:
			bUseWeaponOrigin = !mb->isChecked( IDC_OPTIONS_WEAPONORIGIN );
			mb->setChecked( IDC_OPTIONS_WEAPONORIGIN, bUseWeaponOrigin );
			break;

		case IDC_OPTIONS_LEFTHAND:
			bUseWeaponLeftHand = !mb->isChecked( IDC_OPTIONS_LEFTHAND );
			mb->setChecked( IDC_OPTIONS_LEFTHAND, bUseWeaponLeftHand );
			break;

		case IDC_OPTIONS_AUTOPLAY:
			g_viewerSettings.sequence_autoplay = !mb->isChecked( IDC_OPTIONS_AUTOPLAY );
			mb->setChecked( IDC_OPTIONS_AUTOPLAY, g_viewerSettings.sequence_autoplay ? true : false );
			break;

		case IDC_OPTIONS_CENTERVIEW:
		{
			centerModel ();
			d_GlWindow->redraw ();
		}
		break;
		case IDC_VIEW_FILEASSOCIATIONS:
			g_FileAssociation->setAssociation (0);
			g_FileAssociation->setVisible (true);
			break;

#ifdef WIN32
		case IDC_HELP_GOTOHOMEPAGE:
			ShellExecute (0, "open", "http://cs-mapping.com.ua/forum/forumdisplay.php?f=189", 0, 0, SW_SHOW);
			break;
#endif

		case IDC_HELP_ABOUT:
			mxMessageBox (this,
				"Quake 1 Model Viewer v0.60 (c) 2018 by Unkle Mike\n"
				"Based on original MD2V code by Mete Ciragan\n\n"
				"Left-drag to rotate.\n"
				"Right-drag to zoom.\n"
				"Shift-left-drag to x-y-pan.\n"
				"Ctrl-drag to move lights.\n\n"
				"Build:\t" __DATE__ ".\n"
				"Email:\tg-cont@rambler.ru\n"
				"Web:\thttp://www.hlfx.ru/forum\n\thttp://cs-mapping.com.ua/forum", "About Quake 1 Model Viewer",
				MX_MB_OK | MX_MB_INFORMATION );
			break;

		// 
		// widget actions
		//
		//

		//
		// Model Panel
		//

		case IDC_RENDERMODE:
		{
			int index = cRenderMode->getSelectedIndex ();
			if (index >= 0)
			{
				setRenderMode (index);
			}
		}
		break;

		case IDC_TRANSPARENCY:
		{
			int value = slTransparency->getValue ();
			g_viewerSettings.transparency = (float) value / 100.0f; 
			lOpacityValue->setLabel( "Opacity: %d%%", value ); 
		}
		break;

		case IDC_GROUND:
			setShowGround (((mxCheckBox *) event->widget)->isChecked ());
			break;

		case IDC_MIRROR:
			setMirror (((mxCheckBox *) event->widget)->isChecked ());
			break;

		case IDC_BACKGROUND:
			setShowBackground (((mxCheckBox *) event->widget)->isChecked ());
			break;

		case IDC_EYEPOSITION:
			g_viewerSettings.showEyePosition = ((mxCheckBox *) event->widget)->isChecked ();
			break;
		case IDC_FULLBRIGHTS:
			g_viewerSettings.renderLuma = ((mxCheckBox *) event->widget)->isChecked ();
			break;
		case IDC_ADJUST_ORIGIN:
			g_viewerSettings.adjustOrigin = ((mxCheckBox *) event->widget)->isChecked ();
			break;
		case IDC_NORMALS:
			g_viewerSettings.showNormals = ((mxCheckBox *) event->widget)->isChecked ();
			break;

		case IDC_WIREFRAME:
			g_viewerSettings.showWireframeOverlay = ((mxCheckBox *) event->widget)->isChecked ();
			break;

		case IDC_SKINS:
		{
			int index = cSkin->getSelectedIndex ();
			if (index >= 0)
			{
				g_aliasModel.SetSkin (index);
				g_viewerSettings.skin = index;
				d_GlWindow->redraw ();
			}
		}
		break;

		case IDC_TEXTURES:
		{
			int index = cTextures->getSelectedIndex ();
			if (index >= 0)
			{
				g_viewerSettings.texture = index;
				daliashdr_t *hdr = g_aliasModel.getAliasHeader ();
				if (hdr) lTexSize->setLabel ("Texture (size: %d x %d)", hdr->skinwidth, hdr->skinheight);
				d_GlWindow->redraw ();
				initTexAnim( index );
			}

			cbShowUVMap->setChecked (g_viewerSettings.show_uv_map);
			cbOverlayUVMap->setChecked (g_viewerSettings.overlay_uv_map);
			cbAntiAliasLines->setChecked (g_viewerSettings.anti_alias_lines);
		}
		break;

		case IDC_TEXANIM:
		{
			int index = cTexAnim->getSelectedIndex ();
			if (index >= 0)
			{
				g_viewerSettings.skinAnim = index;
			}
		}
		break;

		case IDC_SHOW_UV_MAP:
		{
			if (cbShowUVMap->isChecked ())
				g_viewerSettings.show_uv_map = true;
			else g_viewerSettings.show_uv_map = false;
		}
		break;

		case IDC_OVERLAY_UV_MAP:
		{
			if (cbOverlayUVMap->isChecked ())
				g_viewerSettings.overlay_uv_map = true;
			else g_viewerSettings.overlay_uv_map = false;
		}
		break;

		case IDC_ANTI_ALIAS_LINES:
		{
			if (cbAntiAliasLines->isChecked ())
				g_viewerSettings.anti_alias_lines = true;
			else g_viewerSettings.anti_alias_lines = false;
		}
		break;

		case IDC_EXPORTTEXTURE:
		{
			char *ptr = (char *) mxGetSaveFileName (this, "", "*.bmp");
			if (!ptr)
				break;

			char filename[256];
			char ext[16];

			strcpy (filename, ptr);
			strcpy (ext, mx_getextension (filename));
			if (mx_strcasecmp (ext, ".bmp"))
				strcat (filename, ".bmp");

			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if (phdr)
			{
				mxImage image;
				image.width = phdr->skinwidth;
				image.height = phdr->skinheight;
				image.bpp = 8;
				image.data = (void *)g_aliasModel.GetSkinData( g_viewerSettings.texture );
				image.palette = (void *)palette_q1;
				if (!mxBmpWrite (filename, &image))
					mxMessageBox (this, "Error writing .BMP texture.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
				image.data = 0;
				image.palette = 0;
			}
		}
		break;

		case IDC_IMPORTTEXTURE:
		{
			char *ptr = (char *) mxGetOpenFileName (this, "", "*.bmp");
			if (!ptr)
				break;

			char filename[256];
			char ext[16];

			strcpy (filename, ptr);
			strcpy (ext, mx_getextension (filename));
			if (mx_strcasecmp (ext, ".bmp"))
				strcat (filename, ".bmp");

			mxImage *image = mxBmpRead (filename);
			if (!image)
			{
				mxMessageBox (this, "Error loading .BMP texture.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
				return 1;
			}

			if (!image->palette)
			{
				delete image;
				mxMessageBox (this, "Error loading .BMP texture.  Must be 8-bit!", g_appTitle, MX_MB_OK | MX_MB_ERROR);
				return 1;
			}

			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if (phdr)
			{
				if (image->width == phdr->skinwidth && image->height == phdr->skinheight)
				{
					byte *dst = g_aliasModel.GetSkinData( g_viewerSettings.texture );
					memcpy (dst, image->data, image->width * image->height);
					g_aliasModel.UploadTexture (dst, phdr->skinwidth, phdr->skinheight, palette_q1, TEXTURE_COUNT + g_viewerSettings.texture );
					g_viewerSettings.numModelChanges++;
				}
				else
					mxMessageBox (this, "Texture must be of same size.", g_appTitle, MX_MB_OK | MX_MB_ERROR);
			}

			delete image;
			d_GlWindow->redraw ();
		}
		break;

		case IDC_EXPORT_UVMAP:
		{
			char *ptr = (char *) mxGetSaveFileName (this, "", "*.bmp");
			if (!ptr)
				break;

			char ext[16];

			strcpy (g_viewerSettings.uvmapPath, ptr);
			strcpy (ext, mx_getextension (g_viewerSettings.uvmapPath));
			if (mx_strcasecmp (ext, ".bmp"))
				strcat (g_viewerSettings.uvmapPath, ".bmp");
			g_viewerSettings.pending_export_uvmap = true;
		}
		break;

		case IDC_TEXTURESCALE:
		{
			g_viewerSettings.textureScale =  1.0f + (float) ((mxSlider *) event->widget)->getValue () * 4.0f / 100.0f;
			lTexScale->setLabel( "Scale Texture View (%.fx)", g_viewerSettings.textureScale );
			d_GlWindow->redraw ();
		}
		break;

		//
		// Animation Panel
		//
		case IDC_ANIMATION:
		{
			int index = cAnim->getSelectedIndex ();
			if (index >= 0)
			{
				// set the animation
				initAnimation( index );
			}
		}
		break;

		case IDC_SPEEDSCALE:
		{
			int v = ((mxSlider *)event->widget)->getValue ();
			g_viewerSettings.speedScale = (float) (v * 5) / 200.0f;
		}
		break;

		case IDC_STOP:
		{
			if( !g_bStopPlaying )
			{
				tbStop->setLabel ("Play");
				g_bStopPlaying = true;
				g_nCurrFrame = g_aliasModel.setFrame (-1, true );
				leFrame->setLabel ("%d", g_nCurrFrame);
				bPrevFrame->setEnabled (true);
				leFrame->setEnabled (true);
				bNextFrame->setEnabled (true);
			}
			else
			{
				tbStop->setLabel ("Stop");
				g_bStopPlaying = false;
				if( g_bEndOfSequence )
					g_nCurrFrame = g_aliasModel.setFrame( 0, true );
				bPrevFrame->setEnabled (false);
				leFrame->setEnabled (false);
				bNextFrame->setEnabled (false);
				g_bEndOfSequence = false;
			}
		}
		break;

		case IDC_PREVFRAME:
		{
			g_nCurrFrame = g_aliasModel.setFrame (g_nCurrFrame - 1, true );
			leFrame->setLabel ("%d", g_nCurrFrame);
			g_bEndOfSequence = false;
		}
		break;

		case IDC_FRAME:
		{
			g_nCurrFrame = atoi (leFrame->getLabel ());
			g_nCurrFrame = g_aliasModel.setFrame (g_nCurrFrame, true );
			g_bEndOfSequence = false;
		}
		break;

		case IDC_NEXTFRAME:
		{
			g_nCurrFrame = g_aliasModel.setFrame (g_nCurrFrame + 1, true );
			leFrame->setLabel ("%d", g_nCurrFrame);
			g_bEndOfSequence = false;
		}
		break;
		case IDC_MISC:
		{
			slTopColor->setValue( g_viewerSettings.topcolor );
			slBottomColor->setValue( g_viewerSettings.bottomcolor );
		}
		break;

		case IDC_ALIAS_ROCKET:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagRocket->isChecked ())
					phdr->flags |= ALIAS_ROCKET;
				else phdr->flags &= ~ALIAS_ROCKET;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_GRENADE:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagGrenade->isChecked ())
					phdr->flags |= ALIAS_GRENADE;
				else phdr->flags &= ~ALIAS_GRENADE;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_GIB:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagGib->isChecked ())
					phdr->flags |= ALIAS_GIB;
				else phdr->flags &= ~ALIAS_GIB;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_ROTATE:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagRotate->isChecked ())
					phdr->flags |= ALIAS_ROTATE;
				else phdr->flags &= ~ALIAS_ROTATE;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_TRACER:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagTracer->isChecked ())
					phdr->flags |= ALIAS_TRACER;
				else phdr->flags &= ~ALIAS_TRACER;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_ZOMGIB:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagZomgib->isChecked ())
					phdr->flags |= ALIAS_ZOMGIB;
				else phdr->flags &= ~ALIAS_ZOMGIB;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_TRACER2:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagTracer2->isChecked ())
					phdr->flags |= ALIAS_TRACER2;
				else phdr->flags &= ~ALIAS_TRACER2;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_ALIAS_TRACER3:
		{
			daliashdr_t *phdr = g_aliasModel.getAliasHeader ();
			if( phdr )
			{
				if (cbFlagTracer3->isChecked ())
					phdr->flags |= ALIAS_TRACER3;
				else phdr->flags &= ~ALIAS_TRACER3;
				g_viewerSettings.numModelChanges++;
			}
		}

		case IDC_TOPCOLOR:
		{
			int v = ((mxSlider *)event->widget)->getValue();
			g_aliasModel.SetTopColor( v );
		}
		break;

		case IDC_BOTTOMCOLOR:
		{
			int v = ((mxSlider *)event->widget)->getValue();
			g_aliasModel.SetBottomColor( v );
		}
		break;

		case IDC_EDIT_MODE:
		{
			int index = cEditMode->getSelectedIndex ();
			if (index >= 0)
			{
				if( !g_aliasModel.SetEditMode (index))
				{
					// edit mode is cancelled
					cEditMode->select (g_viewerSettings.editMode);
				}
				else
				{
					if( g_viewerSettings.editMode == EDIT_SOURCE )
						leEditString->setEnabled( true );
					else leEditString->setEnabled( false );
					leEditString->setLabel( g_aliasModel.getQCcode( ));
				}
			}
		}
		break;

		case IDC_EDIT_TYPE:
		{
			int index = cEditType->getSelectedIndex ();
			if (index >= 0)
			{
				bool editSize = g_aliasModel.SetEditType (index);
				leEditString->setLabel( g_aliasModel.getQCcode( ));
			}
		}
		break;
		case IDC_MOVE_PX:
		case IDC_MOVE_NX:
		case IDC_MOVE_PY:
		case IDC_MOVE_NY:
		case IDC_MOVE_PZ:
		case IDC_MOVE_NZ:
		{
			g_viewerSettings.editStep = (float)atof (leEditStep->getLabel ());
			g_aliasModel.editPosition( g_viewerSettings.editStep, event->action );
			leEditString->setLabel( g_aliasModel.getQCcode( ));
		}
		break;
	}

	return 1;
}


void
MDLViewer::redraw ()
{
	mxEvent event;
	event.event = mxEvent::Size;
	event.width = w2 ();
	event.height = h2 ();
	handleEvent (&event);
}



void MDLViewer :: makeScreenShot( const char *filename )
{
#ifdef WIN32
	d_GlWindow->redraw ();
	int w = d_GlWindow->w2 ();
	int h = d_GlWindow->h2 ();

	mxImage *image = new mxImage ();
	if( image->create( w, h, 24 ))
	{
		glReadBuffer( GL_FRONT );
		glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image->data );

		image->flip_vertical();

		if( !mxBmpWrite( filename, image ))
			mxMessageBox( this, "Error writing screenshot.", g_appTitle, MX_MB_OK|MX_MB_ERROR );

		delete image;
	}
#endif
}

int MDLViewer::getTableIndex()
{
	return tab->getSelectedIndex ();
}

void MDLViewer :: setRenderMode( int mode )
{
	if (mode >= 0)
	{
		cRenderMode->select (mode);
		d_GlWindow->setRenderMode (mode);
		g_viewerSettings.renderMode = mode;
	}
}

void MDLViewer :: setShowGround( bool b )
{
	g_viewerSettings.showGround = b;
	cbGround->setChecked (b);
	if (!b)
	{
		cbMirror->setChecked (b);
		g_viewerSettings.mirror = b;
	}
}



void MDLViewer :: setMirror( bool b )
{
	g_viewerSettings.useStencil = b;
	g_viewerSettings.mirror = b;
	cbMirror->setChecked (b);

	if (b)
	{
		cbGround->setChecked (b);
		g_viewerSettings.showGround = b;
	}
}

void MDLViewer :: setShowBackground( bool b )
{
	g_viewerSettings.showBackground = b;
	cbBackground->setChecked (b);
}

void MDLViewer::centerModel ()
{
	g_aliasModel.centerView( false );
}

bool MDLViewer::loadModel( const char *ptr, bool centering )
{
	if( !d_GlWindow->loadModel( ptr, centering ))
		return false;

	char path[256];
	strcpy (path, mx_getpath (ptr));
	mx_setcwd (path);

	initAnimation( 0 );
	setModelInfo();
	setRenderMode( 3 );
	initTextures();
	centerModel();					
	d_GlWindow->redraw ();

	return true;
}

void MDLViewer::addEditType ( const char *name, int type, int id )
{
	if( g_aliasModel.AddEditField( type, id ))
		cEditType->add (name);
}

void
MDLViewer::setModelInfo (void)
{
	static char str[1024];

	daliashdr_t *phdr = g_aliasModel.getAliasHeader();

	if (phdr)
	{
		cbFlagRocket->setChecked ((phdr->flags & ALIAS_ROCKET) == ALIAS_ROCKET);
		cbFlagGrenade->setChecked ((phdr->flags & ALIAS_GRENADE) == ALIAS_GRENADE);
		cbFlagGib->setChecked ((phdr->flags & ALIAS_GIB) == ALIAS_GIB);
		cbFlagRotate->setChecked ((phdr->flags & ALIAS_ROTATE) == ALIAS_ROTATE);
		cbFlagTracer->setChecked ((phdr->flags & ALIAS_TRACER) == ALIAS_TRACER);
		cbFlagZomgib->setChecked ((phdr->flags & ALIAS_ZOMGIB) == ALIAS_ZOMGIB);
		cbFlagTracer2->setChecked ((phdr->flags & ALIAS_TRACER2) == ALIAS_TRACER2);
		cbFlagTracer3->setChecked ((phdr->flags & ALIAS_TRACER3) == ALIAS_TRACER3);

		cEditType->removeAll ();
		addEditType ("Model origin", TYPE_ORIGIN );
		addEditType ("Eye Position", TYPE_EYEPOSITION);
		g_aliasModel.SetEditType (0);
		cEditType->select (0);

		leEditString->setLabel( g_aliasModel.getQCcode( ));

		sprintf( str,
			"Vertices: %d\n"
			"Triangles: %d\n"
			"Skins: %d\n"
			"Frames: %d\n",
			phdr->numverts,
			phdr->numtris,
			phdr->numskins,
			phdr->numframes
			);
	}
	else
	{
		strcpy (str, "No Model.");
	}

	lModelInfo1->setLabel (str);
}

void MDLViewer :: initAnimation( int animation )
{
	cAnim->removeAll ();

	daliashdr_t *phdr = g_aliasModel.getAliasHeader();

	if (!phdr)
		return;

	int count = g_aliasModel.getAnimationCount();

	for (int i = 0; i < count; i++)
		cAnim->add (g_aliasModel.getAnimationName(i));

	int startFrame, endFrame;
	g_aliasModel.getAnimationFrames( animation, &startFrame, &endFrame );
	d_GlWindow->setFrameInfo( startFrame, endFrame );
//	g_aliasModel.setFrame( startFrame );
	cAnim->select( animation );

	// compute real framecount indclude poses
	int frameCount = (endFrame - startFrame) + 1;
	if( frameCount == 1 )
		frameCount = g_aliasModel.GetNumPoses( startFrame );

	char str[128];

	sprintf (str,
		"Sequence#: %d\n"
		"Frames: %d\n"
		"FPS: %.f\n"
		"Start Frame: %d\n"
		"End Frame: %d\n",
		animation,
		frameCount,
		10.0f,
		startFrame,
		endFrame
		);

	lSequenceInfo->setLabel (str);
}

void MDLViewer :: initTexAnim( int skin )
{
	char str[32];

	sprintf (str, "Frame %d", 0 );
	cTexAnim->removeAll ();
	cTexAnim->add (str);

	for( int i = 0; i < 4; i++ )
	{
		if( g_aliasModel.GetTextureSkin( skin, 0 ) != g_aliasModel.GetTextureSkin( skin, i ))
			break;
		sprintf (str, "Frame %d", i + 1 );
		cTexAnim->add (str);
	}

	if( i != 4 ) cTexAnim->setEnabled( true );
	else cTexAnim->setEnabled( false );
	g_viewerSettings.skinAnim = 0;
	cTexAnim->select (0);
}

void MDLViewer :: initTextures( void )
{
	daliashdr_t *hdr = g_aliasModel.getAliasHeader ();
	if (hdr)
	{
		char str[32];

		cTextures->removeAll ();
		for (int i = 0; i < hdr->numskins; i++)
			cTextures->add (va( "skin %d", i ));
		cTextures->select (0);
		g_viewerSettings.texture = 0;
		if (hdr->numskins > 0)
		{
			char str[32];
			sprintf (str, "Texture (size: %d x %d)", hdr->skinwidth, hdr->skinheight);
			lTexSize->setLabel (str);
		}

		cSkin->setEnabled (hdr->numskins > 1);
		cSkin->removeAll ();

		for (i = 0; i < hdr->numskins; i++)
		{
			sprintf (str, "Skin %d", i + 1);
			cSkin->add (str);
		}

		cSkin->select (0);
		g_aliasModel.SetSkin (0);
		g_viewerSettings.skin = 0;
		initTexAnim( 0 );
	}
}

int main( int argc, char *argv[] )
{
	//
	// make sure, we start in the right directory
	//
	mx_setcwd (mx::getApplicationPath ());

	char cmdline[1024] = "";
	if (argc > 1)
	{
		strcpy (cmdline, argv[1]);
		for (int i = 2; i < argc; i++)
		{
			strcat (cmdline, " ");
			strcat (cmdline, argv[i]);
		}
	}

	if( IsStudioModel( cmdline ) && COM_FileExists( "hlmv.exe" ))
	{
		strcpy (cmdline, "hlmv.exe" );
		for (int i = 1; i < argc; i++)
		{
			strcat (cmdline, " ");
			strcat (cmdline, argv[i]);
		}
		WinExec (cmdline, SW_SHOW);
		return 0;
	}

	LoadViewerSettings();

	mx::init (argc, argv);

	g_MDLViewer = new MDLViewer ();
	g_MDLViewer->setMenuBar (g_MDLViewer->getMenuBar ());
	g_MDLViewer->setBounds (20, 20, 640, 540);
	g_MDLViewer->setVisible (true);

	if( g_viewerSettings.showMaximized )
		g_MDLViewer->Maximize();

	if (strstr (cmdline, ".mdl"))
	{
		g_MDLViewer->loadModel (cmdline);
	}

	int ret = mx::run ();

	mx::cleanup ();

	return ret;
}
