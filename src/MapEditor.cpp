#include "MapEditor.h"
#include <string>
#include <sstream>
#include <fstream>

#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"
#include "Network.h"
#include "ButtonHandlers.h"

MapEditor::MapEditor()
	: mSystem(0), mRenderer(0)
{
}

MapEditor::~MapEditor() 
{
	if(mSystem)
		//delete mSystem;   // use this line if using a CEGUI version before 0.7
		mSystem->destroy(); // use this line if using a CEGUI version after 0.7

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
	//Initialise sound
	OgreOggSound::OgreOggSoundManager::getSingleton().init("", 100, 64, mSceneMgr);
	//OgreOggSound::OgreOggSoundManager::getSingleton().setMasterVolume(10);
	assert(SoundEffectsHelper::getSingletonPtr() == 0);
	std::cout << "Creating sf helper" << std::endl;
	new SoundEffectsHelper();

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
	Light *light;

	// Create the scene node that the camera attaches to
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
	node->pitch(Degree(25), Node::TS_WORLD);
	node->roll(Degree(30), Node::TS_WORLD);
	node->attachObject(mCamera);

	// Create the single tile selection mesh
	ent = mSceneMgr->createEntity("SquareSelector", "SquareSelector.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("SquareSelectorNode");
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
	light->setDiffuseColour(ColourValue(.5, .7, .6));
	light->setSpecularColour(ColourValue(.5, .4, .4));
	light->setPosition(0, 0, 5);
	light->setAttenuation(20, 0.15, 0.15, 0.017);
	node->attachObject(light);

	// Setup CEGUI
	mRenderer = &CEGUI::OgreRenderer::create();
	mSystem = &CEGUI::System::create(*mRenderer);

	// Show the mouse cursor
	CEGUI::SchemeManager::getSingleton().create((CEGUI::utf8*)"Media/gui/TaharezLookSkin.scheme");
	mSystem->setDefaultMouseCursor((CEGUI::utf8*)"TaharezLook", (CEGUI::utf8*)"MouseArrow");
	mSystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-12");
	CEGUI::MouseCursor::getSingleton().setImage(CEGUI::System::getSingleton().getDefaultMouseCursor());

	// Create the singleton for the TextRenderer class
	new TextRenderer();

	// Display some text
	TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 10, 10, 50, 70, Ogre::ColourValue::Green);

	try
	{
		CEGUI::Window* sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"Media/gui/OpenDungeons.layout"); 
		mSystem->setGUISheet(sheet);

		CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();

		CEGUI::Window *window;

		// Set the active tabs on the tab selector across the bottom of the screen so
		// the user doesn't have to click into them first to see the contents.
		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl");
		((CEGUI::TabControl*)window)->setSelectedTab(0);
		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 1/RoomSubTab");
		((CEGUI::TabControl*)window)->setSelectedTab(0);

		// Subscribe the various button handlers to the CEGUI button pressed events.
		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 6/QuitButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&quitButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 1/RoomSubTab/Tab 1/QuartersButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&quartersButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 1/RoomSubTab/Tab 1/TreasuryButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&treasuryButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 1/RoomSubTab/Tab 1/ForgeButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&forgeButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 1/RoomSubTab/Tab 2/CannonButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cannonButtonPressed));
	}
	catch (...)
	{
		cerr << "\n\nERROR:  Caught and ignored an exception in the loading of the CEGUI overlay, the game will continue to function albeit without the GUI overlay functionality.\n\n";
	}


	mMusicPlayer.load();
}

void MapEditor::createFrameListener(void)
{
	mFrameListener = new ExampleFrameListener(mWindow, mCamera, mSceneMgr, mRenderer, true, true, false);
	mFrameListener->showDebugOverlay(true);
	mRoot->addFrameListener(mFrameListener);
	mMusicPlayer.start();
}

void MapEditor::chooseSceneManager(void)
{
	// Use the terrain scene manager.
	mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
}

