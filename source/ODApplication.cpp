/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include "render/TextRenderer.h"
#include "render/RenderManager.h"
#include "sound/MusicPlayer.h"
#include "sound/SoundEffectsManager.h"
#include "render/Gui.h"
#include "utils/ResourceManager.h"
#include "utils/LogManager.h"
#include "utils/Random.h"
#include "utils/ConfigManager.h"
#include "network/ODServer.h"
#include "network/ODClient.h"

#include <OgreErrorDialog.h>
#include <OgreGpuProgramManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <Overlay/OgreOverlaySystem.h>
#include <RTShaderSystem/OgreShaderGenerator.h>

#include <string>
#include <sstream>
#include <fstream>

ODApplication::ODApplication()
{
    Random::initialize();
    //NOTE: The order of initialisation of the different "manager" classes is important,
    //as many of them depen on each other.
    ResourceManager resMgr;
    std::cout << "Creating OGRE::Root instance; Plugins path: " << resMgr.getPluginsPath()
              << "; config file: " << resMgr.getCfgFile()
              << "; log file: " << resMgr.getLogFile() << std::endl;

    Ogre::Root ogreRoot(resMgr.getPluginsPath(),
                            resMgr.getCfgFile(),
                            resMgr.getLogFile());

    resMgr.setupOgreResources();

    LogManager logManager;
    logManager.setLogDetail(Ogre::LL_BOREME);
    ConfigManager configManager;

    /* TODO: Skip this and use root.restoreConfig()
      * to load configuration settings if we are sure there are valid ones
      * saved in ogre.cfg
      * We should use this later (when we have an own setup options screen)
      * to avoid having the setup dialog started on every run
      */
    /* TODO: create our own options menu and define good default values
      *       (drop smaller than 800x600, AA, shadow quality, mipmaps, etc)
      */
    if (!ogreRoot.showConfigDialog())
        return;

    // Needed for the TextRenderer and the Render Manager
    Ogre::OverlaySystem overlaySystem;

    Ogre::RenderWindow* renderWindow = ogreRoot.initialise(true, "OpenDungeons " + VERSION);

    ODServer server;
    ODClient client;

    //NOTE: This is currently done here as it has to be done after initialising mRoot,
    //but before running initialiseAllResourceGroups()
    Ogre::GpuProgramManager& gpuProgramManager = Ogre::GpuProgramManager::getSingleton();
    Ogre::ResourceGroupManager& resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();
    if(gpuProgramManager.isSyntaxSupported("glsl"))
    {
        //Add GLSL shader location for RTShader system
        resourceGroupManager.addResourceLocation(
                    "materials/RTShaderLib/GLSL", "FileSystem", "Graphics");
        //Use patched version of shader on shader version 130+ systems
        Ogre::uint16 shaderVersion = ogreRoot.getRenderSystem()->getNativeShadingLanguageVersion();
        logManager.logMessage("Shader version is: " + Ogre::StringConverter::toString(shaderVersion));
        if(shaderVersion >= 130)
        {
            resourceGroupManager.addResourceLocation("materials/RTShaderLib/GLSL/130", "FileSystem", "Graphics");
        }
        else
        {
            resourceGroupManager.addResourceLocation("materials/RTShaderLib/GLSL/120", "FileSystem", "Graphics");
        }
    }

    //Initialise RTshader system
    // IMPORTANT: This needs to be initialized BEFORE the resource groups.
    // eg: Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
    // but after the render window, eg: mRoot->initialise();
    // This advice was taken from here:
    // http://www.ogre3d.org/forums/viewtopic.php?p=487445#p487445
    if (!Ogre::RTShader::ShaderGenerator::initialize())
    {
        //TODO - exit properly
        logManager.logMessage("FATAL:"
                "Failed to initialize the Real Time Shader System, exiting", Ogre::LML_CRITICAL);
        return;
    }

    Ogre::ResourceGroupManager::getSingletonPtr()->initialiseAllResourceGroups();
    MusicPlayer musicPlayer;
    SoundEffectsManager soundEffectsManager;

    Gui gui;
    TextRenderer textRenderer;
    textRenderer.addTextBox("DebugMessages", ODApplication::MOTD.c_str(), 140,
                                30, 50, 70, Ogre::ColourValue::Green);
    textRenderer.addTextBox(ODApplication::POINTER_INFO_STRING, "",
                                0, 0, 200, 50, Ogre::ColourValue::White);

    ODFrameListener frameListener(renderWindow, &overlaySystem);

    ogreRoot.addFrameListener(&frameListener);
    ogreRoot.startRendering();

    logManager.logMessage("Stopping server...");
    server.stopServer();
    logManager.logMessage("Disconnecting client...");
    client.disconnect();
    ogreRoot.removeFrameListener(&frameListener);
}

ODApplication::~ODApplication()
{
}

void ODApplication::displayErrorMessage(const std::string& message, LogManager& logger)
{
    logger.logMessage(message, Ogre::LML_CRITICAL);
    Ogre::ErrorDialog e;
    e.display(message, LogManager::GAMELOG_NAME);
}

//TODO: find some better places for some of these
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
