/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
 */

#include <string>
#include <sstream>
#include <fstream>

#include <sys/stat.h>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <shlwapi.h>
#include <direct.h>
#endif

#include "Globals.h"
#include "Functions.h"
#include "ODFrameListener.h"
#include "GameMap.h"
#include "TextRenderer.h"
#include "RenderManager.h"
#include "MusicPlayer.h"
#include "SoundEffectsHelper.h"
#include "Gui.h"
#include "ResourceManager.h"

#include "ODApplication.h"

template<> ODApplication*
        Ogre::Singleton<ODApplication>::ms_Singleton = 0;

/*! Initializes the Application along with the ResourceManager
 *
 */
ODApplication::ODApplication() :
        mRoot(0)
{
    new ResourceManager;

    if (!setup())
    {
        return;
    }

    mRoot->startRendering();
}

/*! \brief Returns a reference to the singleton object
 *
 */
ODApplication& ODApplication::getSingleton()
{
    assert (ms_Singleton);
    return (*ms_Singleton);
}

/*! \brief Returns a pointer to the singleton object
 *
 */
ODApplication* ODApplication::getSingletonPtr()
{
    return ms_Singleton;
}

ODApplication::~ODApplication()
{
    if(mRoot)
        delete mRoot;
}

/*! \brief Sets up the application - returns false if the user chooses to abandon
 *  configuration.
 */
bool ODApplication::setup()
{
    ResourceManager* resMgr = ResourceManager::getSingletonPtr();
    mRoot = new Ogre::Root(
            resMgr->getPluginsPath(),
            resMgr->getHomePath() + "ogre.cfg",
            resMgr->getHomePath() + "ogre.log");

    resMgr->setupResources();

    /* Show the configuration dialog and initialise the system
     * TODO: Skip this and use root.restoreConfig()
     * to load configuration settings if we are sure there are valid ones
     * saved in ogre.cfg
     * We should use this later (when we have an own setup options screen)
     * to avoid having the setup dialog started on every run
     */
    if(!mRoot->showConfigDialog())
        return false;

    mWindow = mRoot->initialise(true);
    Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();

    //instanciate all singleton helper classes
    new RenderManager(&gameMap);
    new SoundEffectsHelper();
    new Gui();
    new TextRenderer();
    new MusicPlayer();
    RenderManager::getSingletonPtr()->sceneManager
            = mRoot->createSceneManager(Ogre::ST_EXTERIOR_CLOSE);
    //TODO: Main menu should display without having the map loaded, but
    //      this needs refactoring at some other places, too
    Gui::getSingletonPtr()->loadGuiSheet(Gui::mainMenu);
    TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 140,
                10, 50, 70, Ogre::ColourValue::Green);
    //TODO - move this to when the map is actually loaded
    MusicPlayer::getSingleton().start(0);

    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    createCamera();
    createViewports();
    createScene();

    new ODFrameListener(mWindow, mCamera, true, true, false);
    ODFrameListener::getSingletonPtr()->showDebugOverlay(true);
    mRoot->addFrameListener(ODFrameListener::getSingletonPtr());

    return true;
}

/*! \brief Sets up the main camera
 */
void ODApplication::createCamera()
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->sceneManager;
    mCamera = mSceneMgr->createCamera("PlayerCam");
    mCamera->setNearClipDistance(.05);
    mCamera->setFarClipDistance(300.0);
    mCamera->setAutoTracking(false, mSceneMgr->getRootSceneNode()
            ->createChildSceneNode("CameraTarget"), Ogre::Vector3(0, 0, 0));
}

void ODApplication::createScene()
{
    // Turn on shadows
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);	// Quality 1
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);	// Quality 2
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);	// Quality 3

    /* TODO: move level loading to a better place
     *       (own class to exclude from global skope?)
     *       and generalize it for the future when we have more levels
     */
    // Read in the default game map
    std::string levelPath = ResourceManager::getSingletonPtr()
            ->getResourcePath() + "levels_git/Test.level";
    {
        //Check if the level from git exists. If not, use the standard one.
        std::ifstream file(levelPath.c_str(), std::ios_base::in);
        if (!file.is_open())
        {
            levelPath = ResourceManager::getSingletonPtr()->getResourcePath()
                    + "levels/Test.level";
        }
    }

    gameMap.levelFileName = "Test";
    readGameMapFromFile(levelPath);

    // Create ogre entities for the tiles, rooms, and creatures
    gameMap.createAllEntities();

    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->sceneManager;
    // Create the main scene lights
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.3, 0.36, 0.28));

    // Create the scene node that the camera attaches to
    Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()
            ->createChildSceneNode("CamNode1", Ogre::Vector3(1, -1, 16));
    node->pitch(Ogre::Degree(25), Ogre::Node::TS_WORLD);
    node->roll(Ogre::Degree(30), Ogre::Node::TS_WORLD);
    node->attachObject(mCamera);

    // Create the single tile selection mesh
    Ogre::Entity* ent = mSceneMgr->createEntity("SquareSelector", "SquareSelector.mesh");
    node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "SquareSelectorNode");
    node->translate(Ogre::Vector3(0, 0, 0));
    node->scale(Ogre::Vector3(BLENDER_UNITS_PER_OGRE_UNIT,
            BLENDER_UNITS_PER_OGRE_UNIT, BLENDER_UNITS_PER_OGRE_UNIT));
#if OGRE_VERSION < ((1 << 16) | (6 << 8) | 0)
    ent->setNormaliseNormals(true);
#endif
    node->attachObject(ent);
    Ogre::SceneNode *node2 = node->createChildSceneNode("Hand_node");
    node2->setPosition(0.0 / BLENDER_UNITS_PER_OGRE_UNIT, 0.0
            / BLENDER_UNITS_PER_OGRE_UNIT, 3.0 / BLENDER_UNITS_PER_OGRE_UNIT);
    node2->scale(Ogre::Vector3(1.0 / BLENDER_UNITS_PER_OGRE_UNIT, 1.0
            / BLENDER_UNITS_PER_OGRE_UNIT, 1.0 / BLENDER_UNITS_PER_OGRE_UNIT));

    // Create the light which follows the single tile selection mesh
    Ogre::Light* light = mSceneMgr->createLight("MouseLight");
    light->setType(Ogre::Light::LT_POINT);
    light->setDiffuseColour(Ogre::ColourValue(.5, .7, .6));
    light->setSpecularColour(Ogre::ColourValue(.5, .4, .4));
    light->setPosition(0, 0, 5);
    light->setAttenuation(20, 0.15, 0.15, 0.017);
    node->attachObject(light);
}

/*! \brief setup the viewports
 *
 */
void ODApplication::createViewports()
{
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(
            vp->getActualHeight()));
}

const unsigned int ODApplication::PORT_NUMBER = 31222;
const double ODApplication::BLENDER_UNITS_PER_OGRE_UNIT = 10.0;
const double ODApplication::DEFAULT_FRAMES_PER_SECOND = 60.0;
double ODApplication::MAX_FRAMES_PER_SECOND = DEFAULT_FRAMES_PER_SECOND;
double ODApplication::turnsPerSecond = 1.4;
const std::string ODApplication::VERSION = "0.4.7";
const std::string ODApplication::VERSIONSTRING = "OpenDungeons_Version:" + VERSION;
std::string ODApplication::MOTD = "Welcome to Open Dungeons\tVersion:  " + VERSION;
const std::string ODApplication::POINTER_INFO_STRING = "pointerInfo";
const std::string ODApplication::HELP_MESSAGE = "\
The console is a way of interacting with the underlying game engine directly.\
Commands given to the the are made up of two parts: a \'command name\' and one or more \'arguments\'.\
For information on how to use a particular command, type help followed by the command name.\
\n\nThe following commands are avaliable:\
\n\thelp keys - shows the keyboard controls\
\n\tlist - print out lists of creatures, classes, etc\n\thelp - displays this help screen\n\tsave - saves the current level to a file\
\n\tload - loads a level from a file\
\n\tquit - exit the program\
\n\tmaxmessages - Sets or displays the max number of chat messages to display\
\n\tmaxtime - Sets or displays the max time for chat messages to be displayed\
\n\ttermwidth - set the terminal width\
\n\taddcreature - load a creature into the file.\
\n\taddclass - Define a creature class\
\n\taddtiles - adds a rectangular region of tiles\
\n\tnewmap - Creates a new rectangular map\
\n\trefreshmesh - Reloads the meshes for all the objects in the game\
\n\tmovespeed - sets the camera movement speed\
\n\trotatespeed - sets the camera rotation speed\
\n\tfps - sets the maximum framerate\
\n\tturnspersecond - sets the number of turns the AI will carry out per second\
\n\tmousespeed - sets the mouse speed\
\n\tambientlight - set the ambient light color\
\n\tconnect - connect to a server\
\n\thost - host a server\
\n\tchat - send a message to other people in the game\
\n\tnearclip - sets the near clipping distance\
\n\tfarclip - sets the far clipping distance\
\n\tvisdebug - turns on visual debugging for a creature\
\n\taddcolor - adds another player color\
\n\tsetcolor - changes the value of one of the player's color\
\n\tdisconnect - stops a running server or client and returns to the map editor\
\n\taithreads - sets the maximum number of creature AI threads on the server";
