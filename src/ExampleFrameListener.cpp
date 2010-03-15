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
#include "Sleep.h"
#include "Field.h"

using namespace Ogre;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf _snprintf
#endif

/*! \brief A required function to pass input to the OIS system.
 *
 */
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

/*! \brief This constructor is where the OGRE system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ExampleFrameListener::ExampleFrameListener(RenderWindow* win, Camera* cam, SceneManager *sceneManager, CEGUI::Renderer *renderer, bool bufferedKeys, bool bufferedMouse, bool bufferedJoy)
	: mCamera(cam), mTranslateVector(Ogre::Vector3::ZERO), mWindow(win)
{
	mCount = 0;
	mCurrentObject = NULL;
	mLMouseDown = false;
	mRMouseDown = false;
	mSceneMgr = sceneManager;
	terminalActive = false;
	prompt = "-->  ";
	terminalWordWrap = 78;
	gameMap.me = new Player;
	gameMap.me->nick = "";
	mDragType = ExampleFrameListener::nullDragType;
	newRoomType = Room::quarters;
	frameDelay = 0.0;
	mGUIRenderer = renderer;
	zChange = 0.0;
	mCurrentTileRadius = 1;
	mBrushMode = false;
	addRoomsMode = false;
	creatureSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("Creature_scene_node");
	roomSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("Room_scene_node");
	fieldSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("Field_scene_node");
	lightSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("Light_scene_node");

	mStatsOn = false;
	mNumScreenShots = 0;
	mMoveScale = 0.0f;
	mRotScale = 0.0f;
	mTimeUntilNextToggle = 0;
	mFiltering = TFO_BILINEAR;
	mAniso = 1;
	mSceneDetailIndex = 0;
	mMoveSpeed = 50.0;
	mRotateSpeed = 50;
	mDebugOverlay = 0;
	mInputManager = 0;
	mMouse = 0;
	mKeyboard = 0;
	mJoy = 0;
	mZoomSpeed = .5;
	mCurrentTileType = Tile::dirt;
	mCurrentFullness = 100;
	ceguiHasControl = false;
	ignoreOneMouseReleased = false;

	using namespace OIS;

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

	//Register as a Window listener
	WindowEventUtilities::addWindowEventListener(mWindow, this);

	mCamNode = cam->getParentSceneNode();

	mContinue = true;
	mMouse->setEventCallback(this);
	mKeyboard->setEventCallback(this);

	mRaySceneQuery = mSceneMgr->createRayQuery(Ray());
}

/*! \brief Adjust mouse clipping area
 *
 */
void ExampleFrameListener::windowResized(RenderWindow* rw)
{
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

/*! \brief Unattach OIS before window shutdown (very important under Linux)
 *
 */
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

/*! \brief Sets the camera to a new location while still satisfying the constraints placed on its movement
 *
 */
void ExampleFrameListener::moveCamera(double frameTime)
{
	// Record the camera's position, move it, the re-record the position
	// for constraint calculations
	Ogre::Vector3 tempVector = mCamNode->getPosition();
	mCamNode->translate(mTranslateVector * frameTime, Node::TS_LOCAL);
	Ogre::Vector3 tempVector2 = mCamNode->getPosition();

	// Apply the valid motion into tempVector2
	tempVector2.z = tempVector.z + zChange*frameTime*mZoomSpeed;
	double horizontalSpeedFactor = (tempVector2.z >= 25.0) ? 1.0 : tempVector2.z/(25.0);
	tempVector2.x = tempVector.x + (mMouseTranslateVector.x + (tempVector2.x - tempVector.x)) * horizontalSpeedFactor;
	tempVector2.y = tempVector.y + (mMouseTranslateVector.y + (tempVector2.y - tempVector.y)) * horizontalSpeedFactor;
	
	// Prevent camera from moving down into the tiles
	if(tempVector2.z <= 4.5)
		tempVector2.z = 4.5;

	// Move the camera to the new location
	mCamNode->setPosition(tempVector2);

	// Rotate the camera
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
	else
	{
		if(show)
		{
			mDebugOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");
		}
	}
}

/*! \brief The main rendering function for the OGRE 3d environment.
 *
 * This function is the one which actually carries out all of the rendering in
 * the OGRE 3d system.  Since all the rendering must happen here, one of the
 * operations performed by this function is the processing of a request queue
 * full of RenderRequest structures.
 */
bool ExampleFrameListener::frameStarted(const FrameEvent& evt)
{
	stringstream tempSS;

	if(mWindow->isClosed())	return false;

	using namespace OIS;

	// Process the queue of render tasks from the other threads
	while(renderQueue.size() > 0)
	{
		char meshName[255];
		string tempString, tempString2;
		string tileTypeString;
		Entity *ent, *weaponEntity;
		SceneNode *node, *node2;
		Ogre::Matrix3 boneRot;
		Ogre::Vector3 tempVector;
		Quaternion tempQuaternion;
		MaterialPtr tempMaterial;
		Tile *curTile = NULL;
		Room *curRoom = NULL;
		Creature *curCreature = NULL;
		MapLight *curMapLight = NULL;
		Light *light;
		Weapon *curWeapon = NULL;
		Bone *weaponBone;
		string boneString;
		Player *curPlayer = NULL;
		Field *curField = NULL;
		FieldType::iterator fieldItr;
		int tempX, tempY;
		double tempDouble, tempDouble2;

		// Remove the first item from the render queue
		sem_wait(&renderQueueSemaphore);
		RenderRequest *curReq = renderQueue.front();
		renderQueue.pop_front();
		sem_post(&renderQueueSemaphore);

		// Switch based on the type of render request we are processing
		switch(curReq->type)
		{
			case RenderRequest::refreshTile:
				curTile = (Tile*)curReq->p;
				if(mSceneMgr->hasSceneNode(curTile->name + "_node"))
				{
					// Unlink and delete the old mesh
					mSceneMgr->getSceneNode(curTile->name + "_node")->detachObject(curTile->name);
					mSceneMgr->destroyEntity(curTile->name);

					// Create the new mesh
					string tileTypeString = Tile::tileTypeToString(curTile->getType());
					snprintf(meshName, sizeof(meshName), "%s%i.mesh", tileTypeString.c_str(), curTile->getFullnessMeshNumber());
					ent = mSceneMgr->createEntity(curTile->name, meshName);

					// Link the tile mesh back to the relevant scene node so OGRE will render it
					node = mSceneMgr->getSceneNode(curTile->name + "_node");
					node->attachObject(ent);
					node->resetOrientation();
					node->roll(Degree(curTile->rotation));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
					ent->setNormaliseNormals(true);
#endif
				}
				break;

			case RenderRequest::createTile:
				curTile = (Tile*)curReq->p;
				tileTypeString = Tile::tileTypeToString(curTile->getType());

				snprintf(meshName, sizeof(meshName), "%s%i.mesh", tileTypeString.c_str(), curTile->getFullnessMeshNumber());
				ent = mSceneMgr->createEntity(curTile->name, meshName);

				node = mSceneMgr->getRootSceneNode()->createChildSceneNode(curTile->name + "_node");
				//node->setPosition(Ogre::Vector3(x/BLENDER_UNITS_PER_OGRE_UNIT, y/BLENDER_UNITS_PER_OGRE_UNIT, 0));
				node->attachObject(ent);
				node->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0));
				node->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
				node->resetOrientation();
				node->roll(Degree(curTile->rotation));

#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				ent->setNormaliseNormals(true);
#endif
				break;

			case RenderRequest::destroyTile:
				curTile = (Tile*)curReq->p;
				if(mSceneMgr->hasEntity(curTile->name))
				{
					ent = mSceneMgr->getEntity(curTile->name);
					node = mSceneMgr->getSceneNode(curTile->name + "_node");
					node->detachAllObjects();
					mSceneMgr->destroySceneNode(curTile->name + "_node");
					mSceneMgr->destroyEntity(ent);
				}
				break;

			case RenderRequest::createRoom:
				curRoom = (Room*)curReq->p;
				curTile = (Tile*)curReq->p2;

				tempString = "";
				tempSS.str(tempString);
				tempSS << curRoom->name << "_" << curTile->x << "_" << curTile->y;
				ent = mSceneMgr->createEntity(tempSS.str(), curRoom->meshName + ".mesh");
				//colourizeEntity(ent, curRoom->color);
				node = roomSceneNode->createChildSceneNode(tempSS.str() + "_node");
				node->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0.0));
				node->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				ent->setNormaliseNormals(true);
#endif
				node->attachObject(ent);
				sem_post(&curRoom->meshCreationFinishedSemaphore);
				break;

			case RenderRequest::destroyRoom:
				curRoom = (Room*)curReq->p;
				curTile = (Tile*)curReq->p2;

				tempString = "";
				tempSS.str(tempString);
				tempSS << curRoom->name << "_" << curTile->x << "_" << curTile->y;
				if(mSceneMgr->hasEntity(tempSS.str()))
				{
					ent = mSceneMgr->getEntity(tempSS.str());
					node = mSceneMgr->getSceneNode(tempSS.str() + "_node");
					node->detachObject(ent);
					roomSceneNode->removeChild(node);
					mSceneMgr->destroyEntity(ent);
					mSceneMgr->destroySceneNode(tempSS.str() + "_node");
				}
				sem_post(&curRoom->meshDestructionFinishedSemaphore);
				break;

			case RenderRequest::deleteRoom:
				curRoom = (Room*)curReq->p;
				delete curRoom;
				break;

			case RenderRequest::deleteTile:
				curTile = (Tile*)curReq->p;
				delete curTile;
				break;

			case RenderRequest::createCreature:
				curCreature = (Creature*)curReq->p;

				// Load the mesh for the creature
				ent = mSceneMgr->createEntity("Creature_" + curCreature->name, curCreature->meshName);
				//colourizeEntity(ent, curCreature->color);
				node = creatureSceneNode->createChildSceneNode(curCreature->name + "_node");
				curCreature->sceneNode = node;
				node->setPosition(curCreature->getPosition());
				node->setScale(curCreature->scale);
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				ent->setNormaliseNormals(true);
#endif

				node->attachObject(ent);
				sem_post(&curCreature->meshCreationFinishedSemaphore);
				break;

			case RenderRequest::destroyCreature:
				curCreature = (Creature*)curReq->p;
				if(mSceneMgr->hasEntity("Creature_" + curCreature->name))
				{
					ent = mSceneMgr->getEntity("Creature_" + curCreature->name);
					node = mSceneMgr->getSceneNode(curCreature->name + "_node");
					node->detachObject(ent);
					creatureSceneNode->removeChild(node);
					mSceneMgr->destroyEntity(ent);
					mSceneMgr->destroySceneNode(curCreature->name + "_node");
				}
				curCreature->sceneNode = NULL;
				sem_post(&curCreature->meshDestructionFinishedSemaphore);
				break;

			case RenderRequest::reorientSceneNode:
				node = (SceneNode*)curReq->p;
				tempQuaternion = curReq->quaternion;

				if(node != NULL)
					node->rotate(tempQuaternion);

				break;

			case RenderRequest::scaleSceneNode:
				node = (SceneNode*)curReq->p;
				tempVector = curReq->vec;

				if(node != NULL)
					node->scale(tempVector);

				break;

			case RenderRequest::createWeapon:
				curWeapon = (Weapon*)curReq->p;
				curCreature = (Creature*)curReq->p2;

				ent = mSceneMgr->getEntity("Creature_" + curCreature->name);
				weaponEntity = mSceneMgr->createEntity("Weapon_" + curWeapon->handString + "_" + curCreature->name, curWeapon->meshName);
				boneString = (string)"Weapon_" + curWeapon->handString;
				weaponBone = ent->getSkeleton()->getBone(boneString);
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				weaponEntity->setNormaliseNormals(true);
#endif

				// Rotate by -90 degrees around the x-axis from the bone's rotation.
				tempQuaternion.FromAngleAxis(Degree(-90.0), Ogre::Vector3(1.0, 0.0, 0.0));

				ent->attachObjectToBone(weaponBone->getName(), weaponEntity, tempQuaternion);
				sem_post(&curWeapon->meshCreationFinishedSemaphore);
				break;

			case RenderRequest::destroyWeapon:
				curWeapon = (Weapon*)curReq->p;
				curCreature = (Creature*)curReq->p2;

				if(curWeapon->name.compare("none") != 0)
				{
					ent = mSceneMgr->getEntity("Weapon_" + curWeapon->handString + "_" + curCreature->name);
					mSceneMgr->destroyEntity(ent);
				}
				sem_post(&curWeapon->meshDestructionFinishedSemaphore);
				break;

			case RenderRequest::createMapLight:
				curMapLight = (MapLight*)curReq->p;

				// Create the light and attach it to the lightSceneNode.
				tempString = (string)"MapLight_" + curMapLight->getName();
				light = mSceneMgr->createLight(tempString);
				light->setDiffuseColour(curMapLight->getDiffuseColor());
				light->setSpecularColour(curMapLight->getSpecularColor());
				light->setAttenuation(curMapLight->getAttenuationRange(), curMapLight->getAttenuationConstant(),
						curMapLight->getAttenuationLinear(), curMapLight->getAttenuationQuadratic());

				// Create the base node that the "flicker_node" and the mesh attach to.
				node = lightSceneNode->createChildSceneNode(tempString + "_node");
				node->setPosition(curMapLight->getPosition());

				if(serverSocket == NULL && clientSocket == NULL)
				{
					// Create the MapLightIndicator mesh so the light can be drug around in the map editor.
					tempString2 = (string)"MapLightIndicator_" + curMapLight->getName();
					ent = mSceneMgr->createEntity(tempString2, "Light.mesh");
					node->attachObject(ent);
				}

				// Create the "flicker_node" which moves around randomly relative to
				// the base node.  This node carries the light itself.
				node2 = node->createChildSceneNode(tempString + "_flicker_node");
				node2->attachObject(light);
				//TODO: Post a creation finished semaphore for the light if necessary.
				break;

			case RenderRequest::destroyMapLight:
				curMapLight = (MapLight*)curReq->p;
				tempString = (string)"MapLight_" + curMapLight->getName();
				if(mSceneMgr->hasLight(tempString))
				{
					light = mSceneMgr->getLight(tempString);
					node = mSceneMgr->getSceneNode(tempString + "_node");
					node2 = mSceneMgr->getSceneNode(tempString + "_flicker_node");
					node2->detachObject(light);
					lightSceneNode->removeChild(node);
					mSceneMgr->destroyLight(light);
					tempString2 = (string)"MapLightIndicator_" + curMapLight->getName();
					if(mSceneMgr->hasEntity(tempString))
					{
						ent = mSceneMgr->getEntity(tempString2);
						node->detachObject(ent);
					}
					mSceneMgr->destroySceneNode(node2->getName());
					mSceneMgr->destroySceneNode(node->getName());
				}
				break;

			case RenderRequest::destroyMapLightVisualIndicator:
				curMapLight = (MapLight*)curReq->p;
				tempString = (string)"MapLight_" + curMapLight->getName();
				if(mSceneMgr->hasLight(tempString))
				{
					node = mSceneMgr->getSceneNode(tempString + "_node");
					tempString = (string)"MapLightIndicator_" + curMapLight->getName();
					if(mSceneMgr->hasEntity(tempString))
					{
						ent = mSceneMgr->getEntity(tempString);
						node->detachObject(ent);
					}
				}
				break;

			case RenderRequest::deleteMapLight:
				curMapLight = (MapLight*)curReq->p;
				delete curMapLight;
				break;

			case RenderRequest::createField:
				curField = (Field*)curReq->p;
				tempDouble = *(double*)curReq->p2;
				delete (double*)curReq->p2;

				fieldItr = curField->begin();
				while(fieldItr != curField->end())
				{
					tempX = fieldItr->first.first;
					tempY = fieldItr->first.second;
					tempDouble2 = fieldItr->second;
					//cout << "\ncreating field tile:  " << tempX << "\t" << tempY << "\t" << tempDouble;
					tempString = "";
					tempSS.str(tempString);
					tempSS << "Field_" << curField->name << "_" << tempX << "_" << tempY;
					ent = mSceneMgr->createEntity(tempSS.str(), "Field_indicator.mesh");
					node = fieldSceneNode->createChildSceneNode(tempSS.str() + "_node");
					node->setPosition(tempX, tempY, tempDouble + tempDouble2);
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
					ent->setNormaliseNormals(true);
#endif
					node->attachObject(ent);

					fieldItr++;
				}
				break;

			case RenderRequest::refreshField:
				curField = (Field*)curReq->p;
				tempDouble = *(double*)curReq->p2;
				delete (double*)curReq->p2;

				// Update existing meshes and create any new ones needed.
				fieldItr = curField->begin();
				while(fieldItr != curField->end())
				{
					tempString = "";
					tempSS.str(tempString);
					tempSS << "Field_" << curField->name << "_" << tempX << "_" << tempY;

					tempX = fieldItr->first.first;
					tempY = fieldItr->first.second;
					tempDouble2 = fieldItr->second;

					if(mSceneMgr->hasEntity(tempSS.str()))
					{
						// The mesh alread exists, just get the existing one
						node = mSceneMgr->getSceneNode(tempSS.str() + "_node");
					}
					else
					{
						// The mesh does not exist, create a new one
						ent = mSceneMgr->createEntity(tempSS.str(), "Field_indicator.mesh");
						node = fieldSceneNode->createChildSceneNode(tempSS.str() + "_node");
						node->attachObject(ent);
					}

					node->setPosition(tempX, tempY, tempDouble + tempDouble2);
					fieldItr++;
				}

				//TODO:  This is not done yet.
				// Delete any meshes not in the field currently
				break;

			case RenderRequest::destroyField:
				break;

			case RenderRequest::pickUpCreature:
				curCreature = (Creature*)curReq->p;
				// Detach the creature from the creature scene node
				node = mSceneMgr->getSceneNode(curCreature->name + "_node");
				creatureSceneNode->removeChild(node);

				// Attatch the creature to the hand scene node
				mSceneMgr->getSceneNode("Hand_node")->addChild(node);
				node->scale(0.333, 0.333, 0.333);

				// Move the other creatures in the player's hand to make room for the one just picked up.
				for(unsigned int i = 0; i < gameMap.me->numCreaturesInHand(); i++)
				{
					curCreature = gameMap.me->getCreatureInHand(i);
					node = mSceneMgr->getSceneNode(curCreature->name + "_node");
					node->setPosition(i%6 + 1, (i/(int)6), 0.0);
				}
				break;

			case RenderRequest::dropCreature:
				curCreature = (Creature*)curReq->p;
				curPlayer = (Player*)curReq->p2;
				// Detach the creature from the "hand" scene node
				node = mSceneMgr->getSceneNode(curCreature->name + "_node");
				mSceneMgr->getSceneNode("Hand_node")->removeChild(node);

				// Attach the creature from the creature scene node
				creatureSceneNode->addChild(node);
				node->setPosition(curCreature->getPosition());
				node->scale(3.0, 3.0, 3.0);

				// Move the other creatures in the player's hand to replace the dropped one
				for(unsigned int i = 0; i < curPlayer->numCreaturesInHand(); i++)
				{
					curCreature = curPlayer->getCreatureInHand(i);
					node = mSceneMgr->getSceneNode(curCreature->name + "_node");
					node->setPosition(i%6 + 1, (i/(int)6), 0.0);
				}
				break;

			case RenderRequest::rotateCreaturesInHand:
				// Loop over the creatures in our hand and redraw each of them in their new location.
				for(unsigned int i = 0; i < gameMap.me->numCreaturesInHand(); i++)
				{
					curCreature = gameMap.me->getCreatureInHand(i);
					node = mSceneMgr->getSceneNode(curCreature->name + "_node");
					node->setPosition(i%6 + 1, (i/(int)6), 0.0);
				}
				break;

			case RenderRequest::createCreatureVisualDebug:
				curTile = (Tile*)curReq->p;
				curCreature = (Creature*)curReq->p2;

				if(curTile != NULL && curCreature != NULL)
				{
					tempString = "";
					tempSS.str(tempString);
					tempSS << "Vision_indicator_" << curCreature->name << "_" << curTile->x << "_" << curTile->y;

					ent = mSceneMgr->createEntity( tempSS.str(), "Cre_vision_indicator.mesh");
					node = creatureSceneNode->createChildSceneNode( tempSS.str() + "_node" );
					node->attachObject(ent);
					node->setPosition(Ogre::Vector3(curTile->x, curTile->y, 0));
					node->setScale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
					ent->setNormaliseNormals(true);
#endif
				}
				break;

			case RenderRequest::destroyCreatureVisualDebug:
				curTile = (Tile*)curReq->p;
				curCreature = (Creature*)curReq->p2;

				tempString = "";
				tempSS.str(tempString);
				tempSS << "Vision_indicator_" << curCreature->name << "_" << curTile->x << "_" << curTile->y; 
				if(mSceneMgr->hasEntity(tempSS.str()))
				{
					ent = mSceneMgr->getEntity(tempSS.str());
					node = mSceneMgr->getSceneNode(tempSS.str() + "_node");

					node->detachAllObjects();
					mSceneMgr->destroyEntity(ent);
					mSceneMgr->destroySceneNode(tempSS.str() + "_node");
				}
				break;

			case RenderRequest::setCreatureAnimationState:
				curCreature = (Creature*)curReq->p;
				ent = mSceneMgr->getEntity("Creature_" + curCreature->name);

				if(ent->hasSkeleton() && ent->getSkeleton()->hasAnimation(curReq->str))
				{
					curCreature->animationState = ent->getAnimationState(curReq->str);
					curCreature->animationState->setLoop(true);
					curCreature->animationState->setEnabled(true);
				}
				break;

			case RenderRequest::deleteCreature:
				curCreature = (Creature*)curReq->p;
				delete curCreature;
				break;

			case RenderRequest::moveSceneNode:
				node = mSceneMgr->getSceneNode(curReq->str);
				node->setPosition(curReq->vec);
				break;

			case RenderRequest::noRequest:
				break;

			default:
				cerr << "\n\n\nERROR: Unhandled render request!\n\n\n";
				break;
		}

		delete curReq;
		curReq = NULL;
	}

	string chatBaseString = "\n---------- Chat ----------\n";
	chatString = chatBaseString;

	// Delete any chat messages older than the maximum allowable age
	//TODO:  Lock this queue before doing this stuff
	time_t now;
	time(&now);
	ChatMessage *currentMessage;
	unsigned int i = 0;
	while(i < chatMessages.size())
	{
		deque< ChatMessage* >::iterator itr;
		itr = chatMessages.begin() + i;
		currentMessage = *itr;
		if(difftime(now, currentMessage->recvTime) > 20.0)
		{
			chatMessages.erase(itr);
		}
		else
		{
			i++;
		}
	}

	// Only keep the N newest chat messages of the ones that remain
	while(chatMessages.size() > 10)
	{
		delete chatMessages.front();
		chatMessages.pop_front();
	}

	// Fill up the chat window with the arrival time and contents of all the chat messages left in the queue.
	for(unsigned int i = 0; i < chatMessages.size(); i++)
	{
		struct tm *friendlyTime = localtime(&chatMessages[i]->recvTime);
		string tempString = "";
		stringstream tempSS(tempString);
		tempSS << friendlyTime->tm_hour << ":" << friendlyTime->tm_min << ":" << friendlyTime->tm_sec << "  " << chatMessages[i]->clientNick << ":  " << chatMessages[i]->message;
		chatString += tempSS.str() + "\n";
	}

	// Display the terminal, the current turn number, and the
	// visible chat messages at the top of the screen
	string nullString = "";
	string turnString = "Average AI leftover time:  " + StringConverter::toString((Ogre::Real)fabs(gameMap.averageAILeftoverTime)).substr(0, 4) + " s ";
	turnString += (gameMap.averageAILeftoverTime >= 0.0 ? "early" : "late");
	turnString += "\nTurn number:  " + StringConverter::toString(turnNumber);
	printText((string)MOTD + "\n" + (terminalActive?(commandOutput + "\n"):nullString) + (terminalActive?prompt:nullString) + (terminalActive?promptCommand:nullString) + "\n" + turnString + "\n" + (chatMessages.size()>0?chatString:nullString));

	// Update the animations on any creatures who have them
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		Creature *currentCreature = gameMap.getCreature(i);

		// Advance the animation
		if(currentCreature->animationState != NULL)
			currentCreature->animationState->addTime(turnsPerSecond * evt.timeSinceLastFrame * currentCreature->moveSpeed);

		// Move the creature
		if(currentCreature->walkQueue.size() > 0)
		{
			//FIXME: The moveDist should probably be tied to the scale of the creature as well
			//FIXME: When the client and the server are using different frame rates, the creatures walk at different speeds
			double moveDist = turnsPerSecond * currentCreature->moveSpeed * evt.timeSinceLastFrame;
			currentCreature->shortDistance -= moveDist;

			// Check to see if we have walked to, or past, the first destination in the queue
			if(currentCreature->shortDistance <= 0.0)
			{
				// Compensate for any overshoot and place the creature at the intended destination
				currentCreature->setPosition(currentCreature->walkQueue.front());
				currentCreature->walkQueue.pop_front();

				// If there are no more places to walk to still left in the queue
				if(currentCreature->walkQueue.size() == 0)
				{
					// Stop walking
					currentCreature->stopWalking();
					currentCreature->setAnimationState(currentCreature->destinationAnimationState);
				}
				else // There are still entries left in the queue
				{
					SceneNode *node = mSceneMgr->getSceneNode(currentCreature->name + "_node");

					// Turn to face the next direction
					currentCreature->walkDirection = currentCreature->walkQueue.front() - currentCreature->getPosition();
					currentCreature->shortDistance = currentCreature->walkDirection.normalise();

					Ogre::Vector3 src = node->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

					// Work around 180 degree quaternion rotation quirk
					if ((1.0f + src.dotProduct(currentCreature->walkDirection)) < 0.0001f)
					{
						node->roll(Degree(180));
					}
					else
					{
						Quaternion quat = src.getRotationTo(currentCreature->walkDirection);
						node->rotate(quat);
					}
				}
			}
			else // We have not reached the destination at the front of the queue
			{
				currentCreature->setPosition(currentCreature->getPosition() + currentCreature->walkDirection * moveDist);
			}
		}

	}

	// Advance the "flickering" of the lights by the amount of time that has passed since the last frame.
	for(unsigned int i = 0; i < gameMap.numMapLights(); i++)
	{
	        MapLight *tempMapLight = gameMap.getMapLight(i);
	        tempMapLight->advanceFlicker(evt.timeSinceLastFrame);
	}

	// Update the CEGUI displays of gold, mana, etc.
	if(serverSocket != NULL || clientSocket != NULL)
	{
		Seat *mySeat = gameMap.me->seat;

		CEGUI::WindowManager *windowManager = CEGUI::WindowManager::getSingletonPtr();

		CEGUI::Window *tempWindow = windowManager->getWindow((CEGUI::utf8*)"Root/TerritoryDisplay");
		tempSS.str("");
		tempSS << "Territory\n" << gameMap.me->seat->numClaimedTiles;
		tempWindow->setText(tempSS.str());

		tempWindow = windowManager->getWindow((CEGUI::utf8*)"Root/GoldDisplay");
		tempSS.str("");
		tempSS << "Gold\n" << gameMap.me->seat->gold;
		tempWindow->setText(tempSS.str());

		tempWindow = windowManager->getWindow((CEGUI::utf8*)"Root/ManaDisplay");
		tempSS.str("");
		tempSS << "Mana\n" << mySeat->mana << "\n" << (mySeat->manaDelta >= 0 ? "+" : "-") << mySeat->manaDelta;
		tempWindow->setText(tempSS.str());

		// Update the goals display in the message window.
		tempWindow = windowManager->getWindow((CEGUI::utf8*)"Root/MessagesDisplayWindow");
		tempSS.str("");
		if(serverSocket != NULL || clientSocket != NULL)
		{
			bool iAmAWinner = gameMap.seatIsAWinner(gameMap.me->seat);

			if(!iAmAWinner)
			{
				// Loop over the list of unmet goals for the seat we are sitting in an print them.
				tempSS << "Unfinished Goals:\n----------\t-----------\n";
				for(unsigned int i = 0; i < gameMap.me->seat->numGoals(); i++)
				{
					Goal *tempGoal = gameMap.me->seat->getGoal(i);
					tempSS << tempGoal->getName() << ":\t" << tempGoal->getDescription() << "\n";
				}
			}

			// Loop over the list of completed goals for the seat we are sitting in an print them.
			tempSS << "\n\nCompleted Goals:\n----------\t-----------\n";
			for(unsigned int i = 0; i < gameMap.me->seat->numCompletedGoals(); i++)
			{
				Goal *tempGoal = gameMap.me->seat->getCompletedGoal(i);
				tempSS << tempGoal->getName() << ":\t" << tempGoal->getSuccessMessage() << "\n";
			}

			if(iAmAWinner)
			{
				tempSS << "\nCongratulations, you have completed this level.\nOpen the terminal and run the \'next\'\n";
				tempSS << "command to move on to move on to the next level.\n\nThe next level is:  " << gameMap.nextLevel;
			}
		}
		tempWindow->setText(tempSS.str());

	}

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();
	//if( mJoy ) mJoy->capture();

	moveCamera(evt.timeSinceLastFrame);

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

	return mContinue;
}

bool ExampleFrameListener::frameEnded(const FrameEvent& evt)
{
	updateStats();
	return true;
}

/*! \brief Exit the game.
 *
 */
bool ExampleFrameListener::quit(const CEGUI::EventArgs &e)
{
	mContinue = false;
	return true;
}

RaySceneQueryResult& ExampleFrameListener::doRaySceneQuery(const OIS::MouseEvent &arg)
{
	// Setup the ray scene query, use CEGUI's mouse position
	CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();
	Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.d_x/float(arg.state.width), mousePos.d_y/float(arg.state.height));
	mRaySceneQuery->setRay(mouseRay);
	mRaySceneQuery->setSortByDistance(true);
	
	// Execute query
	return mRaySceneQuery->execute();
}

/*! \brief Process the mouse movement event.
 *
 * The function does a raySceneQuery to determine what object the mouse is over
 * to handle things like dragging out selections of tiles and selecting
 * creatures.
 */
bool ExampleFrameListener::mouseMoved(const OIS::MouseEvent &arg)
{
	string  resultName;

	CEGUI::System::getSingleton().injectMouseMove(arg.state.X.rel, arg.state.Y.rel);

	RaySceneQueryResult &result = doRaySceneQuery(arg);
	RaySceneQueryResult::iterator itr = result.begin( );

	if(mDragType == ExampleFrameListener::tileSelection || mDragType == ExampleFrameListener::nullDragType)
	{
		// Since this is a tile selection query we loop over the result set and look for the first object which is actually a tile.
		itr = result.begin();
		while(itr != result.end())
		{
			if(itr->movable != NULL)
			{
				// Check to see if the current query result is a tile.
				resultName = itr->movable->getName();
				if(resultName.find("Level_") != string::npos)
				{
					// Get the x-y coordinates of the tile.
					sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

					// Make sure the "square selector" mesh is visible and position it over the current tile.
					mSceneMgr->getEntity("SquareSelector")->setVisible(true);
					mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(xPos, yPos, 0);

					if(mLMouseDown)
					{
						// Loop over the tiles in the rectangular selection region and set their setSelected flag accordingly.
						//TODO: This function is horribly inefficient, it should loop over a rectangle selecting tiles by x-y coords rather than the reverse that it is doing now.
						TileMap_t::iterator itr = gameMap.firstTile();
						while(itr != gameMap.lastTile())
						{
							Tile *tempTile = itr->second;
							if(tempTile->x >= min(xPos, mLStartDragX) && \
									tempTile->x <= max(xPos, mLStartDragX) && \
									tempTile->y >= min(yPos, mLStartDragY) && \
									tempTile->y <= max(yPos, mLStartDragY))
							{
								tempTile->setSelected(true);
							}
							else
							{
								tempTile->setSelected(false);
							}

							itr++;
						}
					}

					if(mRMouseDown)
					{
					}

					break;
				}
			}

			itr++;
		}
	}

	else //if(mDragType == ExampleFrameListener::creature)
	{
		// We are dragging a creature but we want to loop over the result set to find the first tile entry,
		// we do this to get the current x-y location of where the "square selector" should be drawn.
		itr = result.begin( );
		while(itr != result.end())
		{
			if(itr->movable != NULL)
			{
				// Check to see if the current query result is a tile.
				resultName = itr->movable->getName();
				if(resultName.find("Level_") != string::npos)
				{
					// Get the x-y coordinates of the tile.
					sscanf(resultName.c_str(), "Level_%i_%i", &xPos, &yPos);

					// Make sure the "square selector" mesh is visible and position it over the current tile.
					mSceneMgr->getEntity("SquareSelector")->setVisible(true);
					mSceneMgr->getSceneNode("SquareSelectorNode")->setPosition(xPos, yPos, 0);
				}
			}

			itr++;
		}
	}

	// If we are drawing with the brush in the map editor.
	if(mLMouseDown && mDragType == ExampleFrameListener::tileBrushSelection && serverSocket == NULL && clientSocket == NULL)
	{
		// Loop over the square region surrounding current mouse location and either set the tile type of the affected tiles or create new ones.
		Tile *currentTile;
		list<Tile*> affectedTiles;
		for(int i = -1*(mCurrentTileRadius-1); i <= (mCurrentTileRadius-1); i++)
		{
			for(int j = -1*(mCurrentTileRadius-1); j <= (mCurrentTileRadius-1); j++)
			{
				//TODO: Rather than drawing a rectangular region around the brush, check the rSquared distance and use that to exclude tiles that do not lie in a circle of the same radius as the rectangle, note: the next loop which checks the meshes needs to reflect this change.
				currentTile = gameMap.getTile(xPos + i, yPos + j);

				// Check to see if the current tile already exists.
				if(currentTile != NULL)
				{
					// It does exist so set its type and fullness.
					affectedTiles.push_back(currentTile);
					currentTile->setType(mCurrentTileType);
					currentTile->setFullness(mCurrentFullness);
				}
				else
				{
					// The current tile does not exist so we need to create it.
					//currentTile = new Tile;
					char tempArray[255];
					snprintf(tempArray, sizeof(tempArray), "Level_%3i_%3i", xPos + i, yPos + j);
					currentTile = new Tile(xPos + i, yPos + j, mCurrentTileType, mCurrentFullness);
					currentTile->name = tempArray;
					gameMap.addTile(currentTile);
					currentTile->createMesh();
				}
			}
		}

		// Add any tiles which border the affected region to the affected tiles list
		// as they may alo want to switch meshes to optimize polycount now too.
		//      Adding the top and bottom rows
		int xMin = -1 * (mCurrentTileRadius-1) + xPos;
		int xMax = (mCurrentTileRadius-1) + xPos;
		int yMin = -1 * (mCurrentTileRadius-1) + yPos;
		int yMax = (mCurrentTileRadius-1) + yPos;
		for(int i = xMin-1; i < xMax+1; i++)
		{
			Tile *currentTile = gameMap.getTile(i, yMax+1);
			if(currentTile != NULL)
			{
				affectedTiles.push_back(currentTile);
			}

			currentTile = gameMap.getTile(i, yMin-1);
			if(currentTile != NULL)
			{
				affectedTiles.push_back(currentTile);
			}
		}

		//      Adding the side rows
		for(int j = yMin-1; j < yMax+1; j++)
		{
			Tile *currentTile = gameMap.getTile(xMax+1, j);
			if(currentTile != NULL)
			{
				affectedTiles.push_back(currentTile);
			}

			currentTile = gameMap.getTile(xMin-1, j);
			if(currentTile != NULL)
			{
				affectedTiles.push_back(currentTile);
			}
		}

		// Loop over all the affected tiles and force them to examine their
		// neighbors.  This allows them to switch to a mesh with fewer
		// polygons if some are hidden by the neighbors.
		list<Tile*>::iterator itr = affectedTiles.begin();
		while(itr != affectedTiles.end())
		{
			(*itr)->setFullness( (*itr)->getFullness() );
			itr++;
		}
	}

	// If we are dragging a map light we need to update its position to the current x-y location.
	if(mLMouseDown && mDragType == ExampleFrameListener::mapLight && serverSocket == NULL && clientSocket == NULL)
	{
		MapLight *tempMapLight = gameMap.getMapLight(draggedMapLight);
		if(tempMapLight != NULL)
			tempMapLight->setPosition(xPos, yPos, tempMapLight->getPosition().z);
	}

	// Check the scroll wheel.
	if(arg.state.Z.rel > 0)
	{
		gameMap.me->rotateCreaturesInHand(1);
	}

	if(arg.state.Z.rel < 0)
	{
		gameMap.me->rotateCreaturesInHand(-1);
	}

	return true;
}

/*! \brief Handle mouse clicks.
 *
 * This function does a ray scene query to determine what is under the mouse
 * and determines whether a creature or a selection of tiles, is being dragged.
 */
bool ExampleFrameListener::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonDown(convertButton(id));
	string  resultName;

	// If the mouse press is on a CEGUI window ignore it
	CEGUI::Window *tempWindow = CEGUI::System::getSingleton().getWindowContainingMouse();
	if(tempWindow != NULL && tempWindow->getName().compare("Root") != 0)
	{
		//cout << "\n\nWindowName:  " << tempWindow->getName();
		ceguiHasControl = true;
		return true;
	}
	else
	{
		// The mouse press is somewhere not on a CEGUI window
		// Deactivate the active CEGUI control to return keyboard control back to OpenDungeons
		if(ceguiHasControl)
		{
			ceguiHasControl = false;
			ignoreOneMouseReleased = true;

			CEGUI::Window *tempWindow = CEGUI::System::getSingleton().getGUISheet();
			if(tempWindow != NULL)
			{
				tempWindow = tempWindow->getActiveChild();
				if(tempWindow != NULL)
				{
					//cout << "\nUser clicked on root window, returning to normal OpenDungeons controls.";
					tempWindow->deactivate();
				}
			}
			return true;
		}
	}

	RaySceneQueryResult &result = doRaySceneQuery(arg);
	RaySceneQueryResult::iterator itr = result.begin( );

	// Left mouse button down
	if (id == OIS::MB_Left)
	{
		mLMouseDown = true;
		mLStartDragX = xPos;
		mLStartDragY = yPos;

		// See if the mouse is over any creatures
		while (itr != result.end() )
		{
			if(itr->movable != NULL)
			{
				resultName = itr->movable->getName();

				if(resultName.find("Creature_") != string::npos)
				{
					// if in a game:  Pick the creature up and put it in our hand
					if(serverSocket != NULL || clientSocket != NULL)
					{
						// through away everything before the '_' and then copy the rest into 'array'
						char array[255];
						stringstream tempSS;
						tempSS.str(resultName);
						tempSS.getline(array, sizeof(array), '_');
						tempSS.getline(array, sizeof(array));

						Creature *currentCreature = gameMap.getCreature(array);
						if(currentCreature != NULL && currentCreature->color == gameMap.me->seat->color)
						{
							gameMap.me->pickUpCreature(currentCreature);
							return true;
						}
					}
					else  // if in the Map Editor:  Begin dragging the creature
					{
						//Entity *resultEnt = mSceneMgr->getEntity(resultName);
						mSceneMgr->getEntity("SquareSelector")->setVisible(false);

						
						draggedCreature = resultName.substr(((string)"Creature_").size(), resultName.size());
						SceneNode *node = mSceneMgr->getSceneNode(draggedCreature	+ "_node");
						creatureSceneNode->removeChild(node);
						mSceneMgr->getSceneNode("Hand_node")->addChild(node);
						node->setPosition(0,0,0);
						mDragType = ExampleFrameListener::creature;
						return true;
					}
				}

			}

			itr++;
		}

		// If no creatures are under the  mouse run through the list again to check for lights
		if(serverSocket == NULL && clientSocket == NULL)
		{
			//FIXME: These other code blocks that loop over the result list should probably use this same loop structure.
			itr = result.begin( );
			while(itr != result.end())
			{
				itr++;
				if(itr == result.end())
					break;

				if(itr->movable != NULL)
				{
					resultName = itr->movable->getName();
					if(resultName.find("MapLightIndicator_") != string::npos)
					{
						mDragType = ExampleFrameListener::mapLight;
						draggedMapLight = resultName.substr(((string)"MapLightIndicator_").size(), resultName.size());
						return true;
					}
				}
			}
		}

		// If no creatures or lights are under the  mouse run through the list again to check for tiles
		itr = result.begin( );
		while(itr != result.end())
		{
			itr++;
			if(itr->movable != NULL)
			{
				if(resultName.find("Level_") != string::npos)
				{
					if(serverSocket != NULL || clientSocket != NULL || !mBrushMode)
					{
						mDragType = ExampleFrameListener::tileSelection;
					}
					else
					{
						mDragType = ExampleFrameListener::tileBrushSelection;
					}

					break;
				}
			}
		}

		if(serverSocket != NULL || clientSocket != NULL)
		{
			Tile *tempTile = gameMap.getTile(xPos, yPos);
			if(tempTile != NULL)
			{
				digSetBool = !(tempTile->getMarkedForDigging(gameMap.me));
			}
		}
	}

	// Right mouse button down
	if(id == OIS::MB_Right)
	{
		mRMouseDown = true;
		mRStartDragX = xPos;
		mRStartDragY = yPos;

		Tile *curTile = gameMap.getTile(xPos, yPos);
		if(curTile != NULL)
		{
			gameMap.me->dropCreature(curTile);
		}
	}
	       
	return true;
}

/*! \brief Handle mouse button releases.
 *
 * Finalize the selection of tiles or drop a creature when the user releases the mouse button.
 */
bool ExampleFrameListener::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	CEGUI::System::getSingleton().injectMouseButtonUp(convertButton(id));

	// If the mouse release is on a CEGUI window ignore it
	if(ceguiHasControl)
	{
		return true;
	}

	if(ignoreOneMouseReleased)
	{
		ignoreOneMouseReleased = false;
		return true;
	}

	// Unselect all tiles
	//for(int i = 0; i < gameMap.numTiles(); i++)
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		itr->second->setSelected(false);

		itr++;
	}

	// Left mouse button up
	if (id == OIS::MB_Left)
	{
		// Check to see if we are moving a creature
		if(mDragType == ExampleFrameListener::creature)
		{
			if(serverSocket == NULL && clientSocket == NULL)
			{
				SceneNode *node = mSceneMgr->getSceneNode(draggedCreature + "_node");
				mSceneMgr->getSceneNode("Hand_node")->removeChild(node);
				creatureSceneNode->addChild(node);
				mDragType = ExampleFrameListener::nullDragType;
				gameMap.getCreature(draggedCreature)->setPosition(xPos, yPos, 0);
			}
		}

		// Check to see if we are dragging a map light.
		else if(mDragType == ExampleFrameListener::mapLight)
		{
			if(serverSocket == NULL && clientSocket == NULL)
			{
				MapLight *tempMapLight = gameMap.getMapLight(draggedMapLight);
				if(tempMapLight != NULL)
					tempMapLight->setPosition(xPos, yPos, tempMapLight->getPosition().z);
			}
		}

		// Check to see if we are dragging out a selection of tiles
		else if(mDragType == ExampleFrameListener::tileSelection)
		{
			list<Tile*> affectedTiles;
			int xMin = min(xPos, mLStartDragX);
			int xMax = max(xPos, mLStartDragX);
			int yMin = min(yPos, mLStartDragY);
			int yMax = max(yPos, mLStartDragY);
			for(int i = xMin; i <= xMax; i++)
			{
				for(int j = yMin; j <= yMax; j++)
				{
					// Make sure the tile exists before we set its value
					Tile *currentTile = gameMap.getTile(i, j);
					if(currentTile != NULL)
					{
						affectedTiles.push_back(currentTile);

						// See if we are hosting a game or not
						if(serverSocket == NULL)
						{
							if(clientSocket == NULL)
							{
								// In the map editor:  Fill the current tile with the new value
								currentTile->setType( mCurrentTileType );
								currentTile->setFullness( mCurrentFullness );
							}
							else
							{
								// On the client:  Inform the server about our choice
								if(currentTile->getFullness() > 0)
								{
									ClientNotification *clientNotification = new ClientNotification;
									clientNotification->type = ClientNotification::markTile;
									clientNotification->p = currentTile;
									clientNotification->flag = digSetBool;

									sem_wait(&clientNotificationQueueLockSemaphore);
									clientNotificationQueue.push_back(clientNotification);
									sem_post(&clientNotificationQueueLockSemaphore);

									sem_post(&clientNotificationQueueSemaphore);

									currentTile->setMarkedForDigging(digSetBool, gameMap.me);
								}
							}
						}
						else
						{
							if(currentTile->getFullness() > 0)
							{
								currentTile->setMarkedForDigging(digSetBool, gameMap.me);
							}
						}
					}
				}
			}

			// Loop over any tiles which had their fullness values changed and recheck their
			// neighbors to see if they need to update their meshes.  In server mode the mouse
			// drag does not change the tiles fullness so we can skip this step
			if(serverSocket == NULL)
			{
				// Add any tiles which border the affected region to the affected tiles list
				// as they may alo want to swicth meshes to optimize polycount now too.
				//      Adding the top and bottom rows
				for(int i = xMin-1; i < xMax+1; i++)
				{
					Tile *currentTile = gameMap.getTile(i, yMax+1);
					if(currentTile != NULL)
					{
						affectedTiles.push_back(currentTile);
					}

					currentTile = gameMap.getTile(i, yMin-1);
					if(currentTile != NULL)
					{
						affectedTiles.push_back(currentTile);
					}
				}

				//      Adding the side rows
				for(int j = yMin-1; j < yMax+1; j++)
				{
					Tile *currentTile = gameMap.getTile(xMax+1, j);
					if(currentTile != NULL)
					{
						affectedTiles.push_back(currentTile);
					}

					currentTile = gameMap.getTile(xMin-1, j);
					if(currentTile != NULL)
					{
						affectedTiles.push_back(currentTile);
					}
				}

				// Loop over all the affected tiles and force them to examine their
				// neighbors.  This allows them to switch to a mesh with fewer
				// polygons if some are hidden by the neighbors.
				list<Tile*>::iterator itr = affectedTiles.begin();
				while(itr != affectedTiles.end())
				{
					(*itr)->setFullness( (*itr)->getFullness() );
					itr++;
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

/*! \brief Handle the keyboard input.
 *
 * The operation of this function is largely determined by whether or not the
 * terminal is active or not.  When the terminal is active the keypresses are
 * treated as line editing on the terminal's command prompt.  When the terminal
 * is not active the keyboard is used to move the camera and control the game
 * through hotkeys.
 */
bool ExampleFrameListener::keyPressed(const OIS::KeyEvent &arg)
{
	using namespace OIS;
	string tempString;
	stringstream tempSS;

	CEGUI::System *sys = CEGUI::System::getSingletonPtr();
	sys->injectKeyDown(arg.key);
	sys->injectChar(arg.text);

	if(ceguiHasControl && arg.key != OIS::KC_ESCAPE)
	{
		//cout << "\n\nKey press ignored because CEGUI is active.";
		return mContinue;
	}

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
				if(serverSocket == NULL && clientSocket == NULL)
				{
					mCurrentTileType = Tile::nextTileType(mCurrentTileType);
					tempString = "";
					tempSS.str(tempString);
					tempSS << "Tile type:  " << Tile::tileTypeToString(mCurrentTileType);
					MOTD = tempSS.str();
				}
				break;

			//Decrease brush radius
			case KC_COMMA:
				if(serverSocket == NULL && clientSocket == NULL)
				{
					if(mCurrentTileRadius > 1)
					{
						mCurrentTileRadius--;
					}

					MOTD = "Brush size:  " + StringConverter::toString(mCurrentTileRadius);
				}
				break;

			//Increase brush radius
			case KC_PERIOD:
				if(serverSocket == NULL && clientSocket == NULL)
				{
					if(mCurrentTileRadius < 10)
					{
						mCurrentTileRadius++;
					}

					MOTD = "Brush size:  " + StringConverter::toString(mCurrentTileRadius);
				}
				break;

			//Toggle mBrushMode
			case KC_B:
				if(serverSocket == NULL && clientSocket == NULL)
				{
					mBrushMode = !mBrushMode;
					if(mBrushMode)
					{
						MOTD = "Brush mode turned on";
					}
					else
					{
						MOTD = "Brush mode turned off";
					}
				}
				break;

			//Toggle mCurrentFullness
			case KC_T:
				if(serverSocket == NULL && clientSocket == NULL)
				{
					mCurrentFullness = Tile::nextTileFullness(mCurrentFullness);
					MOTD = "Tile fullness:  " + StringConverter::toString(mCurrentFullness);
				}
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

/*! \brief Process the key up event.
 *
 * When a key is released during normal gamplay the camera movement may need to be stopped.
 */
bool ExampleFrameListener::keyReleased(const OIS::KeyEvent &arg)
{
	using namespace OIS;
	CEGUI::System::getSingleton().injectKeyUp(arg.key);

	if(ceguiHasControl)
	{
		//cout << "\nKey release ignored because CEGUI is active.";
		return mContinue;
	}

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

/*! \brief Print a string in the upper left corner of the screen.
 *
 * Displays the given text on the screen starting in the upper-left corner.
 * This is the function which displays the text on the in game console.
 */
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

/*! \brief Process the commandline from the terminal and carry out the actions specified in by the user.
 *
 */
void ExampleFrameListener::executePromptCommand()
{
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

	char array2[255];
	stringstream tempSS2;
	tempSS2.str(promptCommand);
	tempSS2.getline(array2, sizeof(array2), ' ');
	command = array2;
	tempSS2.getline(array2, sizeof(array2));
	arguments = array2;

	unsigned int count = 0;
	while(count < arguments.size() && arguments[count] == ' ')
	{
		arguments = arguments.substr(1, arguments.size()-1);
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
			string tempString = "Media/levels/" + arguments + ".level";
			if(readGameMapFromFile(tempString))
			{
				string tempString2 = "";
				stringstream tempSS(tempString2);
				tempSS << "Successfully loaded file:  " << tempString << "\nNum tiles:  " << gameMap.numTiles() << "\nNum classes:  " << gameMap.numClassDescriptions() << "\nNum creatures:  " << gameMap.numCreatures();
				commandOutput = tempSS.str();

				gameMap.createAllEntities();
			}
			else
			{
				tempSS << "ERROR: Could not load game map \'" << tempString << "\'.";
				commandOutput = tempSS.str();
			}
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
			commandOutput = "Ambient light set to:\nRed:  " + StringConverter::toString((Real)tempR) + "    Green:  " + StringConverter::toString((Real)tempG) + "    Blue:  " + StringConverter::toString((Real)tempB);

		}
		else
		{
			ColourValue curLight = mSceneMgr->getAmbientLight();
			commandOutput = "Current ambient light is:\nRed:  " + StringConverter::toString((Real)curLight.r) + "    Green:  " + StringConverter::toString((Real)curLight.g) + "    Blue:  " + StringConverter::toString((Real)curLight.b);
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
			commandOutput += "         " + StringConverter::toString(i+1);
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
		int xMin, yMin, xMax, yMax;
		xMin = min(x1, x2);
		xMax = max(x1, x2);
		yMin = min(y1,y2);
		yMax = max(y1, y2);

		for(int j = yMin; j < yMax; j++)
		{
			for(int i = xMin; i < xMax; i++)
			{
				if(gameMap.getTile(i, j) == NULL)
				{

					char tempArray[255];
					snprintf(tempArray, sizeof(tempArray), "Level_%3i_%3i", i, j);
					Tile *t = new Tile(i, j, Tile::dirt, 100);
					t->name = tempArray;
					gameMap.addTile(t);
					t->createMesh();
				}
			}
		}

		commandOutput = "Creating tiles for region:\n\n\t(" + StringConverter::toString(xMin) + ", " + StringConverter::toString(yMin) + ")\tto\t(" + StringConverter::toString(xMax) + ", " + StringConverter::toString(yMax) + ")";
	}

	// A utility to set the camera movement speed
	else if(command.compare("movespeed") == 0)
	{
		if(arguments.size() > 0)
		{
			tempSS.str(arguments);
			tempSS >> mMoveSpeed;
			commandOutput = "movespeed set to " + StringConverter::toString(mMoveSpeed);
		}
		else
		{
			commandOutput =  "Current movespeed is " + StringConverter::toString(mMoveSpeed);
		}
	}

	// A utility to set the camera rotation speed.
	else if(command.compare("rotatespeed") == 0)
	{
		if(arguments.size() > 0)
		{
			double tempDouble;
			tempSS.str(arguments);
			tempSS >> tempDouble;
			mRotateSpeed = Ogre::Degree(tempDouble);
			commandOutput = "rotatespeed set to " + StringConverter::toString((Real)mRotateSpeed.valueDegrees());
		}
		else
		{
			commandOutput =  "Current rotatespeed is " + StringConverter::toString((Real)mRotateSpeed.valueDegrees());
		}
	}

	// Set max frames per second
	else if(command.compare("fps") == 0)
	{
		if(arguments.size() > 0)
		{
			double tempDouble;
			tempSS.str(arguments);
			tempSS >> tempDouble;
			MAX_FRAMES_PER_SECOND = tempDouble;
			commandOutput = "Maximum framerate set to " + StringConverter::toString((Real)MAX_FRAMES_PER_SECOND);
		}
		else
		{
			commandOutput = "Current maximum framerate is " + StringConverter::toString((Real)MAX_FRAMES_PER_SECOND);
		}
	}

	// Set the turnsPerSecond variable to control the AI speed
	else if(command.compare("turnspersecond") == 0 || command.compare("tps") == 0)
	{
		if(arguments.size() > 0)
		{
			tempSS.str(arguments);
			tempSS >> turnsPerSecond;
			
			if(serverSocket != NULL)
			{
				try
				{
					// Inform any connected clients about the change
					ServerNotification *serverNotification = new ServerNotification;
					serverNotification->type = ServerNotification::setTurnsPerSecond;
					serverNotification->doub = turnsPerSecond;

					queueServerNotification(serverNotification);
				}
				catch(bad_alloc&)
				{
					cerr << "\n\nERROR:  bad alloc in terminal command \'turnspersecond\'\n\n";
					exit(1);
				}
			}

			commandOutput = "Maximum turns per second set to " + StringConverter::toString((Real)turnsPerSecond);
		}
		else
		{
			commandOutput = "Current maximum turns per second is " + StringConverter::toString((Real)turnsPerSecond);
		}
	}

	// Set near clip distance
	else if(command.compare("nearclip") == 0)
	{
		if(arguments.size() > 0)
		{
			double tempDouble;
			tempSS.str(arguments);
			tempSS >> tempDouble;
			mCamera->setNearClipDistance(tempDouble);
			commandOutput = "Near clip distance set to " + StringConverter::toString((Real)mCamera->getNearClipDistance());
		}
		else
		{
			commandOutput = "Current near clip distance is " + StringConverter::toString((Real)mCamera->getNearClipDistance());
		}
	}

	// Set far clip distance
	else if(command.compare("farclip") == 0)
	{
		if(arguments.size() > 0)
		{
			double tempDouble;
			tempSS.str(arguments);
			tempSS >> tempDouble;
			mCamera->setFarClipDistance(tempDouble);
			commandOutput = "Far clip distance set to " + StringConverter::toString((Real)mCamera->getFarClipDistance());
		}
		else
		{
			commandOutput = "Current far clip distance is " + StringConverter::toString((Real)mCamera->getFarClipDistance());
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
			Creature *tempClass = gameMap.getClassDescription(tempCreature->className);
			if(tempClass != NULL)
			{
				tempCreature->meshName = tempClass->meshName;
				tempCreature->scale = tempClass->scale;
				gameMap.addCreature(tempCreature);

				// Create the mesh and SceneNode for the new creature
				Entity *ent = mSceneMgr->createEntity("Creature_" + tempCreature->name, tempCreature->meshName);
				SceneNode *node = creatureSceneNode->createChildSceneNode(tempCreature->name + "_node");
				//node->setPosition(tempCreature->getPosition()/BLENDER_UNITS_PER_OGRE_UNIT);
				node->setPosition(tempCreature->getPosition());
				node->setScale(tempCreature->scale);
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
				ent->setNormaliseNormals(true);
#endif
				node->attachObject(ent);
				commandOutput = "Creature added successfully";
			}
			else
			{
				commandOutput = "Invalid creature class name, you need to first add a class with the \'addclass\' terminal command.";
			}
		}
	}

	// Adds the basic information about a type of creature (mesh name, scaling, etc)
	else if(command.compare("addclass") == 0)
	{
		if(arguments.size() > 0)
		{
			//FIXME: this code should be standardaized with the equivalent code in readGameMapFromFile()
			// This will require making CreatureClass a base class of Creature
			double tempX, tempY, tempZ, tempSightRadius, tempDigRate, tempMoveSpeed;
			int tempHP, tempMana;
			string tempString, tempString2;
			tempSS.str(arguments);
			tempSS >> tempString >> tempString2 >> tempX >> tempY >> tempZ >> tempHP >> tempMana >> tempSightRadius >> tempDigRate >> tempMoveSpeed;
			Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ), tempHP, tempMana, tempSightRadius, tempDigRate, tempMoveSpeed);
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
				tempSS << "Class:\tCreature name:\tLocation:\tColor:\tLHand:\tRHand\n\n";
				for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
				{
					tempSS << gameMap.getCreature(i) << endl;
				}
			}

			else if(arguments.compare("classes") == 0)
			{
				tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
				for(unsigned int i = 0; i < gameMap.numClassDescriptions(); i++)
				{
					Creature *currentClassDesc = gameMap.getClassDescription(i);
					tempSS << currentClassDesc->className << "\t" << currentClassDesc->meshName << "\t";
					tempSS << currentClassDesc->scale << "\t" << currentClassDesc->hp << "\t";
					tempSS << currentClassDesc->mana << "\t" << currentClassDesc->sightRadius << "\t";
					tempSS << currentClassDesc->digRate << "\t" << currentClassDesc->moveSpeed << "\n";
				}
			}

			else if(arguments.compare("players") == 0)
			{
				// There are only players if we are in a game.
				if(serverSocket != NULL || clientSocket != NULL)
				{
					tempSS << "Player:\tNick:\tColor:\n\n";
					tempSS << "me\t\t" << gameMap.me->nick << "\t" << gameMap.me->seat->color << "\n\n";
					for(unsigned int i = 0; i < gameMap.numPlayers(); i++)
					{
						Player *currentPlayer = gameMap.getPlayer(i);
						tempSS << i << "\t\t" << currentPlayer->nick << "\t" << currentPlayer->seat->color << "\n";
					}
				}
				else
				{
					tempSS << "You must either host or join a game before you can list the players in the game.\n";
				}
			}

			else if(arguments.compare("network") == 0)
			{
				if(clientSocket != NULL)
				{
					tempSS << "You are currently connected to a server.";
				}

				if(serverSocket != NULL)
				{
					tempSS << "You are currently acting as a server.";
				}

				if(clientSocket == NULL && serverSocket == NULL)
				{
					tempSS << "You are currently in the map editor.";
				}
			}

			else if(arguments.compare("rooms") == 0)
			{
				tempSS << "Name:\tColor:\tNum tiles:\n\n";
				for(unsigned int i = 0; i < gameMap.numRooms(); i++)
				{
					Room *currentRoom;
					currentRoom = gameMap.getRoom(i);
					tempSS << currentRoom->name << "\t" << currentRoom->color << "\t" << currentRoom->numCoveredTiles() << "\n";
				}
			}

			else if(arguments.compare("colors") == 0 || arguments.compare("colours") == 0)
			{
				tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
				for(unsigned int i = 0; i < playerColourValues.size(); i++)
				{
					tempSS << "\n" << i << "\t\t" << playerColourValues[i].r << "\t\t" << playerColourValues[i].g << "\t\t" << playerColourValues[i].b;
				}
			}

                        // Loop over level directory and display only level files 
                        else if(arguments.compare("levels") == 0)      
                        {
                            vector<string> tempVector;
                            size_t found;
                            size_t found2;
                            string suffix = ".level";
                            string suffix2 = ".level.";
                            tempVector = listAllFiles("./Media/levels/");
                            for(unsigned int j = 0; j < tempVector.size(); j++)
                            
                            {
                                found = tempVector[j].find(suffix);
                                found2 = tempVector[j].find(suffix2);
                                if(found != string::npos && (!(found2 != string::npos)))
                                {
                                    tempSS << tempVector[j] << endl;
                                }
                            }
                            

                        }

			else if(arguments.compare("goals") == 0)
			{
				if(serverSocket != NULL || clientSocket != NULL)
				{
					// Loop over the list of unmet goals for the seat we are sitting in an print them.
					tempSS << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
					for(unsigned int i = 0; i < gameMap.me->seat->numGoals(); i++)
					{
						Goal *tempGoal = gameMap.me->seat->getGoal(i);
						tempSS << tempGoal->getName() << ":\t" << tempGoal->getDescription() << "\n";
					}

					// Loop over the list of completed goals for the seat we are sitting in an print them.
					tempSS << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
					for(unsigned int i = 0; i < gameMap.me->seat->numCompletedGoals(); i++)
					{
						Goal *tempGoal = gameMap.me->seat->getCompletedGoal(i);
						tempSS << tempGoal->getName() << ":\t" << tempGoal->getSuccessMessage() << "\n";
					}
				}
				else
				{
					tempSS << "\n\nERROR: You do not have any goals to meet until you host or join a game.\n\n";
				}
			}

			else
			{
				tempSS << "ERROR:  Unrecognized list.  Type \"list\" with no arguments to see available lists.";
			}

			commandOutput = tempSS.str();
		}
		else
		{
			commandOutput = "lists available:\n\t\tclasses\tcreatures\tplayers\n\t\tnetwork\trooms\tcolors\n\t\tgoals\tlevels";
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
		string tempString;
		if(arguments.size() > 0)
		{
			gameMap.me->nick = arguments;

			commandOutput = "Nickname set to:  " + gameMap.me->nick;
		}
		else
		{
			commandOutput = "Current nickname is:  " + gameMap.me->nick;
		}
	}

	// Connect to a server
	else if(command.compare("connect") == 0)
	{
		// Make sure we have set a nickname.
		if(gameMap.me->nick.size() > 0)
		{
			// Make sure we are not already connected to a server or hosting a game.
			if(serverSocket == NULL && clientSocket == NULL)
			{
				// Make sure an IP address to connect to was provided
				if(arguments.size() > 0)
				{
					clientSocket = new Socket;

					if(!clientSocket->create())
					{
						clientSocket = NULL;
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

						// Start the thread which will watch for local events to send to the server
						CNPStruct *cnps = new CNPStruct;
						cnps->nFrameListener = this;
						pthread_create(&clientNotificationThread, NULL, clientNotificationProcessor, cnps);

						// Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
						gameMap.clearMapLightIndicators();
					}
					else
					{
						clientSocket = NULL;
						commandOutput = "Connection failed!";
					}
				}
				else
				{
					commandOutput = "You must specify the IP address of the server you want to connect to.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1";
				}

			}
			else
			{
				commandOutput = "You are already connected to a server.  You must disconnect before you can connect to a new game.";
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
		// Make sure we have set a nickname.
		if(gameMap.me->nick.size() > 0)
		{
			// Make sure we are not already connected to a server or hosting a game.
			if(serverSocket == NULL && clientSocket == NULL)
			{
				serverSocket = new Socket;

				// Start the server socket listener as well as the server socket thread
				if(serverSocket != NULL && gameMap.numEmptySeats() > 0)
				{
					// Sit down at the first available seat.
					gameMap.me->seat = gameMap.popEmptySeat();

					// Start the server thread which will listen for, and accept, connections
					SSPStruct *ssps = new SSPStruct;
					ssps->nSocket = serverSocket;
					ssps->nFrameListener = this;
					pthread_create(&serverThread, NULL, serverSocketProcessor, (void*) ssps);

					// Start the thread which will watch for local events to send to the clients
					SNPStruct *snps = new SNPStruct;
					snps->nFrameListener = this;
					pthread_create(&serverNotificationThread, NULL, serverNotificationProcessor, snps);

					// Start the creature AI thread
					pthread_create(&creatureThread, NULL, creatureAIThread, NULL);

					// Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
					gameMap.clearMapLightIndicators();

					commandOutput = "Server started successfully.";
				}
				else
				{
					commandOutput = "ERROR:  Could not start server!";
				}

			}
			else
			{
				commandOutput = "ERROR:  You are already connected to a game or are already hosting a game!";
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
			sem_wait(&clientSocket->semaphore);
			clientSocket->send(formatCommand("chat", gameMap.me->nick + ":" + arguments));
			sem_post(&clientSocket->semaphore);
		}
		else if(serverSocket != NULL)
		{
			// Send the chat to all the connected clients
			for(unsigned int i = 0; i < clientSockets.size(); i++)
			{
				sem_wait(&clientSockets[i]->semaphore);
				clientSockets[i]->send(formatCommand("chat", gameMap.me->nick + ":" + arguments));
				sem_post(&clientSockets[i]->semaphore);
			}

			// Display the chat message in our own message queue
			chatMessages.push_back(new ChatMessage(gameMap.me->nick, arguments, time(NULL), time(NULL)));
		}
		else
		{
			commandOutput = "You must be either connected to a server, or hosting a server to use chat.";
		}
	}

	// Start the visual debugging indicators for a given creature
	else if(command.compare("visdebug") == 0)
	{
		if(serverSocket != NULL)
		{
			if(arguments.length() > 0)
			{
				// Activate visual debugging
				Creature *tempCreature = gameMap.getCreature(arguments);
				if(tempCreature != NULL)
				{
					if(!tempCreature->getHasVisualDebuggingEntities())
					{
						tempCreature->createVisualDebugEntities();
						commandOutput = "Visual debugging entities created for creature:  " + arguments;
					}
					else
					{
						tempCreature->destroyVisualDebugEntities();
						commandOutput = "Visual debugging entities destroyed for creature:  " + arguments;
					}
				}
				else
				{
					commandOutput = "Could not create visual debugging entities for creature:  " + arguments;
				}
			}
			else
			{
				commandOutput = "ERROR:  You must supply a valid creature name to create debug entities for.";
			}
		}
		else
		{
			commandOutput = "ERROR:  Visual debugging only works when you are hosting a game.";
		}
	}

	else if(command.compare("addcolor") == 0)
	{
		string tempString;

		if(arguments.size() > 0)
		{
			double tempR, tempG, tempB;
			tempSS.str(arguments);
			tempSS >> tempR >> tempG >> tempB;
			playerColourValues.push_back(ColourValue(tempR, tempG, tempB));
			tempString = "";
			tempSS.str(tempString);
			tempSS << "Color number " << playerColourValues.size() << " added.";
			commandOutput = tempSS.str();
		}
		else
		{
			commandOutput = "ERROR:  You need to specify and RGB triplet with values in (0.0, 1.0)";
		}
	}

	else if(command.compare("setcolor") == 0)
	{
		string tempString;

		if(arguments.size() > 0)
		{
			unsigned int index;
			double tempR, tempG, tempB;
			tempSS.str(arguments);
			tempSS >> index >> tempR >> tempG >> tempB;
			if(index < playerColourValues.size())
			{
				playerColourValues[index] = ColourValue(tempR, tempG, tempB);
				tempString = "";
				tempSS.str(tempString);
				tempSS << "Color number " << index << " changed to " << tempR << "\t" << tempG << "\t" << tempB;
				commandOutput = tempSS.str();
			}

		}
		else
		{
			tempString = "";
			tempSS.str(tempString);
			tempSS << "ERROR:  You need to specify a color index between 0 and " << playerColourValues.size() << " and an RGB triplet with values in (0.0, 1.0)";
			commandOutput = tempSS.str();
		}
	}

	//FIXME:  This function is not yet implemented.
	else if(command.compare("disconnect") == 0)
	{
		if(serverSocket != NULL)
		{
			commandOutput = "Stopping server.";
		}
		else
		{
			if(clientSocket != NULL)
			{
				commandOutput = "Disconnecting from server.";
			}
			else
			{
				commandOutput = "You are not connected to a server and you are not hosting a server.";
			}
		}
	}

	// Load the next level.
	else if(command.compare("next") == 0)
	{
		if(gameMap.seatIsAWinner(gameMap.me->seat))
		{
			gameMap.loadNextLevel = true;
			commandOutput = (string)"Loading level Media/levels/" + gameMap.nextLevel + ".level";
		}
		else
		{
			commandOutput = "You have not completed this level yet.";
		}
	}

	else commandOutput = "Command not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.";

	promptCommand = "";
}

/*! \brief A helper function to return a help text string for a given termianl command.
 *
 */
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

	else if(arg.compare("visdebug") == 0)
	{
		return "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n" + prompt + "visdebug skeletor\n\nThe above command wil turn on visual debugging for the creature named \'skeletor\'.  The same command will turn it back off again.";
	}

	else if(arg.compare("turnspersecond") == 0 || arg.compare("tps") == 0)
	{
		return "turnspersecond (or \"tps\" for short is a utility which displays or sets the speed at which the game is running.\n\nExample:\n" + prompt + "tps 5\n\nThe above command will set the current game speed to 5 turns per second.";
	}

	else if(arg.compare("framespersecond") == 0 || arg.compare("fps") == 0)
	{
		return "framespersecond (or \"fps\" for short is a utility which displays or sets the maximum framerate at which the rendering will attempt to update the screen.\n\nExample:\n" + prompt + "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.";
	}

	return "Help for command:  \"" + arguments + "\" not found.";
}

