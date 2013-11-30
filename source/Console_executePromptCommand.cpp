/*!
 * \file   Console_executePromptCommand.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 */

/* TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */





#include "Console.h"
#include "Socket.h"
#include "RenderManager.h"
#include "MapLoader.h"
#include "CameraManager.h"
#include "ODApplication.h"
#include "ServerNotification.h"
#include "Player.h"
#include "AllGoals.h"
#include "Functions.h"
#include "ResourceManager.h"
#include "Network.h"
#include "CullingManager.h"
#include <OgreSceneNode.h>
#include <OgreEntity.h>

using std::min;
using std::max;
using Ogre::Radian;
bool Console::executePromptCommand(const std::string& command, std::string arguments)
{
    std::stringstream tempSS;

    /*
    // Exit the program
    if (command.compare("quit") == 0 || command.compare("exit") == 0)
    {
        //NOTE: converted to AS
        ODFrameListener::getSingletonPtr()->requestExit();
    }

    // Repeat the arguments of the command back to you
    else if (command.compare("echo") == 0)
    {
        //NOTE: dropped in AS (was this any useful?)
        ODFrameListener::getSingletonPtr()->commandOutput += "\n" + arguments + "\n";
    } */

    /*
    // Write the current level out to file specified as an argument
    if (command.compare("save") == 0)
    {
        //NOTE: convetred to AS
        if (arguments.empty())
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "No level name given: saving over the last loaded level: "
                            + ODFrameListener::getSingletonPtr()->gameMap->getLevelFileName() + "\n\n";
            arguments = ODFrameListener::getSingletonPtr()->gameMap->getLevelFileName();
        }

        string tempFileName = "levels/" + arguments + ".level";
        MapLoader::writeGameMapToFile(tempFileName, *ODFrameListener::getSingletonPtr()->gameMap);
        ODFrameListener::getSingletonPtr()->commandOutput += "\nFile saved to   " + tempFileName + "\n";

        ODFrameListener::getSingletonPtr()->gameMap->setLevelFileName(arguments);
    }*/

    // Clear the current level and load a new one from a file
    if (command.compare("load") == 0)
    {
        if (arguments.empty())
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "No level name given: loading the last loaded level: "
                            + ODFrameListener::getSingletonPtr()->gameMap->getLevelFileName() + "\n\n";
            arguments = ODFrameListener::getSingletonPtr()->gameMap->getLevelFileName();
        }

        if (Socket::clientSocket == NULL)
        {
            /* If the starting point of the string found is equal to the size
             * of the level name minus the extension (.level)
             */
            string tempString = "levels/" + arguments;
            if(arguments.find(".level") != (arguments.size() - 6))
            {
                tempString += ".level";
            }

            if (Socket::serverSocket != NULL)
            {
                ODFrameListener::getSingletonPtr()->gameMap->nextLevel = tempString;
                ODFrameListener::getSingletonPtr()->gameMap->loadNextLevel = true;
            }
            else
            {
                if (MapLoader::readGameMapFromFile(tempString, *ODFrameListener::getSingletonPtr()->gameMap))
                {
                    tempSS << "Successfully loaded file:  " << tempString
                            << "\nNum tiles:  " << ODFrameListener::getSingletonPtr()->gameMap->numTiles()
                            << "\nNum classes:  "
                            << ODFrameListener::getSingletonPtr()->gameMap->numClassDescriptions()
                            << "\nNum creatures:  " << ODFrameListener::getSingletonPtr()->gameMap->numCreatures();
                    ODFrameListener::getSingletonPtr()->commandOutput += tempSS.str();

                    ODFrameListener::getSingletonPtr()->gameMap->createAllEntities();
                }
                else
                {
                    tempSS << "ERROR: Could not load game map \'" << tempString
                            << "\'.";
                    ODFrameListener::getSingletonPtr()->commandOutput += tempSS.str();
                }
            }

            ODFrameListener::getSingletonPtr()->gameMap->setLevelFileName(arguments);
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "ERROR:  Cannot load a level if you are a client, only the sever can load new levels.";
        }
    }

    // Set the ambient light color
    else if (command.compare("ambientlight") == 0)
    {
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        if (!arguments.empty())
        {
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> tempR >> tempG >> tempB;
            mSceneMgr->setAmbientLight(Ogre::ColourValue(tempR, tempG, tempB));
            ODFrameListener::getSingletonPtr()->commandOutput += "\nAmbient light set to:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempR) + "    Green:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempG) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempB) + "\n";

        }
        else
        {
            Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent ambient light is:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.r)
                    + "    Green:  " + Ogre::StringConverter::toString(
                    (Ogre::Real) curLight.g) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.b) + "\n";
        }
    }

    // Print the help message
    else if (command.compare("help") == 0)
    {
        ODFrameListener::getSingletonPtr()->commandOutput += (!arguments.empty())
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
            ODFrameListener::getSingletonPtr()->commandOutput += "         " + Ogre::StringConverter::toString(i + 1);
        }

        ODFrameListener::getSingletonPtr()->commandOutput += "\n";

        // Print the "ones" place
        const std::string tempString = "1234567890";
        for (int i = 0; i < terminalWordWrap - 1; ++i)
        {
            ODFrameListener::getSingletonPtr()->commandOutput += tempString.substr(i % 10, 1);
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
                if (ODFrameListener::getSingletonPtr()->gameMap->getTile(i, j) == NULL)
                {

		stringstream ss;

		ss.str(std::string());
		ss << "Level";
		ss << "_";
		ss << i;
		ss << "_";
		ss << j;



		
		Tile *t = new Tile(i, j, Tile::dirt, 100);



		t->setName(ss.str());
		ODFrameListener::getSingletonPtr()->gameMap->addTile(t);
		t->createMesh();
                }
            }
        }

        ODFrameListener::getSingletonPtr()->commandOutput += "\nCreating tiles for region:\n\n\t("
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
    //         ODFrameListener::getSingletonPtr()->commandOutput += "\nmovespeed set to " + Ogre::StringConverter::toString(
    //                 tempDouble) + "\n";
    //     }
    //     else
    //     {
    //         ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent movespeed is "
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
			Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            cm->setRotateSpeed(Ogre::Degree(tempDouble));
            ODFrameListener::getSingletonPtr()->commandOutput += "\nrotatespeed set to "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    cm->getRotateSpeed().valueDegrees()))
                    + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent rotatespeed is "
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
            ODFrameListener::getSingletonPtr()->commandOutput += "\nMaximum framerate set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent maximum framerate is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
    }*/

    
    // Set the max number of threads the ODFrameListener::getSingletonPtr()->gameMap should spawn when it does the creature AI.
    else if (command.compare("aithreads") == 0)
    {
        //NOTE: converted to AS, but ODFrameListener::getSingletonPtr()->gameMap needs to be prepared
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            unsigned int tempInt;
            tempSS >> tempInt;
            if (tempInt >= 1)
            {
                ODFrameListener::getSingletonPtr()->gameMap->setMaxAIThreads(tempInt);
                ODFrameListener::getSingletonPtr()->commandOutput
                        += "\nMaximum number of creature AI threads set to "
                                + Ogre::StringConverter::toString(
                                        ODFrameListener::getSingletonPtr()->gameMap->getMaxAIThreads()) + "\n";
            }
            else
            {
                ODFrameListener::getSingletonPtr()->commandOutput
                        += "\nERROR: Maximum number of threads must be >= 1.\n";
            }
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nCurrent maximum number of creature AI threads is "
		+ Ogre::StringConverter::toString(ODFrameListener::getSingletonPtr()->gameMap->getMaxAIThreads())
                            + "\n";
        }
    } 

    // Set the turnsPerSecond variable to control the AI speed
    else if(command.compare("turnspersecond") == 0
            || command.compare("tps") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> ODApplication::turnsPerSecond;

            // Clear the queue of early/late time counts to reset the moving window average in the AI time display.
            ODFrameListener::getSingletonPtr()->gameMap->previousLeftoverTimes.clear();

            if (Socket::serverSocket != NULL)
            {
                try
                {
                    // Inform any connected clients about the change
                    ServerNotification *serverNotification =
                            new ServerNotification;
                    serverNotification->type
                            = ServerNotification::setTurnsPerSecond;
                    serverNotification->doub = ODApplication::turnsPerSecond;

                    queueServerNotification(serverNotification);
                }
                catch (bad_alloc&)
                {
                    Ogre::LogManager::getSingleton().logMessage("\n\nERROR:  bad alloc in terminal command \'turnspersecond\'\n\n", Ogre::LML_CRITICAL);
                    ODFrameListener::getSingletonPtr()->requestExit();
                }
            }

            ODFrameListener::getSingletonPtr()->commandOutput += "\nMaximum turns per second set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent maximum turns per second is "
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
            cm->getCamera()->setNearClipDistance(tempDouble);
            ODFrameListener::getSingletonPtr()->commandOutput += "\nNear clip distance set to "
                    + Ogre::StringConverter::toString(
                            cm->getCamera()->getNearClipDistance())
                    + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent near clip distance is "
                    + Ogre::StringConverter::toString(
                            cm->getCamera()->getNearClipDistance())
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
            cm->getCamera()->setFarClipDistance(tempDouble);
            ODFrameListener::getSingletonPtr()->commandOutput += "\nFar clip distance set to "
                    + Ogre::StringConverter::toString(
                            cm->getCamera()->getFarClipDistance()) + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent far clip distance is "
                    + Ogre::StringConverter::toString(
                            cm->getCamera()->getFarClipDistance()) + "\n";
        }
    }

    /*
    //Set/get the mouse movement scaling (sensitivity)
    else if (command.compare("mousespeed") == 0)
    {
        //NOTE: dropped in AS
        //Doesn't do anything at the moment, after the mouse input to cegui change.
        //TODO - remove or make usable.
        ODFrameListener::getSingletonPtr()->commandOutput += "The command is disabled\n";
        //		if(!arguments.empty())
        //		{
        //			float speed;
        //			tempSS.str(arguments);
        //			tempSS >> speed;
        //			CEGUI::System::getSingleton().setMouseMoveScaling(speed);
        //			tempSS.str("");
        //			tempSS << "Mouse speed changed to: " << speed;
        //			ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";
        //		}
        //		else
        //		{
        //			ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent mouse speed is: "
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
            // Creature the creature and add it to the ODFrameListener::getSingletonPtr()->gameMap
            Creature *tempCreature = new Creature(ODFrameListener::getSingletonPtr()->gameMap );
            std::stringstream tempSS(arguments);
            CreatureDefinition *tempClass = ODFrameListener::getSingletonPtr()->gameMap->getClassDescription(
                    tempCreature->getDefinition()->getClassName());
            if (tempClass != NULL)
            {
                *tempCreature = tempClass;
                tempSS >> tempCreature;

                ODFrameListener::getSingletonPtr()->gameMap->addCreature(tempCreature);

                // Create the mesh and SceneNode for the new creature
                Ogre::Entity *ent = RenderManager::getSingletonPtr()->getSceneManager()->createEntity("Creature_"
                        + tempCreature->getName(), tempCreature->getDefinition()->getMeshName());
                Ogre::SceneNode *node = ODFrameListener::getSingletonPtr()->creatureSceneNode->createChildSceneNode(
                        tempCreature->getName() + "_node");
                //node->setPosition(tempCreature->getPosition()/BLENDER_UNITS_PER_OGRE_UNIT);
                node->setPosition(tempCreature->getPosition());
                node->setScale(tempCreature->getDefinition()->getScale());
                node->attachObject(ent);
                ODFrameListener::getSingletonPtr()->commandOutput += "\nCreature added successfully\n";
            }
            else
            {
                ODFrameListener::getSingletonPtr()->commandOutput
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

            ODFrameListener::getSingletonPtr()->gameMap->addClassDescription(tempClass);
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
                for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->gameMap->numCreatures(); ++i)
                {
                    tempSS << ODFrameListener::getSingletonPtr()->gameMap->getCreature(i) << endl;
                }
            }

            else if (arguments.compare("classes") == 0)
            {
                tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
                for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->gameMap->numClassDescriptions(); ++i)
                {
                    CreatureDefinition *currentClassDesc =
                            ODFrameListener::getSingletonPtr()->gameMap->getClassDescription(i);
                    tempSS << currentClassDesc << "\n";
                }
            }

            else if (arguments.compare("players") == 0)
            {
                // There are only players if we are in a game.
                if (ODFrameListener::getSingletonPtr()->isInGame())
                {
                    tempSS << "Player:\tNick:\tColor:\n\n";
                    tempSS << "me\t\t" << ODFrameListener::getSingletonPtr()->gameMap->getLocalPlayer()->getNick() << "\t"
                            << ODFrameListener::getSingletonPtr()->gameMap->getLocalPlayer()->getSeat()->getColor() << "\n\n";
                    for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->gameMap->numPlayers(); ++i)
                    {
                        const Player *currentPlayer = ODFrameListener::getSingletonPtr()->gameMap->getPlayer(i);
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
                if (Socket::clientSocket != NULL)
                {
                    tempSS << "You are currently connected to a server.";
                }

                if (Socket::serverSocket != NULL)
                {
                    tempSS << "You are currently acting as a server.";
                }

                if (!ODFrameListener::getSingletonPtr()->isInGame())
                {
                    tempSS << "You are currently in the map editor.";
                }
            }

            else if (arguments.compare("rooms") == 0)
            {
                tempSS << "Name:\tColor:\tNum tiles:\n\n";
                for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->gameMap->numRooms(); ++i)
                {
                    Room *currentRoom;
                    currentRoom = ODFrameListener::getSingletonPtr()->gameMap->getRoom(i);
                    tempSS << currentRoom->getName() << "\t" << currentRoom->getColor()
                            << "\t" << currentRoom->numCoveredTiles() << "\n";
                }
            }

            else if (arguments.compare("colors") == 0 || arguments.compare(
                    "colours") == 0)
            {
                tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
                for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->playerColourValues.size(); ++i)
                {
                    tempSS << "\n" << i << "\t\t" << ODFrameListener::getSingletonPtr()->playerColourValues[i].r
                            << "\t\t" << ODFrameListener::getSingletonPtr()->playerColourValues[i].g << "\t\t"
                            << ODFrameListener::getSingletonPtr()->playerColourValues[i].b;
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
                if (ODFrameListener::getSingletonPtr()->isInGame())
                {
                    // Loop over the list of unmet goals for the seat we are sitting in an print them.
                    tempSS
                            << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->gameMap->me->getSeat()->numGoals(); ++i)
                    {
                        Goal *tempGoal = ODFrameListener::getSingletonPtr()->gameMap->me->getSeat()->getGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getDescription() << "\n";
                    }

                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS
                            << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i
                            < ODFrameListener::getSingletonPtr()->gameMap->me->getSeat()->numCompletedGoals(); ++i)
                    {
                        Goal *tempGoal = ODFrameListener::getSingletonPtr()->gameMap->me->getSeat()->getCompletedGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getSuccessMessage() << "\n";
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

            ODFrameListener::getSingletonPtr()->commandOutput += "+\n" + tempSS.str() + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
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
            ODFrameListener::getSingletonPtr()->gameMap->createNewMap(tempX, tempY);
        }
    }*/

    /*
    // refreshmesh   Clear all the Ogre entities and redraw them so they reload their appearence.
    else if (command.compare("refreshmesh") == 0)
    {
        //NOTE: Converted to AS
        ODFrameListener::getSingletonPtr()->gameMap->destroyAllEntities();
        ODFrameListener::getSingletonPtr()->gameMap->createAllEntities();
        ODFrameListener::getSingletonPtr()->commandOutput += "\nRecreating all meshes.\n";
    }*/

    // Set your nickname
    else if (command.compare("nick") == 0)
    {
        if (!arguments.empty())
        {
            ODFrameListener::getSingletonPtr()->gameMap->me->setNick(arguments);
            ODFrameListener::getSingletonPtr()->commandOutput += "\nNickname set to:  ";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nCurrent nickname is:  ";
        }

        ODFrameListener::getSingletonPtr()->commandOutput += ODFrameListener::getSingletonPtr()->gameMap->me->getNick() + "\n";
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

        ODFrameListener::getSingletonPtr()->commandOutput += "\n " + tempSS.str() + "\n";
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

        ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";
    } */

    // Connect to a server
    else if (command.compare("connect") == 0)
    {
        // Make sure we have set a nickname.
        if (!ODFrameListener::getSingletonPtr()->gameMap->me->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!ODFrameListener::getSingletonPtr()->isInGame())
            {
                // Make sure an IP address to connect to was provided
                if (!arguments.empty())
                {
                    Socket::clientSocket = new Socket;

                    if (!Socket::clientSocket->create())
                    {
                        Socket::clientSocket = NULL;
                        ODFrameListener::getSingletonPtr()->commandOutput
                                += "\nERROR:  Could not create client socket!\n";
                        goto ConnectEndLabel;
                    }

                    if (Socket::clientSocket->connect(arguments, ODApplication::PORT_NUMBER))
                    {
                        ODFrameListener::getSingletonPtr()->commandOutput += "\nConnection successful.\n";

                        CSPStruct *csps = new CSPStruct;
                        csps->nSocket = Socket::clientSocket;
                        csps->nFrameListener = ODFrameListener::getSingletonPtr();

                        // Start a thread to talk to the server
                        pthread_create(&(ODFrameListener::getSingletonPtr()->clientThread), NULL,
                                clientSocketProcessor, (void*) csps);

                        // Start the thread which will watch for local events to send to the server
                        CNPStruct *cnps = new CNPStruct;
                        cnps->nFrameListener = ODFrameListener::getSingletonPtr();
                        pthread_create(&(ODFrameListener::getSingletonPtr()->clientNotificationThread), NULL,
                                clientNotificationProcessor, cnps);

                        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
                        ODFrameListener::getSingletonPtr()->gameMap->clearMapLightIndicators();
                    }
                    else
                    {
                        Socket::clientSocket = NULL;
                        ODFrameListener::getSingletonPtr()->commandOutput += "\nConnection failed!\n";
                    }
                }
                else
                {
                    ODFrameListener::getSingletonPtr()->commandOutput
                            += "\nYou must specify the IP address of the server you want to connect to.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1\n";
                }

            }
            else
            {
                ODFrameListener::getSingletonPtr()->commandOutput
                        += "\nYou are already connected to a server.  You must disconnect before you can connect to a new game.\n";
            }
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can join a server.\n";
        }

        ConnectEndLabel: ODFrameListener::getSingletonPtr()->commandOutput += "\n";

    }

    // Host a server
    else if (command.compare("host") == 0)
    {
        // Make sure we have set a nickname.
        if (!ODFrameListener::getSingletonPtr()->gameMap->getLocalPlayer()->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!ODFrameListener::getSingletonPtr()->isInGame())
            {

                if (startServer(*ODFrameListener::getSingletonPtr()->gameMap))
                {
                    ODFrameListener::getSingletonPtr()->commandOutput += "\nServer started successfully.\n";

                    // Automatically closes the terminal
                    ODFrameListener::getSingletonPtr()->terminalActive = false;
                }
                else
                {
                    ODFrameListener::getSingletonPtr()->commandOutput += "\nERROR:  Could not start server!\n";
                }

            }
            else
            {
                ODFrameListener::getSingletonPtr()->commandOutput
                        += "\nERROR:  You are already connected to a game or are already hosting a game!\n";
            }
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can host a server.\n";
        }

    }

    // Send help command information to all players
    else if (command.compare("chathelp") == 0)
    {
        if (ODFrameListener::getSingletonPtr()->isInGame())
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

        ODFrameListener::getSingletonPtr()->commandOutput += "\n " + tempSS.str() + "\n";
    }

    // Send a chat message
    else if (command.compare("chat") == 0 || command.compare("c") == 0)
    {
        if (Socket::clientSocket != NULL)
        {
            sem_wait(&Socket::clientSocket->semaphore);
            Socket::clientSocket->send(formatCommand("chat", ODFrameListener::getSingletonPtr()->gameMap->me->getNick() + ":"
                    + arguments));
            sem_post(&Socket::clientSocket->semaphore);
        }
        else if (Socket::serverSocket != NULL)
        {
            // Send the chat to all the connected clients
            for (unsigned int i = 0; i < ODFrameListener::getSingletonPtr()->clientSockets.size(); ++i)
            {
                sem_wait(&ODFrameListener::getSingletonPtr()->clientSockets[i]->semaphore);
                ODFrameListener::getSingletonPtr()->clientSockets[i]->send(formatCommand("chat", ODFrameListener::getSingletonPtr()->gameMap->me->getNick()
                        + ":" + arguments));
                sem_post(&ODFrameListener::getSingletonPtr()->clientSockets[i]->semaphore);
            }

            // Display the chat message in our own message queue
            chatMessages.push_back(new ChatMessage(ODFrameListener::getSingletonPtr()->gameMap->me->getNick(), arguments,
                    time(NULL), time(NULL)));
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nYou must be either connected to a server, or hosting a server to use chat.\n";
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("visdebug") == 0)
    {
        if (Socket::serverSocket != NULL)
        {
            if (arguments.length() > 0)
            {
                // Activate visual debugging
                Creature *tempCreature = ODFrameListener::getSingletonPtr()->gameMap->getCreature(arguments);
                if (tempCreature != NULL)
                {
                    if (!tempCreature->getHasVisualDebuggingEntities())
                    {
                        tempCreature->createVisualDebugEntities();
                        ODFrameListener::getSingletonPtr()->commandOutput
                                += "\nVisual debugging entities created for creature:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        tempCreature->destroyVisualDebugEntities();
                        ODFrameListener::getSingletonPtr()->commandOutput
                                += "\nVisual debugging entities destroyed for creature:  "
                                        + arguments + "\n";
                    }
                }
                else
                {
                    ODFrameListener::getSingletonPtr()->commandOutput
                            += "\nCould not create visual debugging entities for creature:  "
                                    + arguments + "\n";
                }
            }
            else
            {
                ODFrameListener::getSingletonPtr()->commandOutput
                        += "\nERROR:  You must supply a valid creature name to create debug entities for.\n";
            }
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        }
    }

    else if (command.compare("addcolor") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> tempR >> tempG >> tempB;
            ODFrameListener::getSingletonPtr()->playerColourValues.push_back(Ogre::ColourValue(tempR, tempG, tempB));
            tempSS.str("");
            tempSS << "Color number " << ODFrameListener::getSingletonPtr()->playerColourValues.size() << " added.";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput
                    += "\nERROR:  You need to specify and RGB triplet with values in (0.0, 1.0)\n";
        }
    }

    else if (command.compare("setcolor") == 0)
    {
        if (!arguments.empty())
        {
            unsigned int index;
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> index >> tempR >> tempG >> tempB;
            if (index < ODFrameListener::getSingletonPtr()->playerColourValues.size())
            {
                ODFrameListener::getSingletonPtr()->playerColourValues[index] = Ogre::ColourValue(tempR, tempG, tempB);
                tempSS.str("");
                tempSS << "Color number " << index << " changed to " << tempR
                        << "\t" << tempG << "\t" << tempB;
                ODFrameListener::getSingletonPtr()->commandOutput += "an" + tempSS.str() + "\n";
            }

        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify a color index between 0 and "
                    << ODFrameListener::getSingletonPtr()->playerColourValues.size()
                    << " and an RGB triplet with values in (0.0, 1.0)";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";
        }
    }


    else if (command.compare("bspline") == 0){
	if(!arguments.empty()){
	    
	    
	    int NN;
	    double localX;
	    double localY;


	    
            tempSS.str(arguments);
	    tempSS >> NN;
	    for ( int ii = 0 ; ii < NN ; ii++){
		tempSS>>NN;


		}	    

	    // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies
            // cm->setCircleCenter(centerX, centerY);
            // cm->setCircleRadious(radious);
            // cm->setCircleMode(true);
	    	    


        }
	else{

            tempSS.str("");
            tempSS  << "ERROR:  You need to specify:  N - number of control points  and n pairs of  control points for bsplines ";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";

	    }
      
    }


    else if (command.compare("catmullspline") == 0){
	if(!arguments.empty()){
	    
	    
	    int nn;

	    int tempInt1;
	    int tempInt2;
	    
            tempSS.str(arguments);
	    tempSS >> nn;
	    cm->xHCS.resetNodes(nn);
	    cm->yHCS.resetNodes(nn);
	    for (int ii = 0 ; ii < nn ; ii++){
		
		tempSS>>tempInt1;
		tempSS>>tempInt2;

		cerr << "tempInt1 " <<  tempInt1 << endl;
		cerr << "tempInt2 " <<  tempInt2 << endl;
		cm->xHCS.addNode(tempInt1);
		cm->yHCS.addNode(tempInt2);
		

		}
	    
	    // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies

            cm->setCatmullSplineMode(true);
	    	    
	    cerr << "catmullspline loaded from cmd line " << endl;

        }
	else{

            tempSS.str("");
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radious";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";

	    }
      
    }


    else if (command.compare("setcamerafovx") == 0){
	if(!arguments.empty()){
	    

	    double tmp;
            tempSS.str(arguments);
	    tempSS >> tmp;
	    Radian radianAngle(tmp);


	    // cm->mCamera->setFOVx(radianAngle);
	    // // TODO check the for the maximal and minimal value of setFoVy
        }
	else{

            tempSS.str("");
            tempSS  << "ERROR:  No such commend in Ogre, try setcamerafovy and camera aspect ratio ";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";

	    }
	}

    else if (command.compare("setcamerafovy") == 0){
	if(!arguments.empty()){
	    

	    double tmp;
            tempSS.str(arguments);
	    tempSS >> tmp;
	    Radian radianAngle(tmp);


	    cm->mCamera->setFOVy(radianAngle);
	    // TODO check the for the maximal and minimal value of setFoVy
        }
	else{

            tempSS.str("");
            tempSS  << "ERROR:  you need to specify an angle in radians ";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";

	    }
	}

    else if (command.compare("possescreature") == 0){

            tempSS.str("");
            tempSS  << "Click creature you want to posses ";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";
	    modeManager->mc->expectCreatureClick = true;


	}




    else if (command.compare("circlearound") == 0){
	if(!arguments.empty()){
	    
	    
	    double centerX;
	    double centerY;
	    double radious;
	    
            tempSS.str(arguments);
	    tempSS >> centerX >> centerY >> radious;
	    // if(){}else{} TODO : Check if any part of the circle can fall out of the map bounderies
            cm->setCircleCenter(centerX, centerY);
            cm->setCircleRadious(radious);
            cm->setCircleMode(true);
	    	    


        }
	else{

            tempSS.str("");
            tempSS  << "ERROR:  You need to specify an circle center ( two coordinates ) and circle radious";
            ODFrameListener::getSingletonPtr()->commandOutput += "\n" + tempSS.str() + "\n";

	    }
      
    }


    else if (command.compare("switchpolygonmode") == 0){

	    cm->switchPM();
    
    }

    else if (command.compare("starttileculling") == 0){

            ODFrameListener::getSingletonPtr()->gameMap->culm->startTileCulling();
    
    }

    else if (command.compare("startcreatureculling") == 0){

            ODFrameListener::getSingletonPtr()->gameMap->culm->startCreatureCulling();
    
    }


    else if (command.compare("triggercompositor") == 0){
            tempSS.str(arguments);
	    RenderManager::getSingletonPtr()->triggerCompositor(tempSS.str());

	}    



    else if (command.compare("disconnect") == 0)
    {
        ODFrameListener::getSingletonPtr()->commandOutput += (Socket::serverSocket != NULL)
            ? "\nStopping server.\n"
            : (Socket::clientSocket != NULL)
                ? "\nDisconnecting from server.\n"
                : "\nYou are not connected to a server and you are not hosting a server.";
    }

    // Load the next level.
    else if (command.compare("next") == 0)
    {
        if (ODFrameListener::getSingletonPtr()->gameMap->seatIsAWinner(ODFrameListener::getSingletonPtr()->gameMap->me->getSeat()))
        {
            ODFrameListener::getSingletonPtr()->gameMap->loadNextLevel = true;
            ODFrameListener::getSingletonPtr()->commandOutput += (string) "\nLoading level levels/"
                    + ODFrameListener::getSingletonPtr()->gameMap->nextLevel + ".level\n";
        }
        else
        {
            ODFrameListener::getSingletonPtr()->commandOutput += "\nYou have not completed this level yet.\n";
        }
    }

    else
    {
        //try AngelScript interpreter
        return false;
        //ODFrameListener::getSingletonPtr()->commandOutput
        //        += "\nCommand not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.\n";
    }

    Console::getSingleton().print(ODFrameListener::getSingletonPtr()->commandOutput);
    
    
    return true;
}

