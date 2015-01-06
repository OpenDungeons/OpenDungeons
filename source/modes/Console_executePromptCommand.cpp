/*!
 * \file   Console_executePromptCommand.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
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

#include "modes/Console.h"
#include "render/RenderManager.h"
#include "gamemap/MapLoader.h"
#include "camera/CameraManager.h"
#include "ODApplication.h"
#include "game/Player.h"
#include "goals/AllGoals.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "network/ODClient.h"
#include "utils/ResourceManager.h"
#include "utils/ConfigManager.h"
#include "entities/Creature.h"
#include "entities/Weapon.h"
#include "modes/ODConsoleCommand.h"
#include "render/ODFrameListener.h"

#include <OgreLogManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>

#include <sstream>

bool Console::executePromptCommand(const std::string& command, std::string arguments)
{
    std::stringstream tempSS;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->mGameMap;

    // Exit the program
    if (command.compare("quit") == 0 || command.compare("exit") == 0)
    {
        frameListener->requestExit();
    }

    // Set the ambient light color
    else if (command.compare("ambientlight") == 0)
    {
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        if (!arguments.empty())
        {
            Ogre::Real tempR, tempG, tempB = 0.0;
            tempSS.str(arguments);
            tempSS >> tempR >> tempG >> tempB;
            mSceneMgr->setAmbientLight(Ogre::ColourValue(tempR, tempG, tempB));
            frameListener->mCommandOutput += "\nAmbient light set to:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempR) + "    Green:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempG) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempB) + "\n";

        }
        else
        {
            Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
            frameListener->mCommandOutput += "\nCurrent ambient light is:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.r)
                    + "    Green:  " + Ogre::StringConverter::toString(
                    (Ogre::Real) curLight.g) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.b) + "\n";
        }
    }

    // Print the help message
    else if (command.compare("help") == 0)
    {
        frameListener->mCommandOutput += (!arguments.empty())
                ? "\nHelp for command:  " + arguments + "\n\n" + getHelpText(arguments) + "\n"
                : "\n" + ODApplication::HELP_MESSAGE + "\n";
    }

    // Set max frames per second
    else if (command.compare("fps") == 0)
    {
        //NOTE: converted to AS
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            ODApplication::MAX_FRAMES_PER_SECOND = tempDouble;
            frameListener->mCommandOutput += "\nMaximum framerate set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent maximum framerate is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
    }
    // Set near clip distance
    else if (command.compare("nearclip") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            frameListener->setActiveCameraNearClipDistance(tempDouble);
            frameListener->mCommandOutput += "\nNear clip distance set to "
                    + Ogre::StringConverter::toString(
                            frameListener->getActiveCameraNearClipDistance())
                    + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent near clip distance is "
                    + Ogre::StringConverter::toString(
                            frameListener->getActiveCameraNearClipDistance())
                    + "\n";
        }
    }

    // Set far clip distance
    else if (command.compare("farclip") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            frameListener->setActiveCameraFarClipDistance(tempDouble);
            frameListener->mCommandOutput += "\nFar clip distance set to "
                    + Ogre::StringConverter::toString(
                            frameListener->getActiveCameraFarClipDistance()) + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent far clip distance is "
                    + Ogre::StringConverter::toString(
                            frameListener->getActiveCameraFarClipDistance()) + "\n";
        }
    }

    // Add a new instance of a creature to the current map.  The argument is
    // read as if it were a line in a .level file.
    else if (command.compare("addcreature") == 0)
    {
        if (!arguments.empty())
        {
            // Creature the creature and add it to the gameMap
            // TODO : ask the server to do that
            std::stringstream tempSS(arguments);
            Creature *tempCreature = Creature::getCreatureFromStream(gameMap, tempSS);

            tempCreature->createMesh();
            gameMap->addCreature(tempCreature);
            frameListener->mCommandOutput += "\nCreature added successfully\n";
        }
    }

    // Print out various lists of information, the creatures in the
    // scene, the levels available for loading, etc
    else if (command.compare("list") == 0 || command.compare("ls") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str("");

            if (arguments.compare("creatures") == 0)
            {
                tempSS << "Class:\tCreature name:\tLocation:\tColor:\tLHand:\tRHand\n\n";
                for (unsigned int i = 0; i < gameMap->numCreatures(); ++i)
                {
                    tempSS << gameMap->getCreature(i) << std::endl;
                }
            }

            else if (arguments.compare("classes") == 0)
            {
                tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
                for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
                {
                    const CreatureDefinition *currentClassDesc = gameMap->getClassDescription(i);
                    tempSS << currentClassDesc << "\n";
                }
            }

            else if (arguments.compare("players") == 0)
            {
                // There are only players if we are connected.
                if (frameListener->isConnected())
                {
                    tempSS << "Player:\tNick:\tSeatId\tTeamId:\n\n";
                    tempSS << "me\t\t" << gameMap->getLocalPlayer()->getNick() << "\t"
                            << gameMap->getLocalPlayer()->getSeat()->getId() << "\t"
                            << gameMap->getLocalPlayer()->getSeat()->getTeamId() << "\n\n";
                    for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
                    {
                        const Player *currentPlayer = gameMap->getPlayer(i);
                        tempSS << i << "\t\t" << currentPlayer->getNick() << "\t"
                                << currentPlayer->getSeat()->getId() << "\t"
                                << currentPlayer->getSeat()->getTeamId() << "\n\n";
                    }
                }
                else
                {
                    tempSS << "You must either host or join a game before you can list the players in the game.\n";
                }
            }

            else if (arguments.compare("network") == 0)
            {
                if (ODClient::getSingleton().isConnected())
                {
                    tempSS << "You are currently connected to a server.";
                }

                if (ODServer::getSingleton().isConnected())
                {
                    tempSS << "You are currently acting as a server.";
                }

                // FIXME: This is plain wrong as it will have to test for the application mode.
                //if (!frameListener->isConnected())
                //{
                //    tempSS << "You are currently in the map editor.";
                //}
            }

            else if (arguments.compare("rooms") == 0)
            {
                tempSS << "Name:\tSeat Id:\tNum tiles:\n\n";
                for (unsigned int i = 0; i < gameMap->numRooms(); ++i)
                {
                    Room *currentRoom;
                    currentRoom = gameMap->getRoom(i);
                    tempSS << currentRoom->getName() << "\t" << currentRoom->getSeat()->getId()
                            << "\t" << currentRoom->numCoveredTiles() << "\n";
                }
            }

            else if (arguments.compare("colors") == 0 || arguments.compare("colours") == 0)
            {
                tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
                const std::vector<Seat*> seats = gameMap->getSeats();
                for (Seat* seat : seats)
                {
                    Ogre::ColourValue color = seat->getColorValue();

                    tempSS << "\n" << seat->getId() << "\t\t" << color.r
                           << "\t\t" << color.g << "\t\t" << color.b;
                }
            }

            // Loop over level directory and display only level files
            else if (arguments.compare("levels") == 0)
            {
                std::vector<string> tempVector;
                size_t found;
                size_t found2;
                string suffix = ".level";
                string suffix2 = ".level.";
                tempVector = ResourceManager::getSingletonPtr()->
                        listAllFiles("./levels/");
                for (unsigned int j = 0; j < tempVector.size(); ++j)
                {
                    found = tempVector[j].find(suffix);
                    found2 = tempVector[j].find(suffix2);
                    if (found != string::npos && (!(found2 != string::npos)))
                    {
                        tempSS << tempVector[j] << std::endl;
                    }
                }
            }

            else if (arguments.compare("goals") == 0)
            {
                if (frameListener->isConnected())
                {
                    // Loop over the list of unmet goals for the seat we are sitting in an print them.
                    tempSS
                            << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i < gameMap->getLocalPlayer()->getSeat()->numUncompleteGoals(); ++i)
                    {
                        Seat* s = gameMap->getLocalPlayer()->getSeat();
                        Goal* tempGoal = s->getUncompleteGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getDescription(s) << "\n";
                    }

                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS
                            << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i
                            < gameMap->getLocalPlayer()->getSeat()->numCompletedGoals(); ++i)
                    {
                        Seat* s = gameMap->getLocalPlayer()->getSeat();
                        Goal *tempGoal = s->getCompletedGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getSuccessMessage(s) << "\n";
                    }
                }
                else
                {
                    tempSS << "\n\nERROR: You do not have any goals to meet until you host or join a game.\n\n";
                }
            }

            else
            {
                tempSS << "ERROR:  Unrecognized list.  Type \"list\" with no arguments to see available lists.";
            }

            frameListener->mCommandOutput += "+\n" + tempSS.str() + "\n";
        }
        else
        {
            frameListener->mCommandOutput
                    += "lists available:\n\t\tclasses\tcreatures\tplayers\n\t\tnetwork\trooms\tcolors\n\t\tgoals\tlevels\n";
        }
    }

    /*
    // Set chat message variables
    else if (command.compare("maxtime") == 0)
    {
        //NOTE: Converted to AS
        if (!arguments.empty())
        {
            chatMaxTimeDisplay = atoi(arguments.c_str());
            tempSS << "Max display time for chat messages was changed to: "
                    << arguments;
        }

        else
        {
            tempSS << "Max display time for chat messages is: "
                    << chatMaxTimeDisplay;
        }

        frameListener->mCommandOutput += "\n " + tempSS.str() + "\n";
    } */

    /*
    else if (command.compare("maxmessages") == 0)
    {
        //NOTE: converted to as
        if (!arguments.empty())
        {
            chatMaxMessages = atoi(arguments.c_str());
            tempSS << "Max chat messages to display has been set to: "
                    << arguments;
        }
        else
        {
            tempSS << "Max chat messages to display is: " << chatMaxMessages;
        }

        frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
    } */

    // Connect to a server
    else if (command.compare("connect") == 0)
    {
        // Make sure we have set a nickname.
        if (!gameMap->getLocalPlayer()->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!frameListener->isConnected())
            {
                // Make sure an IP address to connect to was provided
                if (!arguments.empty())
                {
                    tempSS.str(arguments);
                    std::string ip;
                    tempSS >> ip;

                    if (ODClient::getSingleton().connect(ip, ConfigManager::getSingleton().getNetworkPort()))
                    {
                        frameListener->mCommandOutput += "\nConnection successful.\n";
                    }
                    else
                    {
                        frameListener->mCommandOutput += "\nConnection failed!\n";
                    }
                }
                else
                {
                    frameListener->mCommandOutput
                            += "\nYou must specify the IP address of the server you want to connect to and the level map.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1\n";
                }

            }
            else
            {
                frameListener->mCommandOutput
                        += "\nYou are already connected to a server.  You must disconnect before you can connect to a new game.\n";
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can join a server.\n";
        }

        frameListener->mCommandOutput += "\n";

    }

    // Send a chat message
    else if (command.compare("chat") == 0)
    {
        if (ODClient::getSingleton().isConnected())
        {
            ODClient::getSingleton().queueClientNotification(ClientNotification::chat, arguments);
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("creaturevisdebug") == 0)
    {
        if (ODServer::getSingleton().isConnected())
        {
            if (arguments.length() > 0)
            {
                // Activate visual debugging
                Creature *tempCreature = gameMap->getCreature(arguments);
                if (tempCreature != nullptr)
                {
                    if (!tempCreature->getHasVisualDebuggingEntities())
                    {
                        if(ODClient::getSingleton().isConnected())
                        {
                            const std::string& name = tempCreature->getName();
                            ODConsoleCommand* cc = new ODConsoleCommandDisplayCreatureVisualDebug(name, true);
                            ODServer::getSingleton().queueConsoleCommand(cc);
                        }
                        frameListener->mCommandOutput
                                += "\nVisual debugging entities created for creature:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        if(ODClient::getSingleton().isConnected())
                        {
                            const std::string& name = tempCreature->getName();
                            ODConsoleCommand* cc = new ODConsoleCommandDisplayCreatureVisualDebug(name, false);
                            ODServer::getSingleton().queueConsoleCommand(cc);
                        }
                        frameListener->mCommandOutput
                                += "\nVisual debugging entities destroyed for creature:  "
                                        + arguments + "\n";
                    }
                }
                else
                {
                    frameListener->mCommandOutput
                            += "\nCould not create visual debugging entities for creature:  "
                                    + arguments + "\n";
                }
            }
            else
            {
                frameListener->mCommandOutput
                        += "\nERROR:  You must supply a valid creature name to create debug entities for.\n";
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("seatvisdebug") == 0)
    {
        if (ODServer::getSingleton().isConnected())
        {
            if (arguments.length() > 0)
            {
                tempSS.str(arguments);
                int seatId;
                tempSS >> seatId;
                // Activate visual debugging
                Seat* seat = gameMap->getSeatById(seatId);
                if (seat != nullptr)
                {
                    if (!seat->getIsDebuggingVision())
                    {
                        if(ODClient::getSingleton().isConnected())
                        {
                            ODConsoleCommand* cc = new ODConsoleCommandDisplaySeatVisualDebug(seatId, true);
                            ODServer::getSingleton().queueConsoleCommand(cc);
                        }
                        frameListener->mCommandOutput
                                += "\nVisual debugging entities created for seat:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        if(ODClient::getSingleton().isConnected())
                        {
                            ODConsoleCommand* cc = new ODConsoleCommandDisplaySeatVisualDebug(seatId, false);
                            ODServer::getSingleton().queueConsoleCommand(cc);
                        }
                        frameListener->mCommandOutput
                                += "\nVisual debugging entities destroyed for seat:  "
                                        + arguments + "\n";
                    }
                }
                else
                {
                    frameListener->mCommandOutput
                            += "\nCould not create visual debugging entities for seat:  "
                                    + arguments + "\n";
                }
            }
            else
            {
                frameListener->mCommandOutput
                        += "\nERROR:  You must supply a valid seat id debug vision for.\n";
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("icanseedeadpeople") == 0)
    {
        if (ODServer::getSingleton().isConnected())
        {
            if(ODClient::getSingleton().isConnected())
            {
                ODConsoleCommand* cc = new ODConsoleCommandAskToggleFOW();
                ODServer::getSingleton().queueConsoleCommand(cc);
            }
            frameListener->mCommandOutput
                    += "\nAsking to toggle fog of war\n";
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nERROR:  You can toggle fog of war only when you are hosting a game.\n";
        }
    }

    // Changes the level of a given creature
    else if (command.compare("setcreaturelevel") == 0)
    {
        if (ODServer::getSingleton().isConnected())
        {
            if (arguments.length() > 0)
            {
                tempSS.str(arguments);
                std::string name;
                int level;
                tempSS >> name;
                tempSS >> level;
                // Activate visual debugging
                if(ODClient::getSingleton().isConnected())
                {
                    ODConsoleCommand* cc = new ODConsoleCommandSetLevelCreature(name, level);
                    ODServer::getSingleton().queueConsoleCommand(cc);
                }
                frameListener->mCommandOutput
                        += "\nCommand sent to change creature level:  "
                                + arguments + "\n";
            }
            else
            {
                frameListener->mCommandOutput
                        += "\nERROR:  You must supply a valid creature name.\n";
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nERROR:  Only the server can change a creature level.\n";
        }
    }
    else if (command.compare("circlearound") == 0)
    {
        //FIXME: Shall we keep this?
        if(!arguments.empty())
        {
            CameraManager* cm = frameListener->getCameraManager();
            double centerX;
            double centerY;
            double radius;

            tempSS.str(arguments);
            tempSS >> centerX >> centerY >> radius;
            // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies
            cm->setCircleCenter((int)centerX, (int)centerY);
            cm->setCircleRadius((unsigned int)radius);
            cm->setCircleMode(true);
        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radius";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("catmullspline") == 0)
    {
        // FIXME: Shall we keep this?
        if(!arguments.empty())
        {
            CameraManager* cm = frameListener->getCameraManager();
            int nn = 0;

            int tempInt1 = 0;
            int tempInt2 = 0;

            tempSS.str(arguments);
            tempSS >> nn;
            cm->resetHCSNodes(nn);

            for (int ii = 0 ; ii < nn ; ++ii)
            {
                tempSS >> tempInt1;
                tempSS >> tempInt2;

                //std::cout << "tempInt1 " <<  tempInt1 << std::endl;
                //std::cout << "tempInt2 " <<  tempInt2 << std::endl;
                cm->addHCSNodes(tempInt1, tempInt2);
            }

            // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies

            cm->setCatmullSplineMode(true);
        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radious";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("setcamerafovy") == 0)
    {
        // FIXME: Shall we keep this?
        if(!arguments.empty())
        {
            CameraManager* cm = frameListener->getCameraManager();
            double tmp;
            tempSS.str(arguments);
            tempSS >> tmp;
            Ogre::Radian radianAngle((Ogre::Real)tmp);

            cm->mActiveCamera->setFOVy(radianAngle);
            // TODO check the for the maximal and minimal value of setFoVy
        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  you need to specify an angle in radians ";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("addgold") == 0)
    {
        int seatId, gold;
        tempSS.str(arguments);
        tempSS >> seatId >> gold;
        if(ODServer::getSingleton().isConnected())
        {
            ODConsoleCommand* cc = new ODConsoleCommandAddGold(gold, seatId);
            ODServer::getSingleton().queueConsoleCommand(cc);
        }
        else
        {
            frameListener->mCommandOutput += "\nERROR : This command is available on the server only\n";
        }
    }
    else if (command.compare("setcreaturedest") == 0)
    {
        std::string creatureName;
        int x, y;
        tempSS.str(arguments);
        tempSS >> creatureName >> x >> y;
        if(ODServer::getSingleton().isConnected())
        {
            ODConsoleCommand* cc = new ODConsoleCommandSetCreatureDestination(creatureName, x, y);
            ODServer::getSingleton().queueConsoleCommand(cc);
        }
        else
        {
            frameListener->mCommandOutput += "\nERROR : This command is available on the server only\n";
        }
    }
    else if (command.compare("logfloodfill") == 0)
    {
        if(ODServer::getSingleton().isConnected())
        {
            ODConsoleCommand* cc = new ODConsoleCommandLogFloodFill();
            ODServer::getSingleton().queueConsoleCommand(cc);
        }
        else
        {
            frameListener->mCommandOutput += "\nERROR : This command is available on the server only\n";
        }
    }
    else if (command.compare("listmeshanims") == 0)
    {
        std::string meshName;
        tempSS.str(arguments);
        tempSS >> meshName;
        std::string anims = RenderManager::consoleListAnimationsForMesh(meshName);
        frameListener->mCommandOutput += "\nAnimations for " + meshName + anims;
    }
    else if (command.compare("triggercompositor") == 0)
    {
        tempSS.str(arguments);
        RenderManager::getSingletonPtr()->triggerCompositor(tempSS.str());
    }
    else if (command.compare("disconnect") == 0)
    {
        frameListener->mCommandOutput += (ODServer::getSingleton().isConnected())
            ? "\nStopping server.\n"
            : (ODClient::getSingleton().isConnected())
                ? "\nDisconnecting from server.\n"
                : "\nYou are not connected to a server and you are not hosting a server.";
        //TODO: Fix this
    }
    else if (command.compare("pause") == 0)
    {
        if(!arguments.empty())
        {
            int tmp;

            tempSS.str(arguments);
            tempSS >> tmp;

            gameMap->setGamePaused(tmp != 0);
        }
        else
        {
            tempSS.str("");
            tempSS  << "Pause = " << (gameMap->getGamePaused() ? 1 : 0);
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else
    {
        //try AngelScript interpreter
        return false;
        //frameListener->mCommandOutput
        //        += "\nCommand not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.\n";
    }

    Console::getSingleton().print(frameListener->mCommandOutput);

    return true;
}
