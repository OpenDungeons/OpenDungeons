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

/* TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */

#include "Console.h"
#include "RenderManager.h"
#include "MapLoader.h"
#include "CameraManager.h"
#include "ODApplication.h"
#include "Player.h"
#include "AllGoals.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "ODClient.h"
#include "ResourceManager.h"
#include "CullingManager.h"
#include "Weapon.h"

#include <OgreLogManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>

#include <sstream>

using std::min;
using std::max;
using Ogre::Radian;

bool Console::executePromptCommand(const std::string& command, std::string arguments)
{
    std::stringstream tempSS;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    CameraManager* cm = frameListener->cm;
    GameMap* gameMap = frameListener->mGameMap;

    /*
    // Exit the program
    if (command.compare("quit") == 0 || command.compare("exit") == 0)
    {
        //NOTE: converted to AS
        frameListener->requestExit();
    }

    // Repeat the arguments of the command back to you
    else if (command.compare("echo") == 0)
    {
        //NOTE: dropped in AS (was this any useful?)
        frameListener->mCommandOutput += "\n" + arguments + "\n";
    } */

    /*
    // Write the current level out to file specified as an argument
    if (command.compare("save") == 0)
    {
        //NOTE: convetred to AS
        if (arguments.empty())
        {
            frameListener->mCommandOutput
                    += "No level name given: saving over the last loaded level: "
                            + gameMap->getLevelFileName() + "\n\n";
            arguments = gameMap->getLevelFileName();
        }

        string tempFileName = "levels/" + arguments + ".level";
        MapLoader::writeGameMapToFile(tempFileName, *gameMap);
        frameListener->mCommandOutput += "\nFile saved to   " + tempFileName + "\n";

        gameMap->setLevelFileName(arguments);
    }*/

    // Clear the current level and load a new one from a file
    if (command.compare("load") == 0)
    {
        if (arguments.empty())
        {
            frameListener->mCommandOutput
                    += "No level name given: loading the last loaded level: "
                            + gameMap->getLevelFileName() + "\n\n";
            arguments = gameMap->getLevelFileName();
        }

        if (!ODClient::getSingleton().isConnected())
        {
            /* If the starting point of the string found is equal to the size
             * of the level name minus the extension (.level)
             */
            string tempString = "levels/" + arguments;
            if(arguments.find(".level") != (arguments.size() - 6))
            {
                tempString += ".level";
            }

            if (ODServer::getSingleton().isConnected())
            {
                frameListener->mCommandOutput
                    += "ERROR:  Cannot load a level if you are a already running a server.";
            }
            else
            {
                gameMap->LoadLevel(tempString);
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "ERROR:  Cannot load a level if you are a client, only the sever can load new levels.";
        }
    }

    else if (command.compare("saveGameMapToTgaFile") == 0)
    {
        if (!arguments.empty())
        {
	    MapLoader::writeGameMapFromTgaFile(arguments, *gameMap);
	}

    }
    else if (command.compare("loadGameMapFromTgaFile") == 0)
    {
        if (!arguments.empty())
        {
	    MapLoader::readGameMapFromTgaFile(arguments, *gameMap);
	}
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

    /*
    // A utility to set the wordrap on the terminal to a specific value
    else if (command.compare("termwidth") == 0)
    {
        //NOTE: dropped in AS (this done by the console)
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> terminalWordWrap;
        }

        // Print the "tens" place line at the top
        for (int i = 0; i < terminalWordWrap / 10; ++i)
        {
            frameListener->mCommandOutput += "         " + Ogre::StringConverter::toString(i + 1);
        }

        frameListener->mCommandOutput += "\n";

        // Print the "ones" place
        const std::string tempString = "1234567890";
        for (int i = 0; i < terminalWordWrap - 1; ++i)
        {
            frameListener->mCommandOutput += tempString.substr(i % 10, 1);
        }

    } */

    // A utility which adds a new section of the map given as the
    // rectangular region between two pairs of coordinates
    else if (command.compare("addtiles") == 0)
    {
        int x1, y1, x2, y2;
        tempSS.str(arguments);
        tempSS >> x1 >> y1 >> x2 >> y2;
        int xMin, yMin, xMax, yMax;
        xMin = min(x1, x2);
        xMax = max(x1, x2);
        yMin = min(y1, y2);
        yMax = max(y1, y2);

        for (int j = yMin; j < yMax; ++j)
        {
            for (int i = xMin; i < xMax; ++i)
            {
                if (gameMap->getTile(i, j) == NULL)
                {
                    std::stringstream ss;

                    ss.str(std::string());
                    ss << "Level";
                    ss << "_";
                    ss << i;
                    ss << "_";
                    ss << j;

                    Tile* t = new Tile(gameMap, i, j, Tile::dirt, 100);
                    t->setName(ss.str());
                    t->createMesh();
                    gameMap->addTile(t);
                }
            }
        }

        frameListener->mCommandOutput += "\nCreating tiles for region:\n\n\t("
                + Ogre::StringConverter::toString(xMin) + ", "
                + Ogre::StringConverter::toString(yMin) + ")\tto\t("
                + Ogre::StringConverter::toString(xMax) + ", "
                + Ogre::StringConverter::toString(yMax) + ")\n";
    }

    // A utility to set the camera movement speed
    // else if (command.compare("movespeed") == 0)
    // {
    //     //NOTE: converted to AS
    //     if (!arguments.empty())
    //     {
    //         Ogre::Real tempDouble;
    //         tempSS.str(arguments);
    //         tempSS >> tempDouble;
    //         cm->setMoveSpeedAccel(2.0 * tempDouble);
    //         frameListener->mCommandOutput += "\nmovespeed set to " + Ogre::StringConverter::toString(
    //                 tempDouble) + "\n";
    //     }
    //     else
    //     {
    //         frameListener->mCommandOutput += "\nCurrent movespeed is "
    //                 + Ogre::StringConverter::toString(
    //                         cm->getMoveSpeed())
    //                 + "\n";
    //     }
    // }

    // A utility to set the camera rotation speed.
    else if (command.compare("rotatespeed") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempDouble = 0.0;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            cm->setRotateSpeed(Ogre::Degree(tempDouble));
            frameListener->mCommandOutput += "\nrotatespeed set to "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    cm->getRotateSpeed().valueDegrees()))
                    + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent rotatespeed is "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    cm->getRotateSpeed().valueDegrees()))
                    + "\n";
        }
    }

    /*
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
    }*/

    // Set the turnsPerSecond variable to control the AI speed
    else if(command.compare("turnspersecond") == 0
            || command.compare("tps") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> ODApplication::turnsPerSecond;

            if (ODServer::getSingleton().isConnected())
            {
                try
                {
                    // Inform any connected clients about the change
                    ServerNotification *serverNotification = new ServerNotification(
                        ServerNotification::setTurnsPerSecond, NULL);
                    serverNotification->packet << ODApplication::turnsPerSecond;
                    ODServer::getSingleton().queueServerNotification(serverNotification);
                }
                catch (std::bad_alloc&)
                {
                    Ogre::LogManager::getSingleton().logMessage("ERROR: bad alloc in terminal command \'turnspersecond\'\n\n", Ogre::LML_CRITICAL);
                    exit(1);
                }
            }

            frameListener->mCommandOutput += "\nMaximum turns per second set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent maximum turns per second is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
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
            cm->getActiveCamera()->setNearClipDistance(tempDouble);
            frameListener->mCommandOutput += "\nNear clip distance set to "
                    + Ogre::StringConverter::toString(
                            cm->getActiveCamera()->getNearClipDistance())
                    + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent near clip distance is "
                    + Ogre::StringConverter::toString(
                            cm->getActiveCamera()->getNearClipDistance())
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
            cm->getActiveCamera()->setFarClipDistance(tempDouble);
            frameListener->mCommandOutput += "\nFar clip distance set to "
                    + Ogre::StringConverter::toString(
                            cm->getActiveCamera()->getFarClipDistance()) + "\n";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent far clip distance is "
                    + Ogre::StringConverter::toString(
                            cm->getActiveCamera()->getFarClipDistance()) + "\n";
        }
    }

    /*
    //Set/get the mouse movement scaling (sensitivity)
    else if (command.compare("mousespeed") == 0)
    {
        //NOTE: dropped in AS
        //Doesn't do anything at the moment, after the mouse input to cegui change.
        //TODO - remove or make usable.
        frameListener->mCommandOutput += "The command is disabled\n";
        //		if(!arguments.empty())
        //		{
        //			float speed;
        //			tempSS.str(arguments);
        //			tempSS >> speed;
        //			CEGUI::System::getSingleton().setMouseMoveScaling(speed);
        //			tempSS.str("");
        //			tempSS << "Mouse speed changed to: " << speed;
        //			frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        //		}
        //		else
        //		{
        //			frameListener->mCommandOutput += "\nCurrent mouse speed is: "
        //				+ StringConverter::toString(static_cast<Real>(
        //				CEGUI::System::getSingleton().getMouseMoveScaling())) + "\n";
        //		}
    } */

    // Add a new instance of a creature to the current map.  The argument is
    // read as if it were a line in a .level file.
    else if (command.compare("addcreature") == 0)
    {
        if (!arguments.empty())
        {
            // Creature the creature and add it to the gameMap
            // TODO : ask the server to do that
            Creature *tempCreature = new Creature(gameMap, true);
            std::stringstream tempSS(arguments);
            CreatureDefinition *tempClass = gameMap->getClassDescription(tempCreature->getDefinition()->getClassName());
            if (tempClass != NULL)
            {
                tempCreature->setCreatureDefinition(tempClass);
                tempSS >> tempCreature;

                tempCreature->createMesh();
                tempCreature->getWeaponL()->createMesh();
                tempCreature->getWeaponR()->createMesh();
                gameMap->addCreature(tempCreature);
                frameListener->mCommandOutput += "\nCreature added successfully\n";
            }
            else
            {
                frameListener->mCommandOutput
                        += "\nInvalid creature class name, you need to first add a class with the \'addclass\' terminal command.\n";
            }
        }
    }

    // Adds the basic information about a type of creature (mesh name, scaling, etc)
    else if (command.compare("addclass") == 0)
    {
        if (!arguments.empty())
        {
            CreatureDefinition *tempClass = new CreatureDefinition;
            tempSS.str(arguments);
            tempSS >> tempClass;

            gameMap->addClassDescription(tempClass);
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
                    tempSS << gameMap->getCreature(i) << endl;
                }
            }

            else if (arguments.compare("classes") == 0)
            {
                tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
                for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
                {
                    CreatureDefinition *currentClassDesc = gameMap->getClassDescription(i);
                    tempSS << currentClassDesc << "\n";
                }
            }

            else if (arguments.compare("players") == 0)
            {
                // There are only players if we are connected.
                if (frameListener->isConnected())
                {
                    tempSS << "Player:\tNick:\tColor:\n\n";
                    tempSS << "me\t\t" << gameMap->getLocalPlayer()->getNick() << "\t"
                            << gameMap->getLocalPlayer()->getSeat()->getColor() << "\n\n";
                    for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
                    {
                        const Player *currentPlayer = gameMap->getPlayer(i);
                        tempSS << i << "\t\t" << currentPlayer->getNick() << "\t"
                                << currentPlayer->getSeat()->getColor() << "\n";
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
                tempSS << "Name:\tColor:\tNum tiles:\n\n";
                for (unsigned int i = 0; i < gameMap->numRooms(); ++i)
                {
                    Room *currentRoom;
                    currentRoom = gameMap->getRoom(i);
                    tempSS << currentRoom->getName() << "\t" << currentRoom->getColor()
                            << "\t" << currentRoom->numCoveredTiles() << "\n";
                }
            }

            else if (arguments.compare("colors") == 0 || arguments.compare("colours") == 0)
            {
                tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
                for (unsigned int i = 0; i < gameMap->numFilledSeats(); ++i)
                {
                    Ogre::ColourValue color = gameMap->getFilledSeat(i)->getColorValue();

                    tempSS << "\n" << i << "\t\t" << color.r
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
                        tempSS << tempVector[j] << endl;
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
    // clearmap   Erase all of the tiles leaving an empty map
    else if (command.compare("newmap") == 0)
    {
        //NOTE: Converted to AS
        if (!arguments.empty())
        {
            int tempX, tempY;

            tempSS.str(arguments);
            tempSS >> tempX >> tempY;
            gameMap->createNewMap(tempX, tempY);
        }
    }*/

    /*
    // refreshmesh   Clear all the Ogre entities and redraw them so they reload their appearence.
    else if (command.compare("refreshmesh") == 0)
    {
        //NOTE: Converted to AS
        gameMap->destroyAllEntities();
        gameMap->createAllEntities();
        frameListener->mCommandOutput += "\nRecreating all meshes.\n";
    }*/

    // Set your nickname
    else if (command.compare("nick") == 0)
    {
        if (!arguments.empty())
        {
            gameMap->getLocalPlayer()->setNick(arguments);
            frameListener->mCommandOutput += "\nNickname set to:  ";
        }
        else
        {
            frameListener->mCommandOutput += "\nCurrent nickname is:  ";
        }

        frameListener->mCommandOutput += gameMap->getLocalPlayer()->getNick() + "\n";
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
                    std::string level;

                    tempSS >> ip >> level;

                    // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
                    gameMap->clearMapLightIndicators();

                    if (ODClient::getSingleton().connect(ip, ODApplication::PORT_NUMBER, level))
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

    // Host a server
    else if (command.compare("host") == 0)
    {
        // Make sure we have set a nickname.
        if ((!gameMap->getLocalPlayer()->getNick().empty()) &&
            (!arguments.empty()))
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!frameListener->isConnected())
            {
                if (ODServer::getSingleton().startServer(arguments, false))
                {
                    frameListener->mCommandOutput += "\nServer started successfully.\n";

                    // Automatically closes the terminal
                    frameListener->mTerminalActive = false;
                }
                else
                {
                    frameListener->mCommandOutput += "\nERROR:  Could not start server!\n";
                }

            }
            else
            {
                frameListener->mCommandOutput
                        += "\nERROR:  You are already connected to a game or are already hosting a game!\n";
            }
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can host a server.\n";
        }
    }

    // Send help command information to all players
    else if (command.compare("chathelp") == 0)
    {
        if (frameListener->isConnected())
        {
            if (!arguments.empty())
            {
                // call getHelpText()
                string tempString;
                tempString = getHelpText(arguments);

                if (tempString.compare("Help for command:  \"" + arguments
                        + "\" not found.") == 0)
                {
                    tempSS << tempString << "\n";
                }
                else
                {
                    executePromptCommand("chat", "\n" + tempString);
                }
            }
            else
            {
                tempSS << "No command argument specified. See 'help' for a list of arguments.\n";
            }
        }
        else
        {
            tempSS << "Please host or connect to a game before running chathelp.\n";
        }

        frameListener->mCommandOutput += "\n " + tempSS.str() + "\n";
    }

    // Send a chat message
    else if (command.compare("chat") == 0 || command.compare("c") == 0)
    {
        if (ODClient::getSingleton().isConnected())
        {
            ODPacket packSend;
            packSend << "chat" << gameMap->getLocalPlayer()->getNick()
                << arguments;
            ODClient::getSingleton().sendToServer(packSend);
        }
        else if (ODServer::getSingleton().isConnected())
        {
            // Send the chat to all the connected clients
            ODPacket packSend;
            packSend << "chat" << gameMap->getLocalPlayer()->getNick()
                << arguments;
            ODServer::getSingleton().sendToAllClients(packSend);

            // Display the chat message in our own message queue
            mChatMessages.push_back(new ChatMessage(gameMap->getLocalPlayer()->getNick(), arguments,
                    time(NULL), time(NULL)));
        }
        else
        {
            frameListener->mCommandOutput
                    += "\nYou must be either connected to a server, or hosting a server to use chat.\n";
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("visdebug") == 0)
    {
        if (ODServer::getSingleton().isConnected())
        {
            if (arguments.length() > 0)
            {
                // Activate visual debugging
                Creature *tempCreature = gameMap->getCreature(arguments);
                if (tempCreature != NULL)
                {
                    if (!tempCreature->getHasVisualDebuggingEntities())
                    {
                        tempCreature->createVisualDebugEntities();
                        frameListener->mCommandOutput
                                += "\nVisual debugging entities created for creature:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        tempCreature->destroyVisualDebugEntities();
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

    else if (command.compare("bspline") == 0)
    {
        if(!arguments.empty())
        {

            int NN;
            tempSS.str(arguments);
            tempSS >> NN;
            for ( int ii = 0 ; ii < NN ; ++ii)
            {
                tempSS>>NN;
            }

            // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies
            // cm->setCircleCenter(centerX, centerY);
            // cm->setCircleRadious(radious);
            // cm->setCircleMode(true);

        }
        else {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify:  N - number of control points  and n pairs of  control points for bsplines ";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }


    else if (command.compare("catmullspline") == 0){
        if(!arguments.empty()){

            int nn = 0;

            int tempInt1 = 0;
            int tempInt2 = 0;

            tempSS.str(arguments);
            tempSS >> nn;
            cm->xHCS.resetNodes(nn);
            cm->yHCS.resetNodes(nn);

            for (int ii = 0 ; ii < nn ; ++ii)
            {
                tempSS>>tempInt1;
                tempSS>>tempInt2;

                //cerr << "tempInt1 " <<  tempInt1 << endl;
                //cerr << "tempInt2 " <<  tempInt2 << endl;
                cm->xHCS.addNode(tempInt1);
                cm->yHCS.addNode(tempInt2);
            }

            // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies

            cm->setCatmullSplineMode(true);

            //cerr << "catmullspline loaded from cmd line " << endl;
        }
        else
        {

            tempSS.str("");
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radious";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("setcamerafovx") == 0){
        if(!arguments.empty())
        {
            double tmp;
            tempSS.str(arguments);
            tempSS >> tmp;
            Radian radianAngle((Ogre::Real)tmp);

            // mCm->mCamera->setFOVx(radianAngle);
            // TODO check the for the maximal and minimal value of setFoVy
        }
        else
        {

            tempSS.str("");
            tempSS  << "ERROR:  No such commend in Ogre, try setcamerafovy and camera aspect ratio ";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("setcamerafovy") == 0)
    {
        if(!arguments.empty()){

            double tmp;
            tempSS.str(arguments);
            tempSS >> tmp;
            Radian radianAngle((Ogre::Real)tmp);

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
    else if (command.compare("possescreature") == 0)
    {
        tempSS.str("");
        tempSS  << "Click creature you want to posses ";
        frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        frameListener->getModeManager()->getInputManager()->mExpectCreatureClick = true;
    }
    else if (command.compare("circlearound") == 0)
    {
        if(!arguments.empty())
        {
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
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radious";
            frameListener->mCommandOutput += "\n" + tempSS.str() + "\n";
        }
    }
    else if (command.compare("switchpolygonmode") == 0)
    {
        cm->switchPM();
    }
    else if (command.compare("starttileculling") == 0)
    {
            gameMap->culm->startTileCulling();
    }
    else if (command.compare("stoptileculling") == 0)
    {
            gameMap->culm->stopTileCulling();
    }

    else if (command.compare("startcreatureculling") == 0)
    {
            gameMap->culm->startCreatureCulling();
    }
    else if (command.compare("startdb") == 0)
    {
            gameMap->culm->startDebugging();
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
