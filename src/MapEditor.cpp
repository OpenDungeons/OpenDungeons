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
#include "Network.h"

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
	// Turn on shadows
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);	// Quality 1
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);	// Quality 2
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);	// Quality 3

	Entity *ent;
	SceneNode *node;

	// Read in the default game map
	readGameMapFromFile("Media/levels/Test.level");

	// Create ogre entities for the tiles, rooms, and creatures
	gameMap.createAllEntities();

	// Create the main scene lights
	mSceneMgr->setAmbientLight(ColourValue(0.3, 0.36, 0.28));
	double mainLightR = .42;
	double mainLightG = .4;
	double mainLightB = .35;
	double mainLightAttenuationD = 300.0;
	double mainLightAttenuationE = 0.5;
	double mainLightAttenuationL = 0.001;
	double mainLightAttenuationQ = 0.0005;
	Light *light = mSceneMgr->createLight("Light1");
	light->setType(Light::LT_POINT);
	light->setPosition(Ogre::Vector3(0, 0, 0));
	light->setDiffuseColour(ColourValue(mainLightR, mainLightG, mainLightB));
	light->setSpecularColour(ColourValue(.0, .0, .0));
	light->setAttenuation(mainLightAttenuationD, mainLightAttenuationE, mainLightAttenuationL, mainLightAttenuationQ);

	/*
	light = mSceneMgr->createLight("Light2");
	light->setType(Light::LT_POINT);
	light->setPosition(Ogre::Vector3(-30, 40, 40));
	light->setDiffuseColour(ColourValue(mainLightR, mainLightG, mainLightB));
	light->setSpecularColour(ColourValue(.0, .0, .0));
	light->setAttenuation(mainLightAttenuationD, mainLightAttenuationE, mainLightAttenuationL, mainLightAttenuationQ);
	*/

	// Create the scene node that the camera attaches to
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
	node->pitch(Degree(25), Node::TS_WORLD);
	node->roll(Degree(30), Node::TS_WORLD);
	node->attachObject(mCamera);
	node->attachObject(light);

	// Create the single tile selection mesh
	ent = mSceneMgr->createEntity("SquareSelector", "SquareSelector.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
	//node->translate(Ogre::Vector3(1/BLENDER_UNITS_PER_OGRE_UNIT, 1/BLENDER_UNITS_PER_OGRE_UNIT, 0));
	node->translate(Ogre::Vector3(0, 0, 0));
	node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,BLENDER_UNITS_PER_OGRE_UNIT,BLENDER_UNITS_PER_OGRE_UNIT));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
	ent->setNormaliseNormals(true);
#endif
	node->attachObject(ent);
	SceneNode *node2 = node->createChildSceneNode("Hand_node");
	node2->setPosition(0.0/BLENDER_UNITS_PER_OGRE_UNIT, 0.0/BLENDER_UNITS_PER_OGRE_UNIT, 3.0/BLENDER_UNITS_PER_OGRE_UNIT);
	node2->scale(Ogre::Vector3(1.0/BLENDER_UNITS_PER_OGRE_UNIT,1.0/BLENDER_UNITS_PER_OGRE_UNIT,1.0/BLENDER_UNITS_PER_OGRE_UNIT));

	// Create the light which follows the single tile selection mesh
	light = mSceneMgr->createLight("MouseLight");
	light->setType(Light::LT_POINT);
	light->setDiffuseColour(ColourValue(.18, .14, .11));
	light->setSpecularColour(ColourValue(.0, .0, .0));
	//light->setPosition(0, 0, 1.45/BLENDER_UNITS_PER_OGRE_UNIT);
	light->setPosition(0, 0, 11);
	light->setAttenuation(35, 0.0, 0.0, 0.01);
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
	try
	{
		CEGUI::Window* sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"OpenDungeons.layout"); 
		mSystem->setGUISheet(sheet);
		cerr << "\n\none.\n";

		CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();
		CEGUI::Window *quitWindow = wmgr->getWindow((CEGUI::utf8*)"QuitButton");
		cerr << "\n\ntwo.\n";

		//quitWindow->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::SubscriberSlot(&ExampleFrameListener::quit));
		quitWindow->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&quitButtonPressed));
		cerr << "\n\nthree.\n";
	}
	catch (...)
	{
		cerr << "\n\nERROR:  Caught and ignored an exception.\n\n";
	}

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

