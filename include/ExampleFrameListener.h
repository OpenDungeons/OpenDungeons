/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2006 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
LGPL like the rest of the engine.
-----------------------------------------------------------------------------
*/
/*
-----------------------------------------------------------------------------
Filename:    ExampleFrameListener.h
Description: Defines an example frame listener which responds to frame events.
This frame listener just moves a specified camera around based on
keyboard and mouse movements.
-----------------------------------------------------------------------------
*/

#ifndef __ExampleFrameListener_H__
#define __ExampleFrameListener_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "Ogre.h"
#include "OgreStringConverter.h"
#include "OgreException.h"

#include <deque>
#include <CEGUI/CEGUI.h>
#include <OIS/OIS.h>
#include <OgreCEGUIRenderer.h>

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OIS/OIS.h>

using namespace Ogre;

#include "TextRenderer.h"
#include "Socket.h"
#include "Tile.h"
#include "ChatMessage.h"

/*! \brief The main OGRE rendering class.
 *
 * This class provides the rendering framework for the OGRE subsystem, as well
 * as processing keyboard and mouse input from the user.  It loads and
 * initializes the meshes for creatures and tiles, moves the camera, and
 * displays the terminal and chat messages on the game screen.
 */
class ExampleFrameListener: public FrameListener, public WindowEventListener, public OIS::MouseListener, public OIS::KeyListener
{
protected:
	void updateStats(void);

public:
	// Constructor takes a RenderWindow because it uses that to determine input context
	ExampleFrameListener(RenderWindow* win, Camera* cam, SceneManager *sceneManager, CEGUI::Renderer *renderer, bool bufferedKeys, bool bufferedMouse, bool bufferedJoy);

	//Adjust mouse clipping area
	virtual void windowResized(RenderWindow* rw);

	//Unattach OIS before window shutdown (very important under Linux)
	virtual void windowClosed(RenderWindow* rw);
	virtual ~ExampleFrameListener();

	void moveCamera(double frameTime);
	void showDebugOverlay(bool show);

	// Override frameStarted event to process that (don't care about frameEnded)
	bool frameStarted(const FrameEvent& evt);
	bool frameEnded(const FrameEvent& evt);

	//CEGUI Functions
	bool quit(const CEGUI::EventArgs &e);
	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool keyPressed(const OIS::KeyEvent &arg);
	bool keyReleased(const OIS::KeyEvent &arg);

	// Console functions
	void printText(string text);
	void executePromptCommand();
	string getHelpText(string arg);
	
	// Console variables
	string command, arguments, commandOutput, prompt;
	//deque< pair<time_t, string> > chatMessages;
	deque< ChatMessage* > chatMessages;
	string consoleBuffer, promptCommand, chatString;


	// Multiplayer stuff
	vector<Socket*> clientSockets;
	pthread_t clientThread;
	pthread_t serverThread;
	pthread_t serverNotificationThread;
	vector<pthread_t*> clientHandlerThreads;
	pthread_t creatureThread;

protected:
	Camera* mCamera;
	SceneNode *mCamNode;

	Ogre::Vector3 mTranslateVector;
	Ogre::Vector3 mMouseTranslateVector;
	double zChange;
	Ogre::Vector3 mRotateLocalVector;
	Ogre::Vector3 mRotateWorldVector;
	RenderWindow* mWindow;
	bool mStatsOn;

	std::string mDebugText;

	unsigned int mNumScreenShots;
	float mMoveScale;
	float mZoomSpeed;
	Degree mRotScale;
	// just to stop toggles flipping too fast
	Real mTimeUntilNextToggle ;
	Radian mRotX, mRotY, mRotZ;
	TextureFilterOptions mFiltering;
	int mAniso;
	Tile::TileType mCurrentTileType;
	int mCurrentFullness, mCurrentTileRadius;
	bool mBrushMode;
	double frameDelay;

	int mSceneDetailIndex ;
	Real mMoveSpeed;
	Degree mRotateSpeed;
	Overlay* mDebugOverlay;

	//OIS Input devices
	OIS::InputManager* mInputManager;
	OIS::Mouse*    mMouse;
	OIS::Keyboard* mKeyboard;
	OIS::JoyStick* mJoy;

	// Mouse query stuff
	RaySceneQuery *mRaySceneQuery;     // The ray scene query pointer
	bool mLMouseDown, mRMouseDown;     // True if the mouse buttons are down
	int mLStartDragX, mLStartDragY;    // The start tile coordinates for a left drag
	int mRStartDragX, mRStartDragY;    // The start tile coordinates for a left drag
	int mCount;                        // The number of robots on the screen
	SceneManager *mSceneMgr;           // A pointer to the scene manager
	SceneNode *mCurrentObject;         // The newly created object
	CEGUI::Renderer *mGUIRenderer;     // CEGUI renderer
	int xPos, yPos;
	bool digSetBool;                   // For server mode - hods whether to mark or unmark a tile for digging

	enum DragType {creature, tileSelection, tileBrushSelection, nullDragType};

private:
	void handleAcceleration(double accelFactor, double accelLimit, double &accel, bool &positive, bool driven, bool sameDir);
	bool mContinue;
	bool terminalActive;
	int terminalWordWrap;

	DragType mDragType;
	string mDraggedCreature;
};

#endif
