/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
 *
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ODApplication.h"

#include "render/ODFrameListener.h"
#include "gamemap/GameMap.h"
#include "render/TextRenderer.h"
#include "render/RenderManager.h"
#include "sound/MusicPlayer.h"
#include "sound/SoundEffectsManager.h"
#include "render/Gui.h"
#include "utils/ResourceManager.h"
#include "gamemap/MiniMap.h"
#include "utils/LogManager.h"
#include "camera/CameraManager.h"
#include "scripting/ASWrapper.h"
#include "modes/Console.h"
#include "gamemap/GameMap.h"
#include "utils/Random.h"
#include "entities/MapLight.h"
#include "network/ODServer.h"
#include "network/ODClient.h"

#include <OgreErrorDialog.h>
#include <OgreRoot.h>
#include <Overlay/OgreOverlaySystem.h>
#include <OgreResourceGroupManager.h>

#include <string>
#include <sstream>
#include <fstream>

ODApplication::ODApplication() :
    mRoot(NULL),
    mWindow(NULL)
{
    try
    {
        Random::initialize();

        ResourceManager* resMgr = new ResourceManager;

        std::cout << "Creating OGRE::Root instance; Plugins path: " << resMgr->getPluginsPath()
                  << "; config file: " << resMgr->getCfgFile()
                  << "; log file: " << resMgr->getLogFile() << std::endl;

        mRoot = new Ogre::Root(resMgr->getPluginsPath(),
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
        if (!mRoot->showConfigDialog())
            return;

        // Needed for the TextRenderer and the Render Manager
        mOverlaySystem = new Ogre::OverlaySystem();

        mWindow = mRoot->initialise(true, "OpenDungeons " + VERSION);

        LogManager* logManager = new LogManager();
        logManager->setLogDetail(Ogre::LL_BOREME);

        new ODServer();
        new ODClient();

        //Initialise RTshader system
        // IMPORTANT: This needs to be initialized BEFORE the resource groups.
        // eg: Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
        // but after the render window, eg: mRoot->initialise();
        // This advice was taken from here:
        // http://www.ogre3d.org/forums/viewtopic.php?p=487445#p487445
        if (!Ogre::RTShader::ShaderGenerator::initialize())
        {
            //TODO - exit properly
            LogManager::getSingletonPtr()->logMessage("FATAL:"
                    "Failed to initialize the Real Time Shader System, exiting", Ogre::LML_CRITICAL);
            exit(1);
        }

        Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
        new MusicPlayer();
        new SoundEffectsManager();

        new Gui();
        TextRenderer* textRenderer = new TextRenderer();
        textRenderer->addTextBox("DebugMessages", ODApplication::MOTD.c_str(), 140,
                                    30, 50, 70, Ogre::ColourValue::Green);
        textRenderer->addTextBox(ODApplication::POINTER_INFO_STRING, "",
                                    0, 0, 200, 50, Ogre::ColourValue::White);

        mFrameListener = new ODFrameListener(mWindow, mOverlaySystem);
        mRoot->addFrameListener(mFrameListener);

        mRoot->startRendering();
    }
    catch(const Ogre::Exception& e)
    {
        std::cerr << "An internal Ogre3D error ocurred: " << e.getFullDescription() << std::endl;
        displayErrorMessage("Internal Ogre3D exception: " + e.getFullDescription());
    }
}

ODApplication::~ODApplication()
{
    cleanUp();
}

void ODApplication::displayErrorMessage(const std::string& message, bool log)
{
    if (log)
    {
        LogManager::getSingleton().logMessage(message, Ogre::LML_CRITICAL);
    }
    Ogre::ErrorDialog e;
    e.display(message, LogManager::GAMELOG_NAME);
}

void ODApplication::cleanUp()
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Quitting main application...", Ogre::LML_CRITICAL);

    logManager->logMessage("Stopping server...");
    ODServer::getSingleton().stopServer();
    logManager->logMessage("Disconnecting client...");
    ODClient::getSingleton().disconnect();

    logManager->logMessage("Removing ODFrameListener from root...");
    mRoot->removeFrameListener(mFrameListener);
    logManager->logMessage("Deleting ODFrameListener...");
    delete mFrameListener;
    logManager->logMessage("Deleting Ogre::OverlaySystem...");
    delete mOverlaySystem;
    logManager->logMessage("Deleting MusicPlayer...");
    delete MusicPlayer::getSingletonPtr();
    logManager->logMessage("Deleting TextRenderer...");
    delete TextRenderer::getSingletonPtr();
    logManager->logMessage("Deleting Gui...");
    delete Gui::getSingletonPtr();
    logManager->logMessage("Deleting SoundEffectsManager...");
    delete SoundEffectsManager::getSingletonPtr();
    logManager->logMessage("Deleting ODServer...");
    delete ODServer::getSingletonPtr();
    logManager->logMessage("Deleting ODClient...");
    delete ODClient::getSingletonPtr();
    logManager->logMessage("Deleting ResourceManager...");
    delete ResourceManager::getSingletonPtr();
    logManager->logMessage("Deleting LogManager and Ogre::Root...");
    delete LogManager::getSingletonPtr();
    delete mRoot;
}

//TODO: find some better places for some of these
const unsigned int ODApplication::PORT_NUMBER = 31222;
const double ODApplication::DEFAULT_FRAMES_PER_SECOND = 60.0;
double ODApplication::MAX_FRAMES_PER_SECOND = DEFAULT_FRAMES_PER_SECOND;
double ODApplication::turnsPerSecond = 1.4;
#ifdef OD_VERSION
const std::string ODApplication::VERSION = OD_VERSION;
#else
const std::string ODApplication::VERSION = "undefined";
#endif
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
\n\taddgold - Gives gold to one player\
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
\n\tdisconnect - stops a running server or client and returns to the map editor\
+\n\taithreads - sets the maximum number of creature AI threads on the server\
+\n\tsetdest - Sets the creature destination\
+\n\tlistanims - List all the animations for the given mesh\
+\n\tlogfloodfill - Displays the FloodFillValues of all the Tiles in the GameMap";
