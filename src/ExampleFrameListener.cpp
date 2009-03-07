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
     F:        Toggle frame rate stats on/off
		 R:        Render mode
     T:        Cycle texture filtering
	       Bilinear, Trilinear, Anisotropic(8)
     P:        Toggle on/off display of camera position / orientation
-----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <iostream>
#include <algorithm>
using namespace std;

#include "Defines.h"
#include "Tile.h"
#include "Globals.h"
#include "Functions.h"
#include "ExampleFrameListener.h"

using namespace Ogre;

CEGUI::MouseButton convertButton(OIS::MouseButtonID buttonID)
{
	switch (buttonID)
	{
		case OIS::MB_Left:
			return CEGUI::LeftButton;

		case OIS::MB_Right:
			return CEGUI::RightButton;

		case OIS::MB_Middle:
			return CEGUI::MiddleButton;

		default:
			return CEGUI::LeftButton;
	}
}

void ExampleFrameListener::updateStats(void)
{
	static String currFps = "Current FPS: ";
	static String avgFps = "Average FPS: ";
	static String bestFps = "Best FPS: ";
	static String worstFps = "Worst FPS: ";
	static String tris = "Triangle Count: ";
	static String batches = "Batch Count: ";

	// update stats when necessary
	try {
		OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
		OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
		OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
		OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");

		const RenderTarget::FrameStats& stats = mWindow->getStatistics();
		guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
		guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
		guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS)
			+" "+StringConverter::toString(stats.bestFrameTime)+" ms");
		guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS)
			+" "+StringConverter::toString(stats.worstFrameTime)+" ms");

		OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
		guiTris->setCaption(tris + StringConverter::toString(stats.triangleCount));

		OverlayElement* guiBatches = OverlayManager::getSingleton().getOverlayElement("Core/NumBatches");
		guiBatches->setCaption(batches + StringConverter::toString(stats.batchCount));

		OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
		guiDbg->setCaption(mDebugText);
	}
	catch(...) { /* ignore */ }
}

// Constructor takes a RenderWindow because it uses that to determine input context
ExampleFrameListener::ExampleFrameListener(RenderWindow* win, Camera* cam, SceneManager *sceneManager, CEGUI::Renderer *renderer, bool bufferedKeys, bool bufferedMouse, bool bufferedJoy)
	: mCamera(cam), mTranslateVector(Ogre::Vector3::ZERO), mWindow(win),
	mStatsOn(true), mNumScreenShots(0), mMoveScale(0.0f), mRotScale(0.0f),
	mTimeUntilNextToggle(0), mFiltering(TFO_BILINEAR), mAniso(1),
	mSceneDetailIndex(0), mMoveSpeed(2.7), mRotateSpeed(36),
	mDebugOverlay(0), mInputManager(0), mMouse(0), mKeyboard(0), mJoy(0),
	mGUIRenderer(renderer), zChange(0.0), mZoomSpeed(.33),
	mCurrentTileType(Tile::dirt), mCurrentFullness(100),
	mDragType(ExampleFrameListener::nullDragType)
{
	mCount = 0;
	mCurrentObject = NULL;
	mLMouseDown = false;
	mRMouseDown = false;
	mSceneMgr = sceneManager;
	terminalActive = false;
	prompt = "-->  ";
	terminalWordWrap = 50;

	using namespace OIS;

	mDebugOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");

	LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
	ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	win->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	mInputManager = InputManager::createInputSystem( pl );

	//Create all devices (We only catch joystick exceptions here, as, most people have Key/Mouse)
	mKeyboard = static_cast<Keyboard*>(mInputManager->createInputObject( OISKeyboard, bufferedKeys ));
	mMouse = static_cast<Mouse*>(mInputManager->createInputObject( OISMouse, bufferedMouse ));
	try {
		mJoy = static_cast<JoyStick*>(mInputManager->createInputObject( OISJoyStick, bufferedJoy ));
	}
	catch(...) {
		mJoy = 0;
	}

	//Set initial mouse clipping size
	windowResized(mWindow);

	showDebugOverlay(true);

	//Register as a Window listener
	WindowEventUtilities::addWindowEventListener(mWindow, this);

	mCamNode = cam->getParentSceneNode();
	

	mContinue = true;
	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);

	mRaySceneQuery = mSceneMgr->createRayQuery(Ray());

}

//Adjust mouse clipping area
void ExampleFrameListener::windowResized(RenderWindow* rw)
{
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

//Unattach OIS before window shutdown (very important under Linux)
void ExampleFrameListener::windowClosed(RenderWindow* rw)
{
	//Only close for window that created OIS (the main window in these demos)
	if( rw == mWindow )
	{
		if( mInputManager )
		{
			mInputManager->destroyInputObject( mMouse );
			mInputManager->destroyInputObject( mKeyboard );
			mInputManager->destroyInputObject( mJoy );

			OIS::InputManager::destroyInputSystem(mInputManager);
			mInputManager = 0;
		}
	}
}

ExampleFrameListener::~ExampleFrameListener()
{
	mSceneMgr->destroyQuery(mRaySceneQuery);

	//Remove ourself as a Window listener
	WindowEventUtilities::removeWindowEventListener(mWindow, this);
	windowClosed(mWindow);
}

bool ExampleFrameListener::processUnbufferedKeyInput(const FrameEvent& evt)
{
	using namespace OIS;
	const double xyAccelFactor = 0.04, xyAccelLimit = 0.07;
	const double lrAccelFactor = 3, lrAccelLimit = 10;
	const double udAccelFactor = 1.5, udAccelLimit = 5;

	static double xAccel = 0.0, yAccel = 0.0, lrAccel = 0.0, udAccel;
	static bool xPositive, yPositive, lrPositive, udPositive;

	/*
	// Move left
	if(mKeyboard->isKeyDown(KC_LEFT))
	{
		handleAcceleration(xyAccelFactor, xyAccelLimit, xAccel, xPositive, true, false);
		mTranslateVector.x = -mMoveScale + xAccel;	// Move camera forward

		// Move right
	} else if(mKeyboard->isKeyDown(KC_RIGHT))
		{
			handleAcceleration(xyAccelFactor, xyAccelLimit, xAccel, xPositive, true, true);
			mTranslateVector.x = mMoveScale + xAccel;	// Move camera backward
		}
		else
		{
			handleAcceleration(xyAccelFactor, xyAccelLimit, xAccel, xPositive, false, true);
			mTranslateVector.x += 1.3*xAccel;
		}

	// Move backward
	if(mKeyboard->isKeyDown(KC_DOWN))
	{
		handleAcceleration(xyAccelFactor, xyAccelLimit, yAccel, yPositive, true, false);
		mTranslateVector.y = -mMoveScale + yAccel;	// Move camera forward

		// Move forward
	} else if(mKeyboard->isKeyDown(KC_UP))
		{
			handleAcceleration(xyAccelFactor, xyAccelLimit, yAccel, yPositive, true, true);
			mTranslateVector.y = mMoveScale + yAccel;	// Move camera backward
		}
		else
		{
			handleAcceleration(xyAccelFactor, xyAccelLimit, yAccel, yPositive, false, false);
			mTranslateVector.y += 1.3*yAccel;
		}

	// Tilt up
	if(mKeyboard->isKeyDown(KC_HOME) || mKeyboard->isKeyDown(KC_W))
	{
		handleAcceleration(udAccelFactor, udAccelLimit, udAccel, udPositive, true, true);
		mCamNode->rotate(Ogre::Vector3::UNIT_X, mRotScale + Degree(udAccel), Node::TS_LOCAL);
	}
	else
	{
		// Tilt down
		if(mKeyboard->isKeyDown(KC_END) || mKeyboard->isKeyDown(KC_S))
		{
			handleAcceleration(udAccelFactor, udAccelLimit, udAccel, udPositive, true, false);
			mCamNode->rotate(Ogre::Vector3::UNIT_X, -mRotScale + Degree(udAccel), Node::TS_LOCAL);
		}
		else
		{
			handleAcceleration(udAccelFactor, udAccelLimit, udAccel, udPositive, false, false);
			mCamNode->rotate(Ogre::Vector3::UNIT_X, Degree(udAccel), Node::TS_LOCAL);
		}
	}


	// Turn left
	if(mKeyboard->isKeyDown(KC_DELETE) || mKeyboard->isKeyDown(KC_A))
	{
		handleAcceleration(lrAccelFactor, lrAccelLimit, lrAccel, lrPositive, true, true);
		mCamNode->rotate(Ogre::Vector3::UNIT_Z, mRotScale + Degree(lrAccel), Node::TS_WORLD);
	}
	else
	{
		// Turn right
		if(mKeyboard->isKeyDown(KC_PGDOWN) || mKeyboard->isKeyDown(KC_D))
		{
			handleAcceleration(lrAccelFactor, lrAccelLimit, lrAccel, lrPositive, true, false);
			mCamNode->rotate(Ogre::Vector3::UNIT_Z, -mRotScale + Degree(lrAccel), Node::TS_WORLD);
		}
		else
		{
			handleAcceleration(lrAccelFactor, lrAccelLimit, lrAccel, lrPositive, false, false);
			mCamNode->rotate(Ogre::Vector3::UNIT_Z, Degree(lrAccel), Node::TS_WORLD);
		}
	}

	// Zoom out
	if(mKeyboard->isKeyDown(KC_INSERT) || mKeyboard->isKeyDown(KC_Q))
	{
		mCamNode->translate(mMoveScale*Ogre::Vector3::UNIT_Z, Node::TS_LOCAL);
	}

	// Zoom in
	if(mKeyboard->isKeyDown(KC_PGUP) || mKeyboard->isKeyDown(KC_E))
	{
		mCamNode->translate(-mMoveScale*Ogre::Vector3::UNIT_Z, Node::TS_LOCAL);
	}

	if( mKeyboard->isKeyDown(KC_F) && mTimeUntilNextToggle <= 0 )
	{
		mStatsOn = !mStatsOn;
		showDebugOverlay(mStatsOn);
		mTimeUntilNextToggle = 1;
	}
	*/

	if( mKeyboard->isKeyDown(KC_T) && mTimeUntilNextToggle <= 0 )
	{
		switch(mFiltering)
		{
		case TFO_BILINEAR:
			mFiltering = TFO_TRILINEAR;
			mAniso = 1;
			break;
		case TFO_TRILINEAR:
			mFiltering = TFO_ANISOTROPIC;
			mAniso = 8;
			break;
		case TFO_ANISOTROPIC:
			mFiltering = TFO_BILINEAR;
			mAniso = 1;
			break;
		default: break;
		}
		MaterialManager::getSingleton().setDefaultTextureFiltering(mFiltering);
		MaterialManager::getSingleton().setDefaultAnisotropy(mAniso);

		showDebugOverlay(mStatsOn);
		mTimeUntilNextToggle = 1;
	}

	if(mKeyboard->isKeyDown(KC_SYSRQ) && mTimeUntilNextToggle <= 0)
	{
		std::ostringstream ss;
		ss << "screenshot_" << ++mNumScreenShots << ".png";
		mWindow->writeContentsToFile(ss.str());
		mTimeUntilNextToggle = 0.5;
		mDebugText = "Saved: " + ss.str();
	}

	if(mKeyboard->isKeyDown(KC_R) && mTimeUntilNextToggle <=0)
	{
		mSceneDetailIndex = (mSceneDetailIndex+1)%3 ;
		switch(mSceneDetailIndex) {
			case 0 : mCamera->setPolygonMode(PM_SOLID); break;
			case 1 : mCamera->setPolygonMode(PM_WIREFRAME); break;
			case 2 : mCamera->setPolygonMode(PM_POINTS); break;
		}
		mTimeUntilNextToggle = 0.5;
	}

	static bool displayCameraDetails = false;
	if(mKeyboard->isKeyDown(KC_P) && mTimeUntilNextToggle <= 0)
	{
		displayCameraDetails = !displayCameraDetails;
		mTimeUntilNextToggle = 0.5;
		if (!displayCameraDetails)
			mDebugText = "";
	}

	// Print camera details
	if(displayCameraDetails)
		mDebugText = "P: " + StringConverter::toString(mCamera->getDerivedPosition()) +
					 " " + "O: " + StringConverter::toString(mCamera->getDerivedOrientation());

	// Return true to continue rendering
	return true;
}

bool ExampleFrameListener::processUnbufferedMouseInput(const FrameEvent& evt)
{
	return true;
}

void ExampleFrameListener::moveCamera(double frameTime)
{
	// Make all the changes to the camera
	// Note that YAW direction is around a fixed axis (freelook style) rather than a natural YAW
	//(e.g. airplane)
	//mCamera->yaw(mRotX);
	//mCamera->pitch(mRotY);
	//mCamera->roll(mRotZ);
	Ogre::Vector3 tempVector = mCamNode->getPosition();
	mCamNode->translate(mTranslateVector * frameTime, Node::TS_LOCAL);
	Ogre::Vector3 tempVector2 = mCamNode->getPosition();
	tempVector2.z = tempVector.z + zChange*frameTime*mZoomSpeed;
	mCamNode->setPosition(tempVector2);

	mCamNode->rotate(Ogre::Vector3::UNIT_X, Degree(mRotateLocalVector.x * frameTime), Node::TS_LOCAL);
	mCamNode->rotate(Ogre::Vector3::UNIT_Y, Degree(mRotateLocalVector.y * frameTime), Node::TS_LOCAL);
	mCamNode->rotate(Ogre::Vector3::UNIT_Z, Degree(mRotateLocalVector.z * frameTime), Node::TS_LOCAL);

	mCamNode->rotate(Ogre::Vector3::UNIT_X, Degree(mRotateWorldVector.x * frameTime), Node::TS_WORLD);
	mCamNode->rotate(Ogre::Vector3::UNIT_Y, Degree(mRotateWorldVector.y * frameTime), Node::TS_WORLD);
	mCamNode->rotate(Ogre::Vector3::UNIT_Z, Degree(mRotateWorldVector.z * frameTime), Node::TS_WORLD);
}

void ExampleFrameListener::showDebugOverlay(bool show)
{
	if (mDebugOverlay)
	{
		if (show)
			mDebugOverlay->show();
		else
			mDebugOverlay->hide();
	}
}

// Override frameStarted event to process that (don't care about frameEnded)
bool ExampleFrameListener::frameStarted(const FrameEvent& evt)
{
	using namespace OIS;

	if(mWindow->isClosed())	return false;

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();
	if( mJoy ) mJoy->capture();

	bool buffJ = (mJoy) ? mJoy->buffered() : true;

	//Check if one of the devices is not buffered
	if( !mMouse->buffered() || !mKeyboard->buffered() || !buffJ )
	{
		// one of the input modes is immediate, so setup what is needed for immediate movement
		if (mTimeUntilNextToggle >= 0)
			mTimeUntilNextToggle -= evt.timeSinceLastFrame;

		// Move about 100 units per second
		mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
		// Take about 10 seconds for full rotation
		mRotScale = mRotateSpeed * evt.timeSinceLastFrame;

		mRotX = 0;
		mRotY = 0;
		mRotZ = 0;
		mTranslateVector = Ogre::Vector3::ZERO;
	}

	// Move about 100 units per second
	mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
	// Take about 10 seconds for full rotation
	mRotScale = mRotateSpeed * evt.timeSinceLastFrame;

	//Check to see which device is not buffered, and handle it
	if( !mKeyboard->buffered() )
		if( processUnbufferedKeyInput(evt) == false )
			return false;
	if( !mMouse->buffered() )
		if( processUnbufferedMouseInput(evt) == false )
			return false;

	if( !mMouse->buffered() || !mKeyboard->buffered() || !buffJ )
		moveCamera(evt.timeSinceLastFrame);

	moveCamera(evt.timeSinceLastFrame);
	return mContinue;
}

bool ExampleFrameListener::frameEnded(const FrameEvent& evt)
{
	updateStats();
	return true;
}

// This function causes the camera to "coast" a bit after we stop holding the
// button to move in that direction.  It also makes the camera movement speed
// up in a given direction if the key to move that way is held down for a
// period of time.
void ExampleFrameListener::handleAcceleration(double accelFactor, double accelLimit, double &accel, bool &positive, bool driven, bool sameDir)
{
	if(!sameDir && driven)
	{
		positive = false;
		accel -= accelFactor;

		if(accel < -accelLimit) accel = -accelLimit;

		return;
	}

	if(sameDir && driven)
	{
		positive = true;
		accel += accelFactor;

		if(accel > accelLimit) accel = accelLimit;

		return;
	}

	// The following if statement is guaranteed to be true so it could be optimised out
	if(!driven)
	{
		if(positive)
		{
			accel -= accelFactor;
			if(accel < 0.0) accel = 0.0;
		}
		else
		{
			accel += accelFactor;
			if(accel > 0.0) accel = 0.0;
		}
	}
}

bool ExampleFrameListener::quit(const CEGUI::EventArgs &e)
{
	mContinue = false;
	return true;
}

bool ExampleFrameListener::mouseMoved(const OIS::MouseEvent &arg)
{
	CEGUI::System::getSingleton().injectMouseMove(arg.state.X.rel, arg.state.Y.rel);
	string  resultName;

		//FIXME:  This code should be put into a function it is duplicated by mousePressed()
		// Setup the ray scene query, use CEGUI's mouse position
		CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();
		Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.d_x/float(arg.state.width), mousePos.d_y/float(arg.state.height));
		mRaySceneQuery->setRay(mouseRay);
		mRaySceneQuery->setSortByDistance(true);
		
		// Execute query
		RaySceneQueryResult &result = mRaySceneQuery->execute();
		RaySceneQueryResult::iterator itr = result.begin( );

		if(mDragType == ExampleFrameListener::tileSelection || mDragType == ExampleFrameListener::nullDragType)
		{
			// See if the mouse is over any creatures
			while (itr != result.end() )
			{
				if(itr->movable != NULL)
				{
					resultName = itr->movable->getName();

					if(resultName.find("Creature_") != string::npos)
					{
						mSceneMgr->getEntity(resultName);
						mSceneMgr->getEntity("SquareSelector")->setVisible(false);
						break;
					}

				}

				itr++;
			}

			// If no creatures are under the  mouse run through the list again to check for tiles
			itr = result.begin( );
			while(itr != result.end())
			{
				if(itr->movable != NULL)
				{
					resultName = itr->movable->getName();
					if(resultName.find("Level_") != string::npos)
					{
						sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

						mSceneMgr->getEntity("SquareSelector")->setVisible(true);
						mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(yPos/BLENDER_UNITS_PER_OGRE_UNIT, xPos/BLENDER_UNITS_PER_OGRE_UNIT, 0);

						if(mLMouseDown)
						{
							for(int i = 0; i < gameMap.numTiles(); i++)
							{
								Tile *tempTile = gameMap.getTile(i);
								if(tempTile->x >= min(xPos, mLStartDragX) && \
										tempTile->x <= max(xPos, mLStartDragX) && \
										tempTile->y >= min(yPos, mLStartDragY) && \
										tempTile->y <= max(yPos, mLStartDragY))
								{
									gameMap.getTile(i)->setSelected(true);
								}
								else
								{
									gameMap.getTile(i)->setSelected(false);
								}

							}
						}

						if(mRMouseDown)
						{
							for(int i = 0; i < gameMap.numTiles(); i++)
							{
								Tile *tempTile = gameMap.getTile(i);
								if(tempTile->x >= min(xPos, mRStartDragX) && \
										tempTile->x <= max(xPos, mRStartDragX) && \
										tempTile->y >= min(yPos, mRStartDragY) && \
										tempTile->y <= max(yPos, mRStartDragY))
								{
									gameMap.getTile(i)->setSelected(true);
								}
								else
								{
									gameMap.getTile(i)->setSelected(false);
								}

							}
						}

						break;
					}
				}

				itr++;
			}
		}
		else //if(mDragType == ExampleFrameListener::creature)
		{
			itr = result.begin( );
			while(itr != result.end())
			{
				if(itr->movable != NULL)
				{
					resultName = itr->movable->getName();
					cout << resultName << "\n";
					if(resultName.find("Level_") != string::npos)
					{
						sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);
			cout << "Other thing happenening\n" << xPos << "\t" << yPos << "\n";
			cout.flush();
			

						mSceneMgr->getEntity("SquareSelector")->setVisible(true);
						mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(yPos/BLENDER_UNITS_PER_OGRE_UNIT, xPos/BLENDER_UNITS_PER_OGRE_UNIT, 0);
					}
				}

				itr++;
			}
		}

	return true;
}

bool ExampleFrameListener::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id));
	string  resultName;


		//FIXME:  This code should be put into a function it is duplicated by mousePressed()
		// Setup the ray scene query, use CEGUI's mouse position
		CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();
		Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.d_x/float(arg.state.width), mousePos.d_y/float(arg.state.height));
		mRaySceneQuery->setRay(mouseRay);
		mRaySceneQuery->setSortByDistance(true);
		
		// Execute query
		RaySceneQueryResult &result = mRaySceneQuery->execute();
		RaySceneQueryResult::iterator itr = result.begin( );

		// See if the mouse is over any creatures
		while (itr != result.end() )
		{
			if(itr->movable != NULL)
			{
				resultName = itr->movable->getName();

				if(resultName.find("Creature_") != string::npos)
				{
					Entity *resultEnt = mSceneMgr->getEntity(resultName);
					mSceneMgr->getEntity("SquareSelector")->setVisible(false);

					
					mDraggedCreature = resultName.substr(((string)"Creature_").size(), resultName.size());
					SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(mDraggedCreature	+ "_node");
					mSceneMgr->getRootSceneNode()->removeChild(creatureSceneNode);
					mSceneMgr->getSceneNode("SquareSelectorNode")->addChild(creatureSceneNode);
					creatureSceneNode->setPosition(0,0,0);
					mDragType = ExampleFrameListener::creature;
					break;
				}

			}

			itr++;
		}

		// If no creatures are under the  mouse run through the list again to check for tiles
		itr = result.begin( );
		while(itr != result.end())
		{
			itr++;
			if(itr->movable != NULL)
			{
				if(resultName.find("Level_") != string::npos)
				{
					mDragType = ExampleFrameListener::tileSelection;
					break;
				}
			}
		}

	// Left mouse button down
	if (id == OIS::MB_Left)
	{
		
		mLMouseDown = true;
		mLStartDragX = xPos;
		mLStartDragY = yPos;
	}

	// Right mouse button down
	if(id == OIS::MB_Right)
	{

		mRMouseDown = true;
		mRStartDragX = xPos;
		mRStartDragY = yPos;
	}
	       
	return true;
}

bool ExampleFrameListener::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id));

	// Unselect all tiles
	for(int i = 0; i < gameMap.numTiles(); i++)
	{
		gameMap.getTile(i)->setSelected(false);
	}

	// Left mouse button up
	if (id == OIS::MB_Left)
	{
		// Check to see if we are moving a creature
		if(mDragType == ExampleFrameListener::creature)
		{
			SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(mDraggedCreature	+ "_node");
			mSceneMgr->getSceneNode("SquareSelectorNode")->removeChild(creatureSceneNode);
			mSceneMgr->getRootSceneNode()->addChild(creatureSceneNode);
			creatureSceneNode->setPosition(yPos/BLENDER_UNITS_PER_OGRE_UNIT, xPos/BLENDER_UNITS_PER_OGRE_UNIT, 0);
			mDragType = ExampleFrameListener::nullDragType;
			gameMap.getCreature(mDraggedCreature)->position = Ogre::Vector3(yPos, xPos, 0);

		}

		// Check to see if we are dragging out a selection of tiles
		else if(mDragType == ExampleFrameListener::tileSelection)
		{
			for(int i = min(xPos, mLStartDragX); i <= max(xPos, mLStartDragX); i++)
			{
				for(int j = min(yPos, mLStartDragY); j <= max(yPos, mLStartDragY); j++)
				{
					//gameMap.getTile(i, j)->setType( Tile::nextTileType(gameMap.getTile(i, j)->getType()) );
					gameMap.getTile(i, j)->setType( mCurrentTileType );
					gameMap.getTile(i, j)->setFullness( mCurrentFullness );
				}
			}
		}

		mLMouseDown = false;
	}

	// Right mouse button up
	if (id == OIS::MB_Right)
	{
		mRMouseDown = false;
	}

	return true;
}


bool ExampleFrameListener::keyPressed(const OIS::KeyEvent &arg)
{
	using namespace OIS;
	char tempArray[255];

	CEGUI::System *sys = CEGUI::System::getSingletonPtr();
	sys->injectKeyDown(arg.key);
	sys->injectChar(arg.text);

	if(!terminalActive)
	{
		// If the terminal is not active
		// Keyboard is used to move around and play game
		switch(arg.key)
		{
			case OIS::KC_GRAVE:
				terminalActive = true;

				if(commandOutput.size() > 0)
					printText(commandOutput + "\n" + prompt + promptCommand);
				else
					printText(prompt + promptCommand);

				mKeyboard->setTextTranslation(Keyboard::Ascii);
				break;

			// Move left
			case KC_LEFT:
			case KC_A:
				mTranslateVector.x += -mMoveSpeed;	// Move camera left
				break;

			// Move right
			case KC_RIGHT:
			case KC_D:
				mTranslateVector.x += mMoveSpeed;	// Move camera right
				break;
			
			// Move forward
			case KC_UP:
			case KC_W:
				mTranslateVector.y += mMoveSpeed;	// Move camera forward
				break;

			// Move backward
			case KC_DOWN:
			case KC_S:
				mTranslateVector.y += -mMoveSpeed;	// Move camera backward
				break;

			// Move down
			case KC_PGUP:
			case KC_E:
				zChange += -mMoveSpeed;	// Move straight down
				break;

			// Move up
			case KC_INSERT:
			case KC_Q:
				zChange += mMoveSpeed;	// Move straight up
				break;

			// Tilt up
			case KC_HOME:
				mRotateLocalVector.x += mRotateSpeed.valueDegrees();
				break;

			// Tilt down
			case KC_END:
				mRotateLocalVector.x += -mRotateSpeed.valueDegrees();
				break;

			// Turn left
			case KC_DELETE:
				mRotateWorldVector.z += mRotateSpeed.valueDegrees();
				break;

			// Turn right
			case KC_PGDOWN:
				mRotateWorldVector.z += -mRotateSpeed.valueDegrees();
				break;

			//Toggle mCurrentTileType
			case KC_R:
				mCurrentTileType = Tile::nextTileType(mCurrentTileType);
				sprintf(tempArray, "Tile type:  %s", Tile::tileTypeToString(mCurrentTileType).c_str());
				MOTD = tempArray;
				printText(MOTD);
				break;

			//Toggle mCurrentFullness
			case KC_T:
				mCurrentFullness = Tile::nextTileFullness(mCurrentFullness);
				sprintf(tempArray, "Tile fullness:  %i", mCurrentFullness);
				MOTD = tempArray;
				printText(MOTD);
				break;

			// Toggle the framerate display
			case KC_F:
				mStatsOn = !mStatsOn;
				showDebugOverlay(mStatsOn);
				break;

			// Quit the game
			case KC_ESCAPE:
				writeGameMapToFile( ((string)"Media/levels/Test.level" + (string)".out") );
				mContinue = false;
				break;
		}
	}
	else
	{
		// If the terminal is active
		// Keyboard is used to command the terminal
		switch(arg.key)
		{
			case KC_RETURN:
				executePromptCommand();
				break;

			case KC_GRAVE:
			case KC_ESCAPE:
				terminalActive = false;
				printText(MOTD);
				break;

			default:
				// If the key translates to a valid character
				// for the commandline we add it to the current
				// promptCommand
				if( ((string)"abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?1234567890-=\\!@#$%^&*()_+|;\':\"[]{}").find(arg.text) != string::npos)
				{
					promptCommand += arg.text;
				}
				else
				{
					switch(arg.key)
					{
						case KC_BACK:
							promptCommand = promptCommand.substr(0,promptCommand.size()-1);
							break;
					}
				}

				break;
		}

		// The keyboard processing may have de-activated the terminal
		if(terminalActive)
		{
			if(commandOutput.size() > 0)
				printText(commandOutput + "\n" + prompt + promptCommand);
			else
				printText(prompt + promptCommand);
		}

	}

	return mContinue;
}

bool ExampleFrameListener::keyReleased(const OIS::KeyEvent &arg)
{
	using namespace OIS;
	CEGUI::System::getSingleton().injectKeyUp(arg.key);

	if(!terminalActive)
	{
		switch(arg.key)
		{
			// Move left
			case KC_LEFT:
			case KC_A:
				mTranslateVector.x -= -mMoveSpeed;	// Move camera forward
				break;

			// Move right
			case KC_D:
			case KC_RIGHT:
				mTranslateVector.x -= mMoveSpeed;	// Move camera backward
				break;
			
			// Move forward
			case KC_UP:
			case KC_W:
				mTranslateVector.y -= mMoveSpeed;	// Move camera forward
				break;

			// Move backward
			case KC_DOWN:
			case KC_S:
				mTranslateVector.y -= -mMoveSpeed;	// Move camera backward
				break;

			// Move down
			case KC_PGUP:
			case KC_E:
				zChange -= -mMoveSpeed;	// Move straight down
				break;

			// Move up
			case KC_INSERT:
			case KC_Q:
				zChange -= mMoveSpeed;	// Move straight up
				break;

			// Tilt up
			case KC_HOME:
				mRotateLocalVector.x -= mRotateSpeed.valueDegrees();
				break;

			// Tilt down
			case KC_END:
				mRotateLocalVector.x -= -mRotateSpeed.valueDegrees();
				break;

			// Turn left
			case KC_DELETE:
				mRotateWorldVector.z -= mRotateSpeed.valueDegrees();
				break;

			// Turn right
			case KC_PGDOWN:
				mRotateWorldVector.z -= -mRotateSpeed.valueDegrees();
				break;

		}
	}

	return true;
}

void ExampleFrameListener::printText(string text)
{
	string tempString;
	int lineLength = 0;
	for(int i = 0; i < text.size(); i++)
	{
		if(text[i] == '\n')
		{
			lineLength = 0;
		}

		if(lineLength < terminalWordWrap)
		{
			lineLength++;
		}
		else
		{
			lineLength = 0;
			tempString += "\n";
		}

		tempString += text[i];
	}

	TextRenderer::getSingleton().setText("DebugMessages", tempString);
}

void ExampleFrameListener::executePromptCommand()
{
	int firstSpace, lastSpace;
	stringstream tempSS;

	commandOutput = "";
	command = "";
	arguments = "";

	if(promptCommand.size() == 0)
	{
		promptCommand = "";
		printText(MOTD);
		terminalActive = false;

		return;
	}

	// Split the raw text into command and argument strings
	firstSpace = promptCommand.find(" ");
	if(firstSpace != string::npos)
	{
		command = promptCommand.substr(0, firstSpace);

		// Skip any extra spaces in between the command and the arguments
		lastSpace = firstSpace;
		while(promptCommand[lastSpace] == ' ')
		{
			lastSpace++;
		}

		arguments = promptCommand.substr(lastSpace, promptCommand.size()-lastSpace);
	}
	else
	{
		command = promptCommand;
	}

	// Force command to lower case
	for(int i = 0; i < command.size(); i++)
	{
		command[i] = tolower(command[i]);
	}

	// Begin Command Implementation
	//
	// All the code from here to the rest of the function is the implementation code for specific commands which are handled by the terminal.
	// Exit the program
	if(command.compare("quit") == 0 || command.compare("exit") == 0)
	{
		mContinue = false;
	}
	
		// Repeat the arguments of the command back to you
		else if(command.compare("echo") == 0)
		{
			commandOutput = arguments;
		}

		// Write the current level out to file specified as an argument
		else if(command.compare("save") == 0)
		{
			if(arguments.size() > 0)
			{
				string tempFileName = "Media/levels/" + arguments + ".level";
				writeGameMapToFile(tempFileName);
				commandOutput = "File saved to   " + tempFileName;
			}
			else
			{
				commandOutput = "ERROR:  No level name given";
			}
		}

		// Clear the current level and load a new one from a file
		else if(command.compare("load") == 0)
		{
			if(arguments.size() > 0)
			{
				gameMap.clearAll();

				string tempString;
				char tempArray[255];
				tempString = "Media/levels/" + arguments + ".level";
				readGameMapFromFile(tempString);

				sprintf(tempArray, "Successfully loaded file:  %s\nNum tiles:  %i\nNum class descriptions:  %i\nNum creatures:  %i", tempString.c_str(), gameMap.numTiles(), gameMap.numClassDescriptions(), gameMap.numCreatures());
				commandOutput = tempArray;

				gameMap.createAllEntities();
			}
			else
			{
				commandOutput = "ERROR:  No level name given";
			}
		}

		// Set the ambient light color
		else if(command.compare("ambientlight") == 0)
		{
			double tempR, tempG, tempB;
			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> tempR >> tempG >> tempB;
				mSceneMgr->setAmbientLight(ColourValue(tempR,tempG,tempB));
			}
		}

		// Print the help message
		else if(command.compare("help") == 0)
		{
			if(arguments.size() > 0)
			{
				commandOutput = "Help for command:  " + arguments;
			}
			else
			{
				commandOutput = HELP_MESSAGE;
			}
		}

		// A utility to set the wordrap on the terminal to a specific value
		else if(command.compare("termwidth") == 0)
		{
			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> terminalWordWrap;
			}
			else
			{
				// Print the "tens" place line at the top
				int maxWidth = terminalWordWrap;
				for(int i = 0; i < maxWidth/10; i++)
				{
					char tempArray[255];
					sprintf(tempArray, "         %i", i+1);
					commandOutput += tempArray;
				}

				commandOutput += "\n";

				// Print the "ones" place
				for(int i = 0; i < maxWidth-1; i++)
				{
					string tempString = "1234567890";
					commandOutput += tempString.substr(i%10, 1);
				}
			}

		}

		// A utility which adds a new section of the map given as the
		// rectangular region between two pairs of coordinates
		else if(command.compare("addtiles") == 0)
		{
			int x1, y1, x2, y2;
			tempSS.str(arguments);
			tempSS >> x1 >> y1 >> x2 >> y2;
			char tempArray[255];
			int xMin, yMin, xMax, yMax;
			xMin = min(x1, x2);
			xMax = max(x1, x2);
			yMin = min(y1,y2);
			yMax = max(y1, y2);

			sprintf(tempArray, "Creating tiles for region:\n\n\t(%i, %i)\tto\t(%i, %i)", xMin, yMin, xMax, yMax);

			for(int j = yMin; j < yMax; j++)
			{
				for(int i = xMin; i < xMax; i++)
				{
					if(gameMap.getTile(i, j) == NULL)
					{

						char tempArray[255];
						sprintf(tempArray, "Level_%3i_%3i", i, j);
						Tile *t = new Tile(i, j, Tile::dirt, 100);
						t->name = tempArray;
						gameMap.addTile(t);
						t->createMesh();
					}
				}
			}

			commandOutput = tempArray;
		}

		// A utility to set the camera movement speed
		else if(command.compare("movespeed") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> mMoveSpeed;
				sprintf(tempArray, "movespeed set to %lf", mMoveSpeed);
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Current movespeed is %lf", mMoveSpeed);
				commandOutput = tempArray;
			}
		}

		else if(command.compare("rotatespeed") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				double tempDouble;
				tempSS.str(arguments);
				tempSS >> tempDouble;
				mRotateSpeed = Ogre::Degree(tempDouble);
				sprintf(tempArray, "rotatespeed set to %lf", mRotateSpeed.valueDegrees());
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Current rotatespeed is %lf", mRotateSpeed.valueDegrees());
				commandOutput = tempArray;
			}
		}

		// Add a new instance of a creature to the current map.  The argument is
		// read as if it were a line in a .level file.
		else if(command.compare("addcreature") == 0)
		{
			if(arguments.size() > 0)
			{
				// Creature the creature and add it to the gameMap
				Creature *tempCreature = new Creature;
				stringstream tempSS(arguments);
				tempSS >> tempCreature;
				tempCreature->meshName = gameMap.getClass(tempCreature->className)->meshName;
				tempCreature->scale = gameMap.getClass(tempCreature->className)->scale;
				gameMap.addCreature(tempCreature);

				// Create the mesh and SceneNode for the new creature
				Entity *ent = mSceneMgr->createEntity( ("Creature_" + tempCreature->name).c_str(), tempCreature->meshName.c_str());
				SceneNode *node = mSceneMgr->getRootSceneNode()->createChildSceneNode( (tempCreature->name + "_node").c_str() );
				node->setPosition(tempCreature->position/BLENDER_UNITS_PER_OGRE_UNIT);
				//FIXME: Something needs to be done about the caling issue here.
				//node->setScale(1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT);
				node->setScale(tempCreature->scale);
				node->attachObject(ent);
			}
		}

		// Adds the basic information about a type of creature (mesh name, scaling, etc)
		else if(command.compare("addclass") == 0)
		{
			if(arguments.size() > 0)
			{
				//FIXME: this code should be standardaized with the equivalent code in readGameMapFromFile()
				// This will require making CreatureClass a base class of Creature
				double tempX, tempY, tempZ;
				string tempString, tempString2;
				tempSS.str(arguments);
				tempSS >> tempString >> tempString2 >> tempX >> tempY >> tempZ;
				Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ));
				gameMap.addClassDescription(p);
			}

		}

		// Print out various lists of information, the creatures in the
		// scene, the levels available for loading, etc
		else if(command.compare("list") == 0)
		{
			if(arguments.size() > 0)
			{
				string tempString;
				tempSS.str(tempString);

				if(arguments.compare("creatures") == 0)
				{
					for(int i = 0; i < gameMap.numCreatures(); i++)
					{
						tempSS << gameMap.getCreature(i);
						commandOutput += tempSS.str();
					}
				}
			}
		}

		// clearmap   Erase all of the tiles leaving an empty map
		else if(command.compare("newmap") == 0)
		{
			if(arguments.size() > 0)
			{
				int tempX, tempY;

				tempSS.str(arguments);
				tempSS >> tempX >> tempY;
				gameMap.createNewMap(tempX, tempY);
			}
		}

		else commandOutput = "Command not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.";

	promptCommand = "";
}

