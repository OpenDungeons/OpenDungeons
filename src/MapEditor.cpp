#include <string>
#include <sstream>
#include <fstream>

#include <CEGUI.h>

#include "Defines.h"
#include "Globals.h"
#include "Functions.h"
#include "ExampleApplication.h"
#include "ExampleFrameListener.h"
#include "Tile.h"
#include "Network.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "Gui.h"

#include "MapEditor.h"

MapEditor::MapEditor()
{
}

MapEditor::~MapEditor()
{
}

void MapEditor::createCamera(void)
{
    // Set up the main camera
    mCamera = mSceneMgr->createCamera("PlayerCam");
    mCamera->setNearClipDistance(.05);
    mCamera->setFarClipDistance(300.0);
    mCamera->setAutoTracking(false,
            mSceneMgr->getRootSceneNode()->createChildSceneNode("CameraTarget"),
            Ogre::Vector3(0, 0, 0));
}

void MapEditor::createScene(void)
{
    //Initialise sounds
    SoundEffectsHelper* sfxh = new SoundEffectsHelper();
    sfxh->initialiseSound(mResourcePath + "sounds");

    // Turn on shadows
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);	// Quality 1
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);	// Quality 2
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);	// Quality 3

    //Initialise render manager
    RenderManager* renderManager = new RenderManager();
    renderManager->initialize(mSceneMgr, &gameMap);

    // Read in the default game map
    std::string levelPath = mResourcePath + "levels_git/Test.level";
    {
        //Check if the level from git exists. If not, use the standard one.
        std::ifstream file(levelPath.c_str(), std::ios_base::in);
        if (!file.is_open())
        {
            levelPath = mResourcePath + "levels/Test.level";
        }
    }

    gameMap.levelFileName = "Test";
    readGameMapFromFile(levelPath);

    // Create ogre entities for the tiles, rooms, and creatures
    gameMap.createAllEntities();

    // Create the main scene lights
    mSceneMgr->setAmbientLight(ColourValue(0.3, 0.36, 0.28));

    // Create the scene node that the camera attaches to
    SceneNode*node = mSceneMgr->getRootSceneNode()
            ->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
    node->pitch(Degree(25), Node::TS_WORLD);
    node->roll(Degree(30), Node::TS_WORLD);
    node->attachObject(mCamera);

    // Create the single tile selection mesh
    Entity* ent = mSceneMgr->createEntity("SquareSelector", "SquareSelector.mesh");
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "SquareSelectorNode");
    node->translate(Ogre::Vector3(0, 0, 0));
    node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
            BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
    ent->setNormaliseNormals(true);
#endif
    node->attachObject(ent);
    SceneNode *node2 = node->createChildSceneNode("Hand_node");
    node2->setPosition(0.0 / BLENDER_UNITS_PER_OGRE_UNIT, 0.0
            / BLENDER_UNITS_PER_OGRE_UNIT, 3.0 / BLENDER_UNITS_PER_OGRE_UNIT);
    node2->scale(Ogre::Vector3(1.0 / BLENDER_UNITS_PER_OGRE_UNIT, 1.0
            / BLENDER_UNITS_PER_OGRE_UNIT, 1.0 / BLENDER_UNITS_PER_OGRE_UNIT));

    // Create the light which follows the single tile selection mesh
    Light* light = mSceneMgr->createLight("MouseLight");
    light->setType(Light::LT_POINT);
    light->setDiffuseColour(ColourValue(.5, .7, .6));
    light->setSpecularColour(ColourValue(.5, .4, .4));
    light->setPosition(0, 0, 5);
    light->setAttenuation(20, 0.15, 0.15, 0.017);
    node->attachObject(light);

    /* TODO: load main menu. Where? */

    new Gui();
    new TextRenderer();

    // Display some text
    TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 140,
            10, 50, 70, Ogre::ColourValue::Green);

    MusicPlayer* m = new MusicPlayer();
    m->load(mResourcePath + "music/");
}

void MapEditor::createFrameListener(void)
{
    mFrameListener = new ExampleFrameListener(mWindow, mCamera, mSceneMgr,
            true, true, false);
    exampleFrameListener = mFrameListener;
    mFrameListener->showDebugOverlay(true);
    mRoot->addFrameListener(mFrameListener);
    //Start music.
    //TODO - move this to when the map is actually loaded
    MusicPlayer::getSingleton().start(0);
}

void MapEditor::chooseSceneManager(void)
{
    // Use the terrain scene manager.
    mSceneMgr = mRoot->createSceneManager(ST_EXTERIOR_CLOSE);
}

