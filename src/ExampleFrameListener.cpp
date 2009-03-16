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

#include "Socket.h"
#include "Defines.h"
#include "Tile.h"
#include "Globals.h"
#include "Functions.h"
#include "ExampleFrameListener.h"
#include "Creature.h"
#include "ChatMessage.h"
#include "Network.h"

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
	mSceneDetailIndex(0), mMoveSpeed(50.0), mRotateSpeed(50),
	mDebugOverlay(0), mInputManager(0), mMouse(0), mKeyboard(0), mJoy(0),
	mZoomSpeed(.5),
	mCurrentTileType(Tile::dirt), mCurrentFullness(100),
	serverSocket(NULL), clientSocket(NULL)
{
	mCount = 0;
	mCurrentObject = NULL;
	mLMouseDown = false;
	mRMouseDown = false;
	mSceneMgr = sceneManager;
	terminalActive = false;
	prompt = "-->  ";
	terminalWordWrap = 78;
	me = new Player;
	me->nick = "";
	mDragType = ExampleFrameListener::nullDragType;
	frameDelay = 0.0;
	mGUIRenderer = renderer;
	zChange = 0.0;

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
	gameMap.clearAll();
	mSceneMgr->destroyQuery(mRaySceneQuery);

	//Remove ourself as a Window listener
	WindowEventUtilities::removeWindowEventListener(mWindow, this);
	windowClosed(mWindow);
}

//FIXME:  This function is no longer used and should be deleted once the
// remaining functionality has been moved over to the buffered keyboard hadler
bool ExampleFrameListener::processUnbufferedKeyInput(const FrameEvent& evt)
{
	using namespace OIS;
	/*
	const double xyAccelFactor = 0.04, xyAccelLimit = 0.07;
	const double lrAccelFactor = 3, lrAccelLimit = 10;
	const double udAccelFactor = 1.5, udAccelLimit = 5;

	static double xAccel = 0.0, yAccel = 0.0, lrAccel = 0.0, udAccel;
	static bool xPositive, yPositive, lrPositive, udPositive;
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
	double horizontalSpeedFactor = (tempVector2.z >= 25.0) ? 1.0 : tempVector2.z/(25.0);
	tempVector2.x = tempVector.x + (mMouseTranslateVector.x + (tempVector2.x - tempVector.x)) * horizontalSpeedFactor;
	tempVector2.y = tempVector.y + (mMouseTranslateVector.y + (tempVector2.y - tempVector.y)) * horizontalSpeedFactor;
	
	// Prevent camera from moving down into the tiles
	if(tempVector2.z <= 4.5)
		tempVector2.z = 4.5;

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

	// Process the queue of render tasks from the other threads
	while(renderQueue.size() > 0)
	{
		char tempString[255];
		char meshName[255];
		string tileTypeString;
		Entity *ent;
		SceneNode *node;
		Tile *curTile = NULL;
		Creature *curCreature = NULL;

		sem_wait(&renderQueueSemaphore);
		RenderRequest *curReq = renderQueue.front();
		renderQueue.pop_front();
		sem_post(&renderQueueSemaphore);

		switch(curReq->type)
		{
			case RenderRequest::refreshTile:
				curTile = (Tile*)curReq->p;
				if(mSceneMgr->hasSceneNode( (curTile->name + "_node").c_str() ) )
				{
					mSceneMgr->getSceneNode( (curTile->name + "_node").c_str() )->detachObject(curTile->name.c_str());
					mSceneMgr->destroyEntity(curTile->name.c_str());

					string tileTypeString = Tile::tileTypeToString(curTile->getType());
					sprintf(meshName, "%s%i.mesh", tileTypeString.c_str(), curTile->getFullnessMeshNumber());
					ent = mSceneMgr->createEntity(curTile->name.c_str(), meshName);
					ent->setNormaliseNormals(true);

					mSceneMgr->getSceneNode((curTile->name + "_node").c_str())->attachObject(ent);
				}
				break;

			case RenderRequest::createTile:
				curTile = (Tile*)curReq->p;
				tileTypeString = Tile::tileTypeToString(curTile->getType());

				sprintf(meshName, "%s%i.mesh", tileTypeString.c_str(), curTile->getFullnessMeshNumber());
				ent = mSceneMgr->createEntity(curTile->name.c_str(), meshName);

				sprintf(tempString, "%s_node", curTile->name.c_str());
				node = mSceneMgr->getRootSceneNode()->createChildSceneNode(tempString);
				//node->setPosition(Ogre::Vector3(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, 0));
				node->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0));
				node->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
				ent->setNormaliseNormals(true);

				node->attachObject(ent);
				break;

			case RenderRequest::destroyTile:
				cout << "Destroying tile\n";
				cout.flush();

				curTile = (Tile*)curReq->p;
				if(mSceneMgr->hasEntity(curTile->name.c_str()))
				{
					ent = mSceneMgr->getEntity(curTile->name.c_str());
					node = mSceneMgr->getSceneNode((curTile->name + "_node").c_str());
					//mSceneMgr->getRootSceneNode()->detachObject((curTile->name + "_node").c_str());
					node->detachAllObjects();
					mSceneMgr->destroySceneNode((curTile->name + "_node").c_str());
					mSceneMgr->destroyEntity(ent);
				}
				break;

			case RenderRequest::deleteTile:
				cout << "Deleting tile\n";
				cout.flush();

				curTile = (Tile*)curReq->p;
				delete curTile;
				break;

			case RenderRequest::createCreature:
				curCreature = (Creature*)curReq->p;
				ent = mSceneMgr->createEntity( ("Creature_" + curCreature->name).c_str(), curCreature->meshName.c_str());
				node = mSceneMgr->getRootSceneNode()->createChildSceneNode( (curCreature->name + "_node").c_str() );
				//node->setPosition(position/BLENDER_UNITS_PER_OGRE_UNIT);
				node->setPosition(curCreature->getPosition());
				//FIXME: Something needs to be done about the caling issue here.
				//node->setScale(1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT, 1.0/BLENDER_UNITS_PER_OGRE_UNIT);
				node->setScale(curCreature->scale);
				ent->setNormaliseNormals(true);
				node->attachObject(ent);
				break;

			case RenderRequest::setCreatureAnimationState:
				curCreature = (Creature*)curReq->p;
				ent = mSceneMgr->getEntity("Creature_" + curCreature->name);

				if(ent->hasSkeleton())
				{
					curCreature->animationState = ent->getAnimationState(curReq->str.c_str());
					curCreature->animationState->setLoop(true);
					curCreature->animationState->setEnabled(true);
				}
				break;

			case RenderRequest::destroyCreature:
				curCreature = (Creature*)curReq->p;
				if(mSceneMgr->hasEntity( ("Creature_" + curCreature->name).c_str() ))
				{
					ent = mSceneMgr->getEntity( ("Creature_" + curCreature->name).c_str() );
					node = mSceneMgr->getSceneNode( (curCreature->name + "_node").c_str() );
					mSceneMgr->getRootSceneNode()->removeChild( node );
					node->detachObject( ent );
					mSceneMgr->destroyEntity( ent );
					mSceneMgr->destroySceneNode( (curCreature->name + "_node") );
				}
				break;

			case RenderRequest::deleteCreature:
				curCreature = (Creature*)curReq->p;
				delete curCreature;
				break;

			case RenderRequest::noRequest:
				break;

			default:
				cout << "\n\n\nERROR: Unhandled render request!\n\n\n";
				break;
		}

		delete curReq;
		curReq = NULL;
	}
	
		string chatBaseString = "\n---------- Chat ----------\n";
		chatString = chatBaseString;

		// Only keep the N newest chat messages
		while(chatMessages.size() > 5)
		{
			delete chatMessages.front();
			chatMessages.pop_front();
		}

		for(unsigned int i = 0; i < chatMessages.size(); i++)
		{
			char tempArray[255];
			sprintf(tempArray, "%li: %s: %s", chatMessages[i]->recvTime, chatMessages[i]->clientNick.c_str(), chatMessages[i]->message.c_str());
			chatString += (string)tempArray + "\n";
		}

		string nullString = "";
		char turnArray[255];
		sprintf(turnArray, "Turn number:  %li", turnNumber);
		printText((string)MOTD + "\n" + (terminalActive?(commandOutput + "\n"):nullString) + (terminalActive?prompt:nullString) + (terminalActive?promptCommand:nullString) + "\n" + turnArray + "\n" + (chatMessages.size()>0?chatString:nullString));

		// Sleep to limit the framerate to the max value
		frameDelay -= evt.timeSinceLastFrame;
		if(frameDelay > 0.0)
		{
			usleep(1e6 * frameDelay );
		}
		else
		{
			//FIXME: I think this 2.0 should be a 1.0 but this gives the
			// correct result.  This probably indicates a bug.
			frameDelay += 2.0/(double)MAX_FRAMES_PER_SECOND;
		}

	if(mWindow->isClosed())	return false;

	// Update the animations on any creatures who have them
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		Creature *currentCreature = gameMap.getCreature(i);

		// Advance the animation
		if(currentCreature->animationState != NULL)
			currentCreature->animationState->addTime(evt.timeSinceLastFrame);

		// Move the creature
		if(currentCreature->walkQueue.size() > 0)
		{
			double moveDist = currentCreature->moveSpeed * evt.timeSinceLastFrame;
			currentCreature->shortDistance -= moveDist;

			if(currentCreature->shortDistance <= 0.0)
			{
				currentCreature->setPosition(currentCreature->walkQueue.front());
				currentCreature->walkQueue.pop_front();

				if(currentCreature->walkQueue.size() == 0)
				{
					currentCreature->setAnimationState("Idle");
				}
				else
				{
					SceneNode *node = mSceneMgr->getSceneNode(currentCreature->name + "_node");

					currentCreature->shortDistance = currentCreature->getPosition().distance(currentCreature->walkQueue.front());
					currentCreature->walkDirection = currentCreature->walkQueue.front() - currentCreature->getPosition();
					Ogre::Vector3 src = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;
					Quaternion quat = src.getRotationTo(currentCreature->walkDirection);
					node->rotate(quat);

				}
			}
			else
			{
				currentCreature->setPosition(currentCreature->getPosition() + currentCreature->walkDirection * moveDist);
			}
		}

	}

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();
	//if( mJoy ) mJoy->capture();

	bool buffJ = (mJoy) ? mJoy->buffered() : true;

	//Check if one of the devices is not buffered
	if( !mMouse->buffered() || !mKeyboard->buffered() )
	{
		// one of the input modes is immediate, so setup what is needed for immediate movement
		//if (mTimeUntilNextToggle >= 0)
			//mTimeUntilNextToggle -= evt.timeSinceLastFrame;

		// Move about 100 units per second
		//mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
		// Take about 10 seconds for full rotation
		//mRotScale = mRotateSpeed * evt.timeSinceLastFrame;

		mRotX = 0;
		mRotY = 0;
		mRotZ = 0;
		mTranslateVector = Ogre::Vector3::ZERO;
	}

	// Move about 100 units per second
	//mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
	// Take about 10 seconds for full rotation
	//mRotScale = mRotateSpeed * evt.timeSinceLastFrame;

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
	string  resultName;

	CEGUI::System::getSingleton().injectMouseMove(arg.state.X.rel, arg.state.Y.rel);

	// Mouse move code.  This does not work yet, moving the mouse to all four edges of the screen seems to make it work hence the if statement for the delay.
	/*
	if(turnNumber > 10)
	{
		unsigned int width, height, depth;
		int left, top;
		mWindow->getMetrics(width, height, depth, left, top);

		// It is odd that this must be done as a separate subtraction and then multiply but I think it has to be done this way.
		double tempX = width/2.0 - arg.state.X.abs;
		//tempX *= -1.0;
		double tempY = height/2.0 - arg.state.Y.abs;
		tempY *= -1.0;
		tempX /= width/2.0;
		tempY /= height/2.0;
		mMouseTranslateVector.x = tempX;
		mMouseTranslateVector.y = tempY;
		cout << mMouseTranslateVector.x << "\t" << mMouseTranslateVector.y << endl;
	}
	*/

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
					mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(xPos, yPos, 0);

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
								// See if we are hosting a game or not
								if(serverSocket == NULL)
								{
									tempTile->setSelected(true);
								}
								else
								{
									if(tempTile->getFullness() > 0)
									{
										tempTile->setMarkedForDigging(true);
									}
								}
							}
							else
							{
								// See if we are hosting a game or not
								if(serverSocket == NULL)
								{
									tempTile->setSelected(false);
								}
								else
								{
									tempTile->setMarkedForDigging(false);
								}
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
								// See if we are hosting a game or not
								if(serverSocket == NULL)
								{
									tempTile->setSelected(true);
								}
								else
								{
									tempTile->setMarkedForDigging(true);
								}
							}
							else
							{
								// See if we are hosting a game or not
								if(serverSocket == NULL)
								{
									tempTile->setSelected(false);
								}
								else
								{
									tempTile->setMarkedForDigging(false);
								}
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
				if(resultName.find("Level_") != string::npos)
				{
					sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

					mSceneMgr->getEntity("SquareSelector")->setVisible(true);
					mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(xPos, yPos, 0);
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

	// Left mouse button down
	if (id == OIS::MB_Left)
	{
		// See if the mouse is over any creatures
		while (itr != result.end() )
		{
			if(itr->movable != NULL)
			{
				resultName = itr->movable->getName();

				if(resultName.find("Creature_") != string::npos)
				{
					//Entity *resultEnt = mSceneMgr->getEntity(resultName);
					mSceneMgr->getEntity("SquareSelector")->setVisible(false);

					
					mDraggedCreature = resultName.substr(((string)"Creature_").size(), resultName.size());
					SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(mDraggedCreature	+ "_node");
					mSceneMgr->getRootSceneNode()->removeChild(creatureSceneNode);
					mSceneMgr->getSceneNode("Hand_Node")->addChild(creatureSceneNode);
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
		// See if we are hosting a game or not
		if(serverSocket == NULL)
		{
			gameMap.getTile(i)->setSelected(false);
		}
		else
		{
			gameMap.getTile(i)->setMarkedForDigging(false);
		}
	}

	// Left mouse button up
	if (id == OIS::MB_Left)
	{
		// Check to see if we are moving a creature
		if(mDragType == ExampleFrameListener::creature)
		{
			SceneNode *creatureSceneNode = mSceneMgr->getSceneNode(mDraggedCreature + "_node");
			mSceneMgr->getSceneNode("Hand_Node")->removeChild(creatureSceneNode);
			mSceneMgr->getRootSceneNode()->addChild(creatureSceneNode);
			mDragType = ExampleFrameListener::nullDragType;
			gameMap.getCreature(mDraggedCreature)->setPosition(xPos, yPos, 0);
		}

		// Check to see if we are dragging out a selection of tiles
		else if(mDragType == ExampleFrameListener::tileSelection)
		{
			for(int i = min(xPos, mLStartDragX); i <= max(xPos, mLStartDragX); i++)
			{
				for(int j = min(yPos, mLStartDragY); j <= max(yPos, mLStartDragY); j++)
				{
					// Make sure the tile exists before we set its value
					Tile *currentTile = gameMap.getTile(i, j);
					if(currentTile != NULL)
					{
						// See if we are hosting a game or not
						if(serverSocket == NULL)
						{
							currentTile->setType( mCurrentTileType );
							currentTile->setFullness( mCurrentFullness );
						}
						else
						{
							if(currentTile->getFullness() > 0)
							{
								currentTile->setMarkedForDigging(true);
							}
						}
					}
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
			default:
				break;

			case OIS::KC_GRAVE:
			case OIS::KC_F12:
				terminalActive = true;
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
				mRotateWorldVector.z += 1.3 * mRotateSpeed.valueDegrees();
				break;

			// Turn right
			case KC_PGDOWN:
				mRotateWorldVector.z += -1.3 * mRotateSpeed.valueDegrees();
				break;

			//Toggle mCurrentTileType
			case KC_R:
				mCurrentTileType = Tile::nextTileType(mCurrentTileType);
				sprintf(tempArray, "Tile type:  %s", Tile::tileTypeToString(mCurrentTileType).c_str());
				MOTD = tempArray;
				break;

			//Toggle mCurrentFullness
			case KC_T:
				mCurrentFullness = Tile::nextTileFullness(mCurrentFullness);
				sprintf(tempArray, "Tile fullness:  %i", mCurrentFullness);
				MOTD = tempArray;
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

			// Print a screenshot
			case KC_SYSRQ:
				std::ostringstream ss;
				ss << "screenshot_" << ++mNumScreenShots << ".png";
				mWindow->writeContentsToFile(ss.str());
				mTimeUntilNextToggle = 0.5;
				mDebugText = "Saved: " + ss.str();
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
			case OIS::KC_F12:
			case KC_ESCAPE:
				terminalActive = false;
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

						default:
							break;
					}
				}

				break;
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
			default:
				break;

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
				mRotateWorldVector.z -= 1.3 * mRotateSpeed.valueDegrees();
				break;

			// Turn right
			case KC_PGDOWN:
				mRotateWorldVector.z -= -1.3 * mRotateSpeed.valueDegrees();
				break;

		}
	}

	return true;
}

// Displays the given text on the screen starting in the upper-left corner.
// This is the function which displays the text on the in game console.
void ExampleFrameListener::printText(string text)
{
	string tempString;
	int lineLength = 0;
	for(unsigned int i = 0; i < text.size(); i++)
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
	unsigned int firstSpace, lastSpace;
	stringstream tempSS;

	commandOutput = "";
	command = "";
	arguments = "";

	// If the user just presses enter without entering a command we return to the game
	if(promptCommand.size() == 0)
	{
		promptCommand = "";
		terminalActive = false;

		return;
	}

	// Split the raw text into command and argument strings
	firstSpace = promptCommand.find(' ');
	if(firstSpace != string::npos)
	{
		command = promptCommand.substr(0, firstSpace);

		// Skip any extra spaces in between the command and the arguments
		lastSpace = firstSpace;
		while(lastSpace < promptCommand.size() && promptCommand[lastSpace] == ' ')
		{
			lastSpace++;
		}

		//FIXME: This if statement is a hack to prevent a crash, the code above needs
		// to be fixed but i don't see the problem with it.
		if(lastSpace < promptCommand.size())
		{
			arguments = promptCommand.substr(lastSpace, promptCommand.size()-lastSpace);
		}
		else
		{
			command = promptCommand;
		}
	}
	else
	{
		command = promptCommand;
	}

	// Force command to lower case
	for(unsigned int i = 0; i < command.size(); i++)
	{
		command[i] = tolower(command[i]);
	}

	// Begin Command Implementation
	//
	// All the code from here to the rest of the function is the implementation code
	// for specific commands which are handled by the terminal.

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
			char tempArray[255];

			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> tempR >> tempG >> tempB;
				mSceneMgr->setAmbientLight(ColourValue(tempR,tempG,tempB));
				sprintf(tempArray, "Abmbient light set to:\nRed:  %lf/1.0    Green:  %lf/1.0    Blue:  %lf/1.0", tempR, tempG, tempB);
				commandOutput = tempArray;

			}
			else
			{
				ColourValue curLight = mSceneMgr->getAmbientLight();
				sprintf(tempArray, "Current Ambient Light:\nRed:  %f/1.0    Green:  %f/1.0    Blue:  %f/1.0", curLight.r, curLight.g, curLight.b);
				commandOutput = tempArray;
			}
		}

		// Print the help message
		else if(command.compare("help") == 0)
		{
			if(arguments.size() > 0)
			{
				commandOutput = "Help for command:  " + arguments + "\n\n" + getHelpText(arguments);
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

		// A utility to set the camera rotation speed.
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

		// Set max frames per second
		else if(command.compare("fps") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				int tempInt;
				tempSS.str(arguments);
				tempSS >> tempInt;
				MAX_FRAMES_PER_SECOND = tempInt;
				
				sprintf(tempArray, "Maximum framerate set to %i", MAX_FRAMES_PER_SECOND);
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Current maximum framerate is %i", MAX_FRAMES_PER_SECOND);
				commandOutput = tempArray;
			}
		}

		// Set the turnsPerSecond variable to control the AI speed
		else if(command.compare("turnspersecond") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> turnsPerSecond;
				
				sprintf(tempArray, "The game will proceed at %lf turns per second.", turnsPerSecond);
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "The game is proceeding at %lf turns per second.", turnsPerSecond);
				commandOutput = tempArray;
			}
		}

		// Set near clip distance
		else if(command.compare("nearclip") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				double tempDouble;
				tempSS.str(arguments);
				tempSS >> tempDouble;
				mCamera->setNearClipDistance(tempDouble);
				
				sprintf(tempArray, "Near clip distance set to %lf", mCamera->getNearClipDistance());
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Near clip distance set to %lf", mCamera->getNearClipDistance());
				commandOutput = tempArray;
			}
		}

		// Set far clip distance
		else if(command.compare("farclip") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				double tempDouble;
				tempSS.str(arguments);
				tempSS >> tempDouble;
				mCamera->setFarClipDistance(tempDouble);
				
				sprintf(tempArray, "Far clip distance set to %lf", mCamera->getFarClipDistance());
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Far clip distance set to %lf", mCamera->getFarClipDistance());
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
				//node->setPosition(tempCreature->getPosition()/BLENDER_UNITS_PER_OGRE_UNIT);
				node->setPosition(tempCreature->getPosition());
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
				double tempX, tempY, tempZ, tempSightRadius, tempDigRate;
				int tempHP, tempMana;
				string tempString, tempString2;
				tempSS.str(arguments);
				tempSS >> tempString >> tempString2 >> tempX >> tempY >> tempZ >> tempHP >> tempMana >> tempSightRadius >> tempDigRate;
				Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ), tempHP, tempMana, tempSightRadius, tempDigRate);
				gameMap.addClassDescription(p);
			}

		}

		// Print out various lists of information, the creatures in the
		// scene, the levels available for loading, etc
		else if(command.compare("list") == 0 || command.compare("ls") == 0)
		{
			if(arguments.size() > 0)
			{
				string tempString;
				tempSS.str(tempString);

				if(arguments.compare("creatures") == 0)
				{
					tempSS << "Class:\tCreature name:\tLocation:\tColor:\n\n";
					for(int i = 0; i < gameMap.numCreatures(); i++)
					{
						tempSS << gameMap.getCreature(i);
					}
				}

				else if(arguments.compare("classes") == 0)
				{
					tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\n\n";
					for(int i = 0; i < gameMap.numClassDescriptions(); i++)
					{
						Creature *currentClassDesc = gameMap.getClassDescription(i);
						tempSS << currentClassDesc->className << "\t" << currentClassDesc->meshName << "\t";
						tempSS << currentClassDesc->scale << "\t" << currentClassDesc->hp << "\t";
						tempSS << currentClassDesc->mana << "\t" << currentClassDesc->sightRadius << "\t";
						tempSS << currentClassDesc->digRate << "\n";
					}
				}

				else if(arguments.compare("players") == 0)
				{
					tempSS << "Not implemented yet.";
				}

				else if(arguments.compare("network") == 0)
				{
					tempSS << "Not implemented yet.";
				}

				commandOutput = tempSS.str();
			}
			else
			{
				commandOutput = "lists available:\t\tclasses\tcreatures\tplayers\tnetwork";
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

		// Set your nickname
		else if(command.compare("nick") == 0)
		{
			char tempArray[255];
			if(arguments.size() > 0)
			{
				tempSS.str(arguments);
				tempSS >> me->nick;
				sprintf(tempArray, "Nickname set to %s", me->nick.c_str());
				commandOutput = tempArray;
			}
			else
			{
				sprintf(tempArray, "Current nickname is %s", me->nick.c_str());
				commandOutput = tempArray;
			}
		}

		// Connect to a server
		else if(command.compare("connect") == 0)
		{
			if(me->nick.size() > 0)
			{
				if(clientSocket == NULL)
				{
					if(arguments.size() > 0)
					{
						clientSocket = new Socket;

						if(!clientSocket->create())
						{
							commandOutput = "ERROR:  Could not create client socket!";
							goto ConnectEndLabel;
						}

						if(clientSocket->connect(arguments, PORT_NUMBER))
						{
							commandOutput = "Connection successful.";

							CSPStruct *csps = new CSPStruct;
							csps->nSocket = clientSocket;
							csps->nFrameListener = this;
							
							// Start a thread to talk to the server
							pthread_create(&clientThread, NULL, clientSocketProcessor, (void*) csps);
						}
						else
						{
							commandOutput = "Connection failed!";
						}
					}
					else
					{
						commandOutput = "You must specify the IP address of the server you want to connect to.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1";
					}

				}
			}
			else
			{
				commandOutput = "You must set a nick with the \"nick\" command before you can join a server.";
			}

		ConnectEndLabel:
			commandOutput += "\n";

		}

		// Host a server
		else if(command.compare("host") == 0)
		{
			if(me->nick.size() > 0)
			{
				if(serverSocket == NULL)
				{
					serverSocket = new Socket;

					// Start the server socket listener as well as the server socket thread
					if(serverSocket != NULL)
					{
						SSPStruct ssps;
						ssps.nSocket = serverSocket;
						ssps.nFrameListener = this;
						pthread_create(&serverThread, NULL, serverSocketProcessor, (void*) &ssps);
						commandOutput = "Server started successfully.";
					}
					else
					{
						commandOutput = "ERROR:  Could not start server!";
					}

					// Start the creature AI thread
					pthread_create(&creatureThread, NULL, creatureAIThread, NULL);
				}
			}
			else
			{
				commandOutput = "You must set a nick with the \"nick\" command before you can host a server.";
			}
		}

		// Send a chat message
		else if(command.compare("chat") == 0 || command.compare("c") == 0)
		{
			if(clientSocket != NULL)
			{
				clientSocket->send(formatCommand("chat", me->nick + ":" + arguments));
				chatMessages.push_back(new ChatMessage(me->nick, arguments, time(NULL), time(NULL)));
			}
			else if(serverSocket != NULL)
			{
				// Since the server keeps a socket for each client we
				// need to send the chat out the right connection.
				clientSockets[0]->send(formatCommand("chat", me->nick + ":" + arguments));
				chatMessages.push_back(new ChatMessage(me->nick, arguments, time(NULL), time(NULL)));
			}
		}

		else commandOutput = "Command not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.";

	promptCommand = "";
}


string ExampleFrameListener::getHelpText(string arg)
{
	for(unsigned int i = 0; i < arg.size(); i++)
	{
		arg[i] = tolower(arg[i]);
	}

	if(arg.compare("save") == 0)
	{
		return "Save the current level to a file.  The file name is given as an argument to the save command.\n\nExample:\n" + prompt + "save Test\n\nThe above command will save the level to Media/levels/Test.level.  The Test level is loaded automatically when OpenDungeons starts.";
	}

	else if(arg.compare("load") == 0)
	{
		return "Load a level from a file.  The new level replaces the current level.  The levels are stored in the Media/levels/ directory and have a .level extension on the end.  Both the directory and the .level extension are automatically applied for you.\n\nExample:\n" + prompt + "load Level1\n\nThe above command will load the file Level1.level from the Media/levels directory.";
	}

	else if(arg.compare("addclass") == 0)
	{
		return "Add a new class decription to the current map.  Because it is common to load many creatures of the same type creatures are given a class which stores their common information such as the mesh to load, scaling, etc.  Addclass defines a new class of creature, allowing creatures of this class to be loaded in the future.  The argument to addclass is interpreted in the same was as a class description line in the .level file format.\n\nExample:\n" + prompt + "addclass Skeleton Skeleton.mesh 0.01 0.01 0.01\n\nThe above command defines the class \"Skeleton\" which uses the mesh file \"Skeleton.mesh\" and has a scale factor of 0.01 in the X, Y, and Z dimensions.";
	}

	else if(arg.compare("addcreature") == 0)
	{
		return "Add a new creature to the current map.  The creature class to be used must be loaded first, either from the loaded map file or by using the addclass command.  Once a class has been declared a creature can be loaded using that class.  The argument to the addcreature command is interpreted in the same way as a creature line in a .level file.\n\nExample:\n" + prompt + "addcreature Skeleton Bob 10 15 0\n\nThe above command adds a creature of class \"Skeleton\" whose name is \"Bob\" at location X=10, y=15, and Z=0.  The new creature's name must be unique to the creatures in that level.  Alternatively the name can be se to \"autoname\" to have OpenDungeons assign a unique name.";
	}

	else if(arg.compare("quit") == 0)
	{
		return "Exits OpenDungeons";
	}

	else if(arg.compare("termwidth") == 0)
	{
		return "The termwidth program sets the maximum number of characters that can be displayed on the terminal without word wrapping taking place.  When run with no arguments, termwidth displays a ruler across the top of you terminal indicating the terminal's current width.  When run with an argument, termwidth sets the terminal width to a new value specified in the argument.\n\nExample:\n" + prompt + "termwidth 80\n\nThe above command sets the terminal width to 80.";
	}

	else if(arg.compare("addtiles") == 0)
	{
		return "The addtiles command adds a rectangular region of tiles to the map.  The tiles are initialized to a fullness of 100 and have their type set to dirt.  The region to be added is given as two pairs of X-Y coordinates.\n\nExample:\n" + prompt + "addtiles -10 -5 34 20\n\nThe above command adds the tiles in the given region to the map.  Tiles which overlap already existing tiles will be ignored.";
	}

	else if(arg.compare("newmap") == 0)
	{
		return "Replaces the existing map with a new rectangular map.  The X and Y dimensions of the new map are given as arguments to the newmap command.\n\nExample:\n" + prompt + "newmap 10 20\n\nThe above command creates a new map 10 tiles by 20 tiles.  The new map will be filled with dirt tiles with a fullness of 100.";
	}

	else if(arg.compare("movespeed") == 0)
	{
		return "The movespeed command sets how fast the camera moves at.  When run with no argument movespeed simply prints out the current camera move speed.  With an argument movespeed sets the camera move speed.\n\nExample:\n" + prompt + "movespeed 3.7\n\nThe above command sets the camera move speed to 3.7.";
	}

	else if(arg.compare("rotatespeed") == 0)
	{
		return "The rotatespeed command sets how fast the camera rotates.  When run with no argument rotatespeed simply prints out the current camera rotation speed.  With an argument rotatespeed sets the camera rotation speed.\n\nExample:\n" + prompt + "rotatespeed 35\n\nThe above command sets the camera rotation speed to 35.";
	}

	else if(arg.compare("ambientlight") == 0)
	{
		return "The ambientlight command sets the minumum light that every object in the scene is illuminated with.  It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n" + prompt + "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.";
	}

	else if(arg.compare("host") == 0)
	{
		//FIXME:  add some code to automatically include the default port number in this help file.
		return "Starts a server thread running on this machine.  This utility takes a port number as an argument.  The port number is the port to listen on for a connection.  The default (if no argument is given) is to use 31222 for the port number.";
	}

	else if(arg.compare("connnect") == 0)
	{
		return "Connect establishes a connection with a server.  It takes as its argument an IP address specified in dotted decimal notation (such as 192.168.1.100), and starts a client thread which monitors the connection for events.";
	}

	else if(arg.compare("chat") == 0 || arg.compare("c") == 0)
	{
		return "Chat (or \"c\" for short) is a utility to send messages to other players participating in the same game.  The argument to chat is broadcast to all members of the game, along with the nick of the person who sent the chat message.  When a chat message is recieved it is added to the chat buffer along with a timestamp indicating when it was recieved.\n\nThe chat buffer displays the last n chat messages recieved.  The number of displayed messages can be set with the \"chathist\" command.  Displayed chat messages will also be removed from the chat buffer after they age beyond a certain point.";
	}

	else if(arg.compare("list") == 0 || arg.compare("ls") == 0)
	{
		return "List (or \"ls\" for short is a utility which lists various types of information about the current game.  Running list without an argument will produce a list of the lists available.  Running list with an argument displays the contents of that list.\n\nExample:\n" + prompt + "list creatures\n\nThe above command will produce a list of all the creatures currently in the game.";
	}

	return "Help for command:  \"" + arguments + "\" not found.";
}

