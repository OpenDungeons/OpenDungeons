#include <string>
#include <sstream>
#include <fstream>

#include <CEGUI.h>

#include "Globals.h"
#include "Functions.h"
#include "ExampleFrameListener.h"
#include "Tile.h"
#include "Network.h"
#include "GameMap.h"
#include "TextRenderer.h"
#include "RenderManager.h"
#include "Gui.h"

#include "OpenDungeonsApplication.h"

OpenDungeonsApplication::OpenDungeonsApplication()
{
    mFrameListener = 0;
    mRoot = 0;
    // Provide a nice cross platform solution for locating the configuration files
    // On windows files are searched for in the current working directory, on OS X however
    // you must provide the full path, the helper function macBundlePath does this for us.
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    mResourcePath = macBundlePath() + "/Contents/Resources/";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX //Actually this can be other things than linux as well
    //Get path of data
    char* path = std::getenv("OPENDUNGEONS_DATA_PATH");
    if (path)
    {
        mResourcePath = path;
        if (*mResourcePath.end() != '/') //Make sure we have trailing slash
        {
            mResourcePath.append("/");
        }
        //Getenv return value should not be touched/freed.
    }
    else
    {
        mResourcePath = "";
    }
#else
    mResourcePath = "";
#endif
    mHomePath = getHomePath();
}

OpenDungeonsApplication::~OpenDungeonsApplication()
{
    if (mFrameListener)
        delete mFrameListener;
    if (mRoot)
        delete mRoot;
}

void OpenDungeonsApplication::go(void)
{
    if (!setup())
        return;

    mRoot->startRendering();
    //destroyScene();
}

/*! \brief Sets up the application - returns false if the user chooses to abandon
 *  configuration.
 */
bool OpenDungeonsApplication::setup(void)
{
    Ogre::String pluginsPath;
    // only use plugins.cfg if not static
#ifndef OGRE_STATIC_LIB
    pluginsPath = mResourcePath + "plugins.cfg";
#endif

    mRoot = new Ogre::Root(pluginsPath, mHomePath + "ogre.cfg", mHomePath
            + "Ogre.log");

    setupResources();

    bool carryOn = configure();
    if (!carryOn)
        return false;

    chooseSceneManager();
    createCamera();
    createViewports();

    // Set default mipmap level (NB some APIs ignore this)
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Load resources
    loadResources();

    createScene();

    createFrameListener();

    return true;

}

/*! Configures the application - returns false if the user chooses to abandon
 *  configuration.
 */
bool OpenDungeonsApplication::configure(void)
{
    // Show the configuration dialog and initialise the system
    // You can skip this and use root.restoreConfig() to load configuration
    // settings if you were sure there are valid ones saved in ogre.cfg
    if (mRoot->showConfigDialog())
    {
        // If returned true, user clicked OK so initialise
        // Here we choose to let the system create a default rendering window by passing 'true'
        mWindow = mRoot->initialise(true);
        return true;
    }
    else
    {
        return false;
    }
}

void OpenDungeonsApplication::createCamera(void)
{
    // Set up the main camera
    mCamera = mSceneMgr->createCamera("PlayerCam");
    mCamera->setNearClipDistance(.05);
    mCamera->setFarClipDistance(300.0);
    mCamera->setAutoTracking(false,
            mSceneMgr->getRootSceneNode()->createChildSceneNode("CameraTarget"),
            Ogre::Vector3(0, 0, 0));
}

void OpenDungeonsApplication::createScene(void)
{
    // Turn on shadows
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);	// Quality 1
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_MODULATIVE);	// Quality 2
    //mSceneMgr->setShadowTechnique(SHADOWTYPE_STENCIL_ADDITIVE);	// Quality 3

    // TODO: replace mResourcePath by using ogre resource manager
    // TODO: load main menu after the below task is done. Where?
    /* TODO: These: Gui, TextRenderer, MusicPlayer, RenderManager,
     *       SoundEffectsHelper should be instanciated earlier
     */
    new SoundEffectsHelper();
    new RenderManager();
    new Gui();
    new TextRenderer();
    new MusicPlayer();

    SoundEffectsHelper::getSingletonPtr()->initialiseSound(mResourcePath + "sounds");
    RenderManager::getSingletonPtr()->initialize(mSceneMgr, &gameMap);
    Gui::getSingletonPtr()->loadGuiSheet(Gui::ingameMenu);
    MusicPlayer::getSingletonPtr()->load(mResourcePath + "music/");
    TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 140,
            10, 50, 70, Ogre::ColourValue::Green);

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

void OpenDungeonsApplication::createFrameListener(void)
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

void OpenDungeonsApplication::chooseSceneManager(void)
{
    // Use the terrain scene manager.
    mSceneMgr = mRoot->createSceneManager(Ogre::ST_EXTERIOR_CLOSE);
}

void OpenDungeonsApplication::createViewports(void)
{
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(
            vp->getActualHeight()));
}

/*  \brief Method which will define the source of resources
 * (other than current folder)
 */
void OpenDungeonsApplication::setupResources(void)
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load(mResourcePath + "resources.cfg");

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            //Prefix resource path to resource locations.
            archName = mResourcePath + i->second;
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            // OS X does not set the working directory relative to the app,
            // In order to make things portable on OS X we need to provide
            // the loading with it's own bundle path location
            ResourceGroupManager::getSingleton().addResourceLocation(
                    String(macBundlePath() + "/" + archName), typeName, secName);
#else
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
#endif
        }
    }
}

/// Must at least do ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
void OpenDungeonsApplication::loadResources(void)
{
    // Initialise, parse scripts etc
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

Ogre::String OpenDungeonsApplication::getHomePath()
{
    Ogre::String homePath;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    //Currently using run dir in windows.
    /*      TCHAR szPath[MAX_PATH];
     HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath);
     if(SUCCEEDED(result))
     {
     PathAppend(szPath, _T("\\OpenDungeons"));


     int len = _tcslen(szPath);
     char* sZto = new char[len];
     //TODO - check what encoding to use.
     WideCharToMultiByte(CP_UTF8, 0, szPath, -1, sZto, len, NULL, NULL);

     homePath = sZto;

     _stat statbuf;
     int status = stat(homePath.c_str(), &statbuf);
     if(status != 0)
     {
     int dirCreateStatus;
     dirCreateStatus = mkdir(homePath.c_str());
     }

     delete[] sZto;
     homePath.append("\\");
     }
     else
     {
     homePath = ".";
     }*/
    homePath = "./";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

    //If variable is not set, assume we are in a build dir and
    //use the current dir for config files.
    char* useHomeDir = std::getenv("OPENDUNGEONS_DATA_PATH");
    if (useHomeDir)
    {
        //On linux and similar, use home dir
        //http://linux.die.net/man/3/getpwuid
        char* path = std::getenv("HOME");
        if (path)
        {
            homePath = path;
        }
        else //In the unlikely event that home is not defined, use current  working dir
        {
            homePath = ".";
        }
        homePath.append("/.OpenDungeons");

        //Create directory if it doesn't exist
        struct stat statbuf;
        int status = stat(homePath.c_str(), &statbuf);
        if (status != 0)
        {
            int dirCreateStatus;
            dirCreateStatus = mkdir(homePath.c_str(), S_IRWXU | S_IRWXG
                    | S_IROTH | S_IXOTH);

        }

        homePath.append("/");
    }
    else
    {
        homePath = "./";
    }
#else
    homePath = "./";
#endif

    return homePath;
}
