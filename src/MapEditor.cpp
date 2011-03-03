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

	SoundEffectsHelper* sfxh = new SoundEffectsHelper();
	sfxh->initialiseSound(mResourcePath + "Media/sounds");
	// Turn on shadows
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);	// Quality 1
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);	// Quality 2
	//mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);	// Quality 3

	Entity *ent;
	SceneNode *node;

	// Read in the default game map
	gameMap.levelFileName = "Test";
	readGameMapFromFile(mResourcePath + "Media/levels/Test.level");

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
	mSystem = &CEGUI::System::create(*mRenderer,0, 0, 0, 0, "",
	        mResourcePath + "CEGUI.log");

	// Show the mouse cursor

	CEGUI::DefaultResourceProvider* rp =
	        static_cast<CEGUI::DefaultResourceProvider*>(
	                CEGUI::System::getSingleton().getResourceProvider());
	rp->setDefaultResourceGroup("default");
	//Set resource path, remove trailing slash just in case, as cegui adds an extra one.
	//FIXME - should use ogreresourceprovider, but have to fix paths in resource files first
	rp->setResourceGroupDirectory("default",
	        mResourcePath.substr(0, mResourcePath.size() - 2).c_str());

	Ogre::String schemePath("Media/gui/OpenDungeonsSkin.scheme");
	CEGUI::SchemeManager::getSingleton().create(schemePath);
	mSystem->setDefaultMouseCursor((CEGUI::utf8*)"OpenDungeons", (CEGUI::utf8*)"MouseArrow");
	//default font shouldn't be needed anymore with new layout
	//mSystem->setDefaultFont((CEGUI::utf8*)"BlueHighway-12");
	CEGUI::MouseCursor::getSingleton().setImage(CEGUI::System::getSingleton().getDefaultMouseCursor());

	// Create the singleton for the TextRenderer class
	new TextRenderer();

	// Display some text
	TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 10, 10, 50, 70, Ogre::ColourValue::Green);

    //try-catch really needed here?
	try
	{
		CEGUI::Window* sheet = CEGUI::WindowManager::getSingleton().loadWindowLayout((CEGUI::utf8*)"Media/gui/OpenDungeons.layout");
		mSystem->setGUISheet(sheet);

		CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();

		CEGUI::Window *window;

		// Set the active tabs on the tab selector across the bottom of the screen so
		// the user doesn't have to click into them first to see the contents.
		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl");
		((CEGUI::TabControl*)window)->setSelectedTab(0);

		// Subscribe the various button handlers to the CEGUI button pressed events.
		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/Rooms/QuartersButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&quartersButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/Rooms/TreasuryButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&treasuryButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/Rooms/ForgeButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&forgeButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/Rooms/DojoButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&dojoButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/Traps/CannonButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&cannonButtonPressed));

		window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/System/HostButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&serverButtonPressed));

        window = wmgr->getWindow((CEGUI::utf8*)"Root/MainTabControl/System/QuitButton");
		window->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&quitButtonPressed));

		//window = wmgr->getWindow((CEGUI::utf8*)"Root/MapEditorTabControl/Tab 6/LoadSaveCombobox");
		//CEGUI::ListboxTextItem *tempListboxItem = new CEGUI::ListboxTextItem("Blah");
		//((CEGUI::Combobox*)window)->addItem(tempListboxItem);
	}
	catch (...)
	{
		cerr << "\n\nERROR:  Caught and ignored an exception in the loading of the CEGUI overlay, the game will continue to function albeit without the GUI overlay functionality.\n\n";
	}

	MusicPlayer* m = new MusicPlayer();
	m->load(mResourcePath + "Media/music/");
}

void MapEditor::createFrameListener(void)
{
	mFrameListener = new ExampleFrameListener(mWindow, mCamera, mSceneMgr, mRenderer, true, true, false);
	exampleFrameListener = mFrameListener;
	mFrameListener->showDebugOverlay(true);
	mRoot->addFrameListener(mFrameListener);
	//Start music.
	//TODO - move this to when the map is actually loaded
	MusicPlayer::getSingleton().start();
}

void MapEditor::chooseSceneManager(void)
{
	// Use the terrain scene manager.
	mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
}

