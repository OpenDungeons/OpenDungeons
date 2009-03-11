#include "MapEditor.h"
#include <string>
#include <sstream>
#include <fstream>
using namespace std;

#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"

MapEditor::MapEditor()
	: mSystem(0), mRenderer(0)
{
}

MapEditor::~MapEditor() 
{
	if(mSystem)
		delete mSystem;

	if(mRenderer)
		delete mRenderer;
}

void MapEditor::createCamera(void)
{
	SceneNode *node;

	// Set up the main camera
	mCamera = mSceneMgr->createCamera("PlayerCam");
	mCamera->setNearClipDistance(.05);
	mCamera->setFarClipDistance(300.0);
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("CameraTarget");
	mCamera->setAutoTracking(false, node, Ogre::Vector3(0, 0, 0));
}

void MapEditor::createScene(void)
{
	int choice;
	int newXSize, newYSize;
	mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));

	// Turn on shadows
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);

	Entity *ent;
	SceneNode *node;

	//FIXME:  This menu (or  something like it) should be enabled in the full game
	//cout << "Open Dungeons\tVersion:  " << VERSION << endl;
	//cout << "Main Menu:\n  1:  Create new level\n  2:  Load existing level\n\nChoice:  ";
	//cin >> choice;
	choice = 2;

	switch(choice)
	{
		case 1:
			//FIXME:  This menu (or  something like it) should be enabled in the full game
			//cout << "\n\nEnter X and Y dimensions for new map:  ";
			//cin >> newXSize >> newYSize;
			newXSize = 30;
			newYSize = 60;
			gameMap.createNewMap(newXSize, newYSize);
			break;

		case 2:
			//FIXME:  Another menu needed here too
			readGameMapFromFile("Media/levels/Test.level");
			break;

		default:
			exit(1);
			break;
	}

	// Create ogre entities for the tiles and creatures
	gameMap.createAllEntities();

	// Create the main scene light
	Light *light = mSceneMgr->createLight("Light1");
	light->setType(Light::LT_DIRECTIONAL);
	light->setDirection(Ogre::Vector3(-1, -1, -1));
	light->setDiffuseColour(ColourValue(.65, .65, .85));
	light->setSpecularColour(ColourValue(.0, .0, .0));

	// Create the scene node that the camera attaches to
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
	node->pitch(Degree(25), Node::TS_WORLD);
	node->roll(Degree(30), Node::TS_WORLD);
	node->attachObject(mCamera);

	// Create the single tile selection mesh
	ent = mSceneMgr->createEntity("SquareSelector", "SquareSelector.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
	//node->translate(Ogre::Vector3(1/BLENDER_UNITS_PER_OGRE_UNIT, 1/BLENDER_UNITS_PER_OGRE_UNIT, 0));
	node->translate(Ogre::Vector3(0, 0, 0));
	node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,BLENDER_UNITS_PER_OGRE_UNIT,BLENDER_UNITS_PER_OGRE_UNIT));
	node->attachObject(ent);

	// Create the light which follows the single tile selection mesh
	light = mSceneMgr->createLight("MouseLight");
	light->setType(Light::LT_POINT);
	light->setDiffuseColour(ColourValue(.8, .8, .6));
	light->setSpecularColour(ColourValue(.0, .0, .0));
	//light->setPosition(0, 0, 1.45/BLENDER_UNITS_PER_OGRE_UNIT);
	light->setPosition(0, 0, 5.45);
	light->setAttenuation(65, 0.0, 0.4, 0.6);
	node->attachObject(light);


	// Setup CEGUI
	mRenderer = new CEGUI::OgreCEGUIRenderer(mWindow, Ogre::RENDER_QUEUE_OVERLAY, false, 3000, mSceneMgr);
	mSystem = new CEGUI::System(mRenderer);

	// Show the mouse cursor
	CEGUI::SchemeManager::getSingleton().loadScheme((CEGUI::utf8*)"TaharezLookSkin.scheme");
	mSystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
	mSystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-12");
	CEGUI::MouseCursor::getSingleton().setImage(CEGUI::System::getSingleton().getDefaultMouseCursor());


	// Create the singleton for the TextRenderer class
	new TextRenderer();

	// Display some text
	TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 10, 10, 50, 70, Ogre::ColourValue::Green);

	// FIXME: OpenDungeons.layout needs to be filled in to get a gui going.
	//CEGUI::Window* sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"OpenDungeons.layout"); 
	//mSystem->setGUISheet(sheet);
}

void MapEditor::createFrameListener(void)
{
	mFrameListener = new ExampleFrameListener(mWindow, mCamera, mSceneMgr, mRenderer, true, true, false);
	mFrameListener->showDebugOverlay(true);
	mRoot->addFrameListener(mFrameListener);
}

void MapEditor::chooseSceneManager(void)
{
	// Use the terrain scene manager.
	mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
}

