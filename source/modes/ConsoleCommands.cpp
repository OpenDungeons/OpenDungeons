/*!
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

#include "modes/ConsoleCommands.h"

#include "modes/Console.h"
#include "modes/PrefixTree.h"

// For commands
#include "ODConsoleCommand.h"
#include "goals/Goal.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/ODFrameListener.h"
#include "entities/Creature.h"
#include "ODApplication.h"
#include "utils/ResourceManager.h"

static const std::string GENERAL_HELP_MESSAGE = "\
The console is a way of interacting with the underlying game engine directly.\
Commands given to the the are made up of two parts: a \'command name\' and one or more \'arguments\'.\
For information on how to use a particular command, type help followed by the command name.\
\n\nThe following commands are avalaible:\
\n\n==General==\
\n\thelp - Displays this help message.\
\n\thelp keys - Shows the keyboard controls.\
\n\tlist/ls - Prints out lists of creatures, classes, etc...\
\n\tmaxmessages - Sets or displays the max number of chat messages to display.\
\n\tmaxtime - Sets or displays the max time for chat messages to be displayed.\
\n\ttermwidth - Sets the terminal width.\
\n\n==Cheats==\
\n\taddcreature - Adds a creature.\
\n\tsetcreaturelevel - Sets the level of a given creature.\
\n\taddgold - Gives gold to one player.\
\n\ticanseedeadpeople - Toggles on/off fog of war for every connected player.\
\n\n==Developer\'s options==\
\n\tfps - Sets the maximum framerate cap.\
\n\tambientlight - Sets the ambient light color.\
\n\tnearclip - Sets the near clipping distance.\
\n\tfarclip - Sets the far clipping distance.\
\n\tcreaturevisdebug - Turns on visual debugging for a given creature.\
\n\tseatvisdebug - Turns on visual debugging for a given seat.\
\n\tsetcreaturedest - Sets the creature destination/\
\n\tlistmeshanims - Lists all the animations for the given mesh.\
\n\ttriggercompositor - Starts the given Ogre Compositor.\
\n\tcatmullspline - Triggers the catmullspline camera movement type.\
\n\tcirclearound - Triggers the circle camera movement type.\
\n\tsetcamerafovy - Sets the camera vertical field of view aspect ratio value.\
\n\tlogfloodfill - Displays the FloodFillValues of all the Tiles in the GameMap.";

static const std::string COMMAND_NOT_FOUND = "Help for the given command not found.";

const std::string& ConsoleCommands::getGeneralHelpMessage() const
{
    return GENERAL_HELP_MESSAGE;
}

const std::string& ConsoleCommands::getHelpForCommand(const std::string& command)
{
    // Lower case the command text.
    std::string lowCasedCmd = command;
    std::transform(lowCasedCmd.begin(), lowCasedCmd.end(), lowCasedCmd.begin(), ::tolower);

    std::map<std::string, std::string>::const_iterator it = mCommandsHelp.find(lowCasedCmd);

    if (it != mCommandsHelp.end())
        return it->second;

    // TODO: Handle 'list' specific argument help.

    return COMMAND_NOT_FOUND;
}

bool ConsoleCommands::autocomplete(const char* word, std::vector<std::string>& autocompletedWords)
{
    return mCommandsPrefixTree.complete(word, autocompletedWords);
}

void ConsoleCommands::loadCommands()
{
    std::string description = "Add a new creature to the current map.  The creature class to be used must be loaded first, either from the loaded map file or by using the addclass command.  Once a class has been declared a creature can be loaded using that class.  The argument to the addcreature command is interpreted in the same way as a creature line in a .level file.\n\nExample:\n"
                              "addcreature Skeleton Bob 10 15 0\n\nThe above command adds a creature of class \"Skeleton\" whose name is \"Bob\" at location X=10, y=15, and Z=0.  The new creature's name must be unique to the creatures in that level.  Alternatively the name can be se to \"autoname\" to have OpenDungeons assign a unique name.";
    mCommandsHelp.insert(std::make_pair("addcreature", description));

    description = "The 'ambientlight' command sets the minumum light that every object in the scene is illuminated with.  It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n"
                  "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.";
    mCommandsHelp.insert(std::make_pair("ambientlight", description));

    description = "'fps' (framespersecond) for short is a utility which displays or sets the maximum framerate at which the rendering will attempt to update the screen.\n\nExample:\n"
                  "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.";
    mCommandsHelp.insert(std::make_pair("fps", description));

    description = "Sets the minimal viewpoint clipping distance. Objects nearer than that won't be rendered.\n\nE.g.: nearclip 3.0";
    mCommandsHelp.insert(std::make_pair("nearclip", description));

    description = "Sets the maximal viewpoint clipping distance. Objects farther than that won't be rendered.\n\nE.g.: farclip 30.0";
    mCommandsHelp.insert(std::make_pair("farclip", description));

    description = "'list' (or 'ls' for short) is a utility which lists various types of information about the current game. "
                  "Running list without an argument will produce a list of the lists available. "
                  "Running list with an argument displays the contents of that list.\n\nExample:\n"
                  "list creatures\n\nThe above command will produce a list of all the creatures currently in the game.";
    mCommandsHelp.insert(std::make_pair("list", description));
    mCommandsHelp.insert(std::make_pair("ls", description));

    description = "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n"
                  "creaturevisdebug skeletor\n\nThe above command wil turn on visual debugging for the creature named \'skeletor\'.  The same command will turn it back off again.";
    mCommandsHelp.insert(std::make_pair("creaturevisdebug", description));

    description = "Visual debugging is a way to see all the tiles a given seat can see.\n\nExample:\n"
                  "seatvisdebug 1\n\nThe above command will show every tiles seat 1 can see.  The same command will turn it off.";
    mCommandsHelp.insert(std::make_pair("seatvisdebug", description));

    description = "Toggles on/off fog of war for every connected player";
    mCommandsHelp.insert(std::make_pair("icanseedeadpeople", description));

    description = "Sets the level of a given creature.\n\nExample:\n"
                  "setlevel BigKnight1 10\n\nThe above command will set the creature \'BigKnight1\' to 10.";
    mCommandsHelp.insert(std::make_pair("setcreaturelevel", description));

    description = "setcreaturedest Sets the creature destination. The path will be computed if reachable. It takes as arguments the creature name, then the tile coordinales.\n"
                  "Example : setdest Wizard1 10 10";
    mCommandsHelp.insert(std::make_pair("setcreaturedest", description));

    description = "'addgold' adds the given amount of gold to one player. It takes as arguments the color of the player to whom the gold should be given and the amount. If the player's treasuries are full, no more gold is given. Note that this command is available in server mode only. Example to give 5000 gold to player color 1 : addgold 1 5000";
    mCommandsHelp.insert(std::make_pair("addgold", description));

    description = "'logfloodfill' logs the FloodFillValues of all the Tiles in the GameMap.";
    mCommandsHelp.insert(std::make_pair("logfloodfill", description));

    description = "'listmeshanims' lists all the animations for the given mesh.";
    mCommandsHelp.insert(std::make_pair("listmeshanims", description));

    description = "Starts the given compositor. The compositor must exist.\n\nExample:\n"
                  "triggercompositor blacknwhite";
    mCommandsHelp.insert(std::make_pair("triggercompositor", description));

    description = "Sets the max number of lines displayed at once in the info text area.\n\nExample:\n"
                  "maxmessages 4";
    mCommandsHelp.insert(std::make_pair("maxmessages", description));

    description = "Sets the max time (in seconds) a message will be displayed in the info text area.\n\nExample:\n"
                  "maxtime 5";
    mCommandsHelp.insert(std::make_pair("maxtime", description));

    description = "Triggers the catmullspline camera movement behaviour.\n\nExample:\n"
                  "catmullspline 6 4 5 4 6 5 7\n"
                  "Make the camera follow a lazy curved path along the given coordinates pairs. "
                  "The first parameter is the number of pairs";
    mCommandsHelp.insert(std::make_pair("catmullspline", description));

    description = "Triggers the circle camera movement behaviour.\n\nExample:\n"
                  "circlearound 6 4 8\n"
                  "Make the camera follow a lazy a circle path around coors 6,4 with a radius of 8.";
    mCommandsHelp.insert(std::make_pair("circlearound", description));

    description = "Sets the camera vertical field of view aspect ratio on the Y axis.\n\nExample:\n"
                  "setcamerafovy 45";
    mCommandsHelp.insert(std::make_pair("setcamerafovy", description));

    description = "|| Action               || US Keyboard layout ||     Mouse      ||\n\
                   ==================================================================\n\
                   || Pan Left             || Left   -   A       || -              ||\n\
                   || Pan Right            || Right  -   D       || -              ||\n\
                   || Pan Forward          || Up     -   W       || -              ||\n\
                   || Pan Backward         || Down   -   S       || -              ||\n\
                   || Rotate Left          || Q                  || -              ||\n\
                   || Rotate right         || E                  || -              ||\n\
                   || Zoom In              || End                || Wheel Up       ||\n\
                   || Zoom Out             || Home               || Wheel Down     ||\n\
                   || Tilt Up              || Page Up            || -              ||\n\
                   || Tilt Down            || End                || -              ||\n\
                   || Select Tile/Creature || -                  || Left Click     ||\n\
                   || Drop Creature/Gold   || -                  || Right Click    ||\n\
                   || Toggle Debug Info    || F11                || -              ||\n\
                   || Toggle Console       || F12                || -              ||\n\
                   || Quit Game            || ESC                || -              ||\n\
                   || Take screenshot      || Printscreen        || -              ||";
    mCommandsHelp.insert(std::make_pair("keys", description));

    // Loads the prefixes
    for (auto it : mCommandsHelp)
    {
        mCommandsPrefixTree.addNewString(it.first);
    }
}

bool ConsoleCommands::executePromptCommand(const std::string& command, const::std::string& arguments)
{
    std::stringstream tempSS;

    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getClientGameMap();

    // Set the ambient light color
    if (command.compare("ambientlight") == 0)
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
                ? "\nHelp for command:  " + arguments + "\n\n" + getHelpForCommand(arguments) + "\n"
                : "\n" + GENERAL_HELP_MESSAGE + "\n";
    }

    // Set max frames per second
    else if (command.compare("fps") == 0)
    {
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
                std::vector<std::string> tempVector;
                size_t found;
                size_t found2;
                std::string suffix = ".level";
                std::string suffix2 = ".level.";
                tempVector = ResourceManager::getSingletonPtr()->
                        listAllFiles("./levels/");
                for (unsigned int j = 0; j < tempVector.size(); ++j)
                {
                    found = tempVector[j].find(suffix);
                    found2 = tempVector[j].find(suffix2);
                    if (found != std::string::npos && (!(found2 != std::string::npos)))
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

