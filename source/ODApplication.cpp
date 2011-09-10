/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
 */

#include <string>
#include <sstream>
#include <fstream>

#include <OgreErrorDialog.h>

#include "ODFrameListener.h"
#include "GameMap.h"
#include "TextRenderer.h"
#include "RenderManager.h"
#include "MusicPlayer.h"
#include "SoundEffectsHelper.h"
#include "Gui.h"
#include "ResourceManager.h"
#include "MiniMap.h"
#include "LogManager.h"
#include "Translation.h"
#include "GameState.h"
#include "CameraManager.h"
#include "ASWrapper.h"
#include "Console.h"
#include "GameMap.h"
#include "Random.h"
#include "MapLight.h"
#include "MissileObject.h"
#include "ServerNotification.h"
#include "ClientNotification.h"

#include "ODApplication.h"

template<> ODApplication*
        Ogre::Singleton<ODApplication>::ms_Singleton = 0;

/*! Initializes the Application along with the ResourceManager
 *
 */
ODApplication::ODApplication() :
        root(0),
        window(0)
{
    sem_init(&MapLight::lightNumberLockSemaphore, 0, 1);
    sem_init(&MissileObject::missileObjectUniqueNumberLockSemaphore, 0, 1);
    sem_init(&ServerNotification::serverNotificationQueueSemaphore, 0, 0);
    sem_init(&ClientNotification::clientNotificationQueueSemaphore, 0, 0);
    sem_init(&ServerNotification::serverNotificationQueueLockSemaphore, 0, 1);
    sem_init(&ClientNotification::clientNotificationQueueLockSemaphore, 0, 1);
    sem_init(&GameMap::creatureAISemaphore, 0, 1);

    Random::initialize();

    ResourceManager* resMgr = new ResourceManager;
    root = new Ogre::Root(
            resMgr->getPluginsPath(),
            resMgr->getCfgFile(),
            resMgr->getLogFile());

    resMgr->setupResources();

    /* TODO: Skip this and use root.restoreConfig()
     * to load configuration settings if we are sure there are valid ones
     * saved in ogre.cfg
     * We should use this later (when we have an own setup options screen)
     * to avoid having the setup dialog started on every run
     */
    /* TODO: create our own options menu and define good default values
     *       (drop smaller than 800x600, AA, shadow quality, mipmaps, etc)
     */
    if (!root->showConfigDialog())
    {
        return;
    }

    window = root->initialise(true, "OpenDungeons " + VERSION);

    LogManager* logManager = new LogManager();
    logManager->setLogDetail(Ogre::LL_BOREME);
    new Translation();
    new GameState();
    RenderManager* renderMgr = new RenderManager();
    Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
    new SoundEffectsHelper();
    new Gui();
    new TextRenderer();
    new MusicPlayer();
    //TODO: Main menu should display without having the map loaded, but
    //      this needs refactoring at some other places, too
    Gui::getSingletonPtr()->loadGuiSheet(Gui::mainMenu);
    TextRenderer::getSingleton().addTextBox("DebugMessages", MOTD.c_str(), 140,
                10, 50, 70, Ogre::ColourValue::Green);
    //TODO - move this to when the map is actually loaded
    MusicPlayer::getSingleton().start(0);

    //TODO: this should not be created here.
    gameMap = new GameMap;
    renderMgr->setGameMap(gameMap);

    //FIXME: do this only if a level loads after the main menu
    //Try to create the camera, viewport and scene. 
    try
    {
        logManager->logMessage("Creating camera...", Ogre::LML_NORMAL);
        renderMgr->createCamera();
        logManager->logMessage("Creating viewpoerts...", Ogre::LML_NORMAL);
        renderMgr->createViewports();
        logManager->logMessage("Creating scene...", Ogre::LML_NORMAL);
        renderMgr->createScene();
    }
    catch(Ogre::Exception& e)
    {
        displayErrorMessage("Ogre exception when ininialising the render manager:\n"
            + e.getFullDescription(), false);
        cleanUp();
        return;
    }
    catch (std::exception& e)
    {
        displayErrorMessage("Exception when ininialising the render manager:\n"
            + std::string(e.what()), false);
        cleanUp();
        return;
    }

    new CameraManager(renderMgr->getCamera());
    logManager->logMessage("Creating frame listener...", Ogre::LML_NORMAL);
    root->addFrameListener(new ODFrameListener(window, gameMap));
    //TODO: This should be moved once we have separated level loading from startup.
    

    //FIXME: This should be at a better place (when level loads for the first time)
    //new MiniMap;

    //FIXME: Is this the best place for instanciating these two?
    //Console needs to exist BEFORE ASWrapper because it needs it for callback
    new Console();
    new ASWrapper();
    try
    {
        root->startRendering();
    }
    catch(Ogre::Exception& e)
    {
        displayErrorMessage("Ogre exception:\n"
            + e.getFullDescription());
        cleanUp();
        return;
    }
    catch(std::exception& e)
    {
        displayErrorMessage("Exception:\n"
            + std::string(e.what()));
        cleanUp();
        return;
    }
    //Moved out from cleanup, as we only want to remove it if it exists.
    root->removeFrameListener(ODFrameListener::getSingletonPtr());
    cleanUp();
}

ODApplication::~ODApplication()
{
    if(root)
    {
        delete root;
    }
}

/*! \brief Display a GUI error message
 *
 */
void ODApplication::displayErrorMessage(const std::string& message, bool log)
{
    if(log)
    {
        LogManager::getSingleton().logMessage(message, Ogre::LML_CRITICAL);
    }
    Ogre::ErrorDialog e;
    e.display(message, LogManager::GAMELOG_NAME);
}


/*! \brief Delete the various singleton objects and clean up other stuff
 *
 */
void ODApplication::cleanUp()
{
    delete MiniMap::getSingletonPtr();

    delete ODFrameListener::getSingletonPtr();
    delete MusicPlayer::getSingletonPtr();
    delete TextRenderer::getSingletonPtr();
    delete Gui::getSingletonPtr();
    delete SoundEffectsHelper::getSingletonPtr();
    delete RenderManager::getSingletonPtr();
    delete GameState::getSingletonPtr();
    delete Translation::getSingletonPtr();
    delete LogManager::getSingletonPtr();
    delete CameraManager::getSingletonPtr();
    delete Console::getSingletonPtr();
    delete ASWrapper::getSingletonPtr();
    delete gameMap;
}

//TODO: find some better places for some of these
const unsigned int ODApplication::PORT_NUMBER = 31222;
const double ODApplication::DEFAULT_FRAMES_PER_SECOND = 60.0;
double ODApplication::MAX_FRAMES_PER_SECOND = DEFAULT_FRAMES_PER_SECOND;
double ODApplication::turnsPerSecond = 1.4;
const std::string ODApplication::VERSION = "0.4.8";
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
