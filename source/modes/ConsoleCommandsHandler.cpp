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

#include "modes/ConsoleCommandsHandler.h"

#include "modes/Console.h"
#include "modes/PrefixTree.h"

// For commands
#include "modes/ServerConsoleCommands.h"
#include "goals/Goal.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/ODFrameListener.h"
#include "entities/Creature.h"
#include "ODApplication.h"
#include "utils/ResourceManager.h"
#include "utils/Helper.h"

static const std::string GENERAL_HELP_MESSAGE = "\
The console is a way of interacting with the underlying game engine directly.\
Commands given to the the are made up of two parts: a \'command name\' and one or more \'arguments\'.\
For information on how to use a particular command, type help followed by the command name.\
\n\nThe following commands are available:\
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

const std::string& ConsoleCommandsHandler::getGeneralHelpMessage() const
{
    return GENERAL_HELP_MESSAGE;
}

const std::string& ConsoleCommandsHandler::getHelpForCommand(const std::string& command)
{
    // Lower case the command text.
    std::string lowCasedCmd = command;
    std::transform(lowCasedCmd.begin(), lowCasedCmd.end(), lowCasedCmd.begin(), ::tolower);

    std::map<std::string, std::string>::const_iterator it = mCommandsHelp.find(lowCasedCmd);

    if (it != mCommandsHelp.end())
        return it->second;

    return COMMAND_NOT_FOUND;
}

bool ConsoleCommandsHandler::autocomplete(const char* word, std::vector<std::string>& autocompletedWords)
{
    return mCommandsPrefixTree.complete(word, autocompletedWords);
}

void ConsoleCommandsHandler::loadCommands()
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
                  "Running list with an argument displays the contents of that list.\n\nExamples:\n"
                  "list creatures\tLists all the creatures currently in the game.\n"
                  "list classes\tLists all creature classes.\n"
                  "list players\tLists every player in game.\n"
                  "list network\tTells whether the game is running as a server, a client or as the map editor.\n"
                  "list rooms\tLists all the current rooms in game.\n"
                  "list colors\tLists all seat's color values.\n"
                  "list goals\tLists The local player goals.\n";
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

bool ConsoleCommandsHandler::executePromptCommand(const std::string& command, const::std::string& arguments)
{
    // Clear out the output before processing a command.
    mCommandOutput.clear();

    // Print the help message
    if (command.compare("help") == 0)
    {
        mCommandOutput += (!arguments.empty())
                ? "\nHelp for command:  " + arguments + "\n\n" + getHelpForCommand(arguments) + "\n"
                : "\n" + GENERAL_HELP_MESSAGE + "\n";
    }
    else if (command.compare("ambientlight") == 0)
    {
        handleAmbientLight(arguments);
    }
    else if (command.compare("fps") == 0)
    {
        handleFPS(arguments);
    }
    else if (command.compare("nearclip") == 0)
    {
        handleNearClip(arguments);
    }
    else if (command.compare("farclip") == 0)
    {
        handleFarClip(arguments);
    }
    else if (command.compare("addcreature") == 0)
    {
        handleAddCreature(arguments);
    }
    else if (command.compare("list") == 0 || command.compare("ls") == 0)
    {
        handleList(arguments);
    }
    else if (command.compare("maxtime") == 0)
    {
        handleMaxTime(arguments);
    }
    else if (command.compare("maxmessages") == 0)
    {
        handleMaxMessages(arguments);
    }
    else if (command.compare("creaturevisdebug") == 0)
    {
        handleCreatureVisDebug(arguments);
    }
    else if (command.compare("seatvisdebug") == 0)
    {
        handleSeatVisDebug(arguments);
    }
    else if (command.compare("icanseedeadpeople") == 0)
    {
        handleICanSeeDeadPeople(arguments);
    }
    else if (command.compare("setcreaturelevel") == 0)
    {
        handleSetCreatureLevel(arguments);
    }
    else if (command.compare("circlearound") == 0)
    {
        handleCircleAround(arguments);
    }
    else if (command.compare("catmullspline") == 0)
    {
        handleCatmullSpline(arguments);
    }
    else if (command.compare("setcamerafovy") == 0)
    {
        handleSetCameraFOVy(arguments);
    }
    else if (command.compare("addgold") == 0)
    {
        handleAddGold(arguments);
    }
    else if (command.compare("setcreaturedest") == 0)
    {
        handleSetCreatureDest(arguments);
    }
    else if (command.compare("logfloodfill") == 0)
    {
        handleLogFloodFill(arguments);
    }
    else if (command.compare("listmeshanims") == 0)
    {
        handleListMeshAnims(arguments);
    }
    else if (command.compare("triggercompositor") == 0)
    {
        RenderManager::getSingletonPtr()->triggerCompositor(arguments);
    }
    else
    {
        mCommandOutput +=
            "\nCommand not found.  Try typing help to get info on how to use the console "
            "or just press F12 to exit the console and return to the game.\n";
    }

    Console::getSingleton().print(mCommandOutput);

    return true;
}

void ConsoleCommandsHandler::handleAmbientLight(const::std::string& arguments)
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();

    if (arguments.empty())
    {
        // Display the current ambient light values.
        Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
        mCommandOutput += "\nCurrent ambient light is:\nRed:  "
                + Ogre::StringConverter::toString((Ogre::Real) curLight.r)
                + "    Green:  " + Ogre::StringConverter::toString(
                (Ogre::Real) curLight.g) + "    Blue:  "
                + Ogre::StringConverter::toString((Ogre::Real) curLight.b) + "\n";
        return;
    }

    // Set the new color.
    Ogre::Real tempR, tempG, tempB = 0.0;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> tempR >> tempG >> tempB;

    mSceneMgr->setAmbientLight(Ogre::ColourValue(tempR, tempG, tempB));
    mCommandOutput += "\nAmbient light set to:\nRed:  "
            + Ogre::StringConverter::toString((Ogre::Real) tempR) + "    Green:  "
            + Ogre::StringConverter::toString((Ogre::Real) tempG) + "    Blue:  "
            + Ogre::StringConverter::toString((Ogre::Real) tempB) + "\n";

}

void ConsoleCommandsHandler::handleFPS(const::std::string& arguments)
{
    if (arguments.empty())
    {
        mCommandOutput += "\nCurrent maximum framerate is "
                + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                + "\n";
        return;
    }

    Ogre::Real newFPS;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> newFPS;
    ODApplication::MAX_FRAMES_PER_SECOND = newFPS;
    mCommandOutput += "\nMaximum framerate set to "
            + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
            + "\n";
}

void ConsoleCommandsHandler::handleNearClip(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    if (arguments.empty())
    {
        mCommandOutput += "\nCurrent near clip distance is "
                + Ogre::StringConverter::toString(
                        frameListener->getActiveCameraNearClipDistance())
                + "\n";
        return;
    }

    Ogre::Real nearClip;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> nearClip;
    frameListener->setActiveCameraNearClipDistance(nearClip);
    mCommandOutput += "\nNear clip distance set to "
            + Ogre::StringConverter::toString(frameListener->getActiveCameraNearClipDistance())
            + "\n";
}

void ConsoleCommandsHandler::handleFarClip(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    if (arguments.empty())
    {
        mCommandOutput += "\nCurrent far clip distance is "
                + Ogre::StringConverter::toString(frameListener->getActiveCameraFarClipDistance())
                + "\n";
        return;
    }

    Ogre::Real farClip;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> farClip;
    frameListener->setActiveCameraFarClipDistance(farClip);
    mCommandOutput += "\nFar clip distance set to "
            + Ogre::StringConverter::toString(frameListener->getActiveCameraFarClipDistance())
            + "\n";
}

void ConsoleCommandsHandler::handleAddCreature(const std::string& arguments)
{
    if (arguments.empty())
        return;

    if(ODServer::getSingleton().isConnected())
    {
        ServerConsoleCommand* cc = new SCCAddCreature(arguments);
        ODServer::getSingleton().queueConsoleCommand(cc);
        mCommandOutput += "\nCreature added successfully\n";
        return;
    }

    mCommandOutput += "\nERROR : This command is available on the server only.\n";
}

void ConsoleCommandsHandler::handleList(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getClientGameMap();

    if (arguments.empty())
    {
        mCommandOutput += "lists available:\n\t\tclasses\tcreatures\tplayers\n\t\tnetwork\trooms\tcolors\n\t\tgoals\n";
        return;
    }

    std::stringstream stringStr;
    stringStr.str("");

    if (arguments.compare("creatures") == 0)
    {
        stringStr << "Class:\tCreature name:\tLocation:\tColor:\tLHand:\tRHand\n\n";
        for (unsigned int i = 0; i < gameMap->numCreatures(); ++i)
        {
            stringStr << gameMap->getCreature(i) << std::endl;
        }
    }
    else if (arguments.compare("classes") == 0)
    {
        stringStr << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
        for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
        {
            const CreatureDefinition* currentClassDesc = gameMap->getClassDescription(i);
            stringStr << currentClassDesc << "\n";
        }
    }
    else if (arguments.compare("players") == 0)
    {
        if (ODClient::getSingleton().isConnected())
        {
            stringStr << "Player:\tNick:\tSeatId\tTeamId:\n\n"
                      << "me\t\t" << gameMap->getLocalPlayer()->getNick() << "\t"
                      << gameMap->getLocalPlayer()->getSeat()->getId() << "\t"
                      << gameMap->getLocalPlayer()->getSeat()->getTeamId() << "\n\n";
        }

        if (ODServer::getSingleton().isConnected())
        {
            stringStr << "Player:\tNick:\tSeatId\tTeamId:\n\n";

            for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
            {
                Player* player = gameMap->getPlayer(i);
                stringStr << i << "\t\t" << player->getNick() << "\t"
                        << player->getSeat()->getId() << "\t"
                        << player->getSeat()->getTeamId() << "\n\n";
            }
        }
    }
    else if (arguments.compare("network") == 0)
    {
        if (!frameListener->getModeManager()->getCurrentModeType() == ModeManager::EDITOR)
        {
            stringStr << "You are currently in the map editor.";
        }
        else if (ODServer::getSingleton().isConnected())
        {
            stringStr << "You are currently acting as a server.";
        }
        else if (ODClient::getSingleton().isConnected())
        {
            stringStr << "You are currently connected to a server.";
        }
    }
    else if (arguments.compare("rooms") == 0)
    {
        stringStr << "Name:\tSeat Id:\tNum tiles:\n\n";
        for (unsigned int i = 0 ; i < gameMap->numRooms(); ++i)
        {
            Room* room = gameMap->getRoom(i);
            stringStr << room->getName() << "\t" << room->getSeat()->getId()
                    << "\t" << room->numCoveredTiles() << "\n";
        }
    }
    else if (arguments.compare("colors") == 0 || arguments.compare("colours") == 0)
    {
        stringStr << "Number:\tRed:\tGreen:\tBlue:\n";
        const std::vector<Seat*> seats = gameMap->getSeats();
        for (Seat* seat : seats)
        {
            Ogre::ColourValue color = seat->getColorValue();

            stringStr << "\n" << seat->getId() << "\t\t" << color.r
                    << "\t\t" << color.g << "\t\t" << color.b;
        }
    }
    else if (arguments.compare("goals") == 0 && ODClient::getSingleton().isConnected())
    {
        // Loop over the list of unmet goals for the seat we are sitting in an print them.
        stringStr << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
        for (unsigned int i = 0; i < gameMap->getLocalPlayer()->getSeat()->numUncompleteGoals(); ++i)
        {
            Seat* s = gameMap->getLocalPlayer()->getSeat();
            Goal* tempGoal = s->getUncompleteGoal(i);
            stringStr << tempGoal->getName() << ":\t"
                    << tempGoal->getDescription(s) << "\n";
        }

        // Loop over the list of completed goals for the seat we are sitting in an print them.
        stringStr << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
        for (unsigned int i = 0; i < gameMap->getLocalPlayer()->getSeat()->numCompletedGoals(); ++i)
        {
            Seat* seat = gameMap->getLocalPlayer()->getSeat();
            Goal* tempGoal = seat->getCompletedGoal(i);
            stringStr << tempGoal->getName() << ":\t"
                    << tempGoal->getSuccessMessage(seat) << "\n";
        }
    }
    else
    {
        stringStr << "ERROR:  Unrecognized list.  Type \"list\" with no arguments to see available lists.";
    }

    mCommandOutput += "+\n" + stringStr.str() + "\n";
}

void ConsoleCommandsHandler::handleMaxTime(const std::string& arguments)
{
    std::stringstream stringStr;
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    if (arguments.empty())
    {
        stringStr << "Max display time for chat messages is: "
                << frameListener->getChatMaxTimeDisplay();
        mCommandOutput += "\n " + stringStr.str() + "\n";
        return;
    }

    float chatMaxTimeDisplay = static_cast<float>(Helper::toDouble(arguments));
    frameListener->setChatMaxTimeDisplay(chatMaxTimeDisplay);

    stringStr << "Max display time for chat messages was changed to: "
            << arguments;
    mCommandOutput += "\n " + stringStr.str() + "\n";
}

void ConsoleCommandsHandler::handleMaxMessages(const std::string& arguments)
{
    std::stringstream stringStr;
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();

    if (arguments.empty())
    {
        stringStr << "Max displayed chat messages is: "
                << frameListener->getChatMaxMessages();
        mCommandOutput += "\n " + stringStr.str() + "\n";
        return;
    }

    float chatMaxMessages = static_cast<float>(Helper::toDouble(arguments));
    frameListener->setChatMaxMessages(chatMaxMessages);

    stringStr << "Max displayed chat messages was changed to: "
            << arguments;
    mCommandOutput += "\n " + stringStr.str() + "\n";
}

void ConsoleCommandsHandler::handleCreatureVisDebug(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        return;
    }

    if(!ODClient::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  You must have joined a game to toggle debug entities.\n";
        return;
    }

    if (arguments.empty())
    {
        mCommandOutput += "\nERROR:  You must supply a valid creature name to create debug entities for.\n";
        return;
    }

    GameMap* gameMap = frameListener->getClientGameMap();
    Creature* creature = gameMap->getCreature(arguments);
    if (creature == nullptr)
    {
        mCommandOutput += "\nCould not create visual debugging entities for creature: "
                          + arguments + "\n";
        return;
    }

    if (creature->getHasVisualDebuggingEntities())
    {
        const std::string& name = creature->getName();
        ServerConsoleCommand* cc = new SCCDisplayCreatureVisualDebug(name, false);
        ODServer::getSingleton().queueConsoleCommand(cc);

        mCommandOutput += "\nVisual debugging entities destroyed for creature: "
                          + arguments + "\n";
    }
    else
    {
        const std::string& name = creature->getName();
        ServerConsoleCommand* cc = new SCCDisplayCreatureVisualDebug(name, true);
        ODServer::getSingleton().queueConsoleCommand(cc);

        mCommandOutput += "\nVisual debugging entities created for creature: "
                          + arguments + "\n";
    }
}

void ConsoleCommandsHandler::handleSeatVisDebug(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if (!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        return;
    }

    if(!ODClient::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  You must have joined a game to toggle debug entities.\n";
        return;
    }

    if (arguments.empty())
    {
        mCommandOutput += "\nERROR:  You must supply a valid seat id debug vision for.\n";
        return;
    }

    int seatId = Helper::toInt(arguments);

    GameMap* gameMap = frameListener->getClientGameMap();
    Seat* seat = gameMap->getSeatById(seatId);
    if (seat == nullptr)
    {
        mCommandOutput += "\nCould not create visual debugging entities for seat: "
                          + arguments + "\n";
        return;
    }

    if (seat->getIsDebuggingVision())
    {
        ServerConsoleCommand* cc = new SCCDisplaySeatVisualDebug(seatId, false);
        ODServer::getSingleton().queueConsoleCommand(cc);

        mCommandOutput += "\nVisual debugging entities destroyed for seat: "
                          + arguments + "\n";
    }
    else
    {
        ServerConsoleCommand* cc = new SCCDisplaySeatVisualDebug(seatId, true);
        ODServer::getSingleton().queueConsoleCommand(cc);

        mCommandOutput += "\nVisual debugging entities created for seat: "
                          + arguments + "\n";
    }
}

void ConsoleCommandsHandler::handleICanSeeDeadPeople(const std::string& /*arguments*/)
{
    if (ODServer::getSingleton().isConnected())
    {
        if(ODClient::getSingleton().isConnected())
        {
            ServerConsoleCommand* cc = new SCCAskToggleFOW();
            ODServer::getSingleton().queueConsoleCommand(cc);
        }
        mCommandOutput += "\nAsking to toggle fog of war\n";
    }
    else
    {
        mCommandOutput += "\nERROR:  You can toggle fog of war only when you are hosting a game.\n";
    }
}

void ConsoleCommandsHandler::handleSetCreatureLevel(const std::string& arguments)
{
    if (!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  Only the server can change a creature level.\n";
        return;
    }

    if(!ODClient::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR:  You must have joined a game to set a creature level.\n";
        return;
    }

    if (arguments.empty())
    {
        mCommandOutput += "\nERROR:  You must supply a valid creature name.\n";
        return;
    }

    std::stringstream stringStr;
    stringStr.str(arguments);
    std::string name;
    int level;
    stringStr >> name;
    stringStr >> level;

    ServerConsoleCommand* cc = new SCCSetLevelCreature(name, level);
    ODServer::getSingleton().queueConsoleCommand(cc);

    mCommandOutput += "\nCommand sent to change creature level: " + arguments + "\n";
}

void ConsoleCommandsHandler::handleCircleAround(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if(arguments.empty())
    {
        mCommandOutput += "\nERROR:  You need to specify an circle center "
                          "(two coordinates) and circle radius\n";
        return;
    }

    CameraManager* cm = frameListener->getCameraManager();
    double centerX;
    double centerY;
    double radius;

    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> centerX >> centerY >> radius;

    cm->circleAround((int)centerX, (int)centerY, (unsigned int)radius);
}

void ConsoleCommandsHandler::handleCatmullSpline(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if(arguments.empty())
    {
        mCommandOutput += "\nERROR:  You need to specify at least one coordinate's pair.\n";
        return;
    }

    CameraManager* cm = frameListener->getCameraManager();
    int nn = 0;

    int tempInt1 = 0;
    int tempInt2 = 0;

    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> nn;
    cm->resetHCSNodes(nn);

    for (int ii = 0 ; ii < nn ; ++ii)
    {
        stringStr >> tempInt1;
        stringStr >> tempInt2;

        cm->addHCSNodes(tempInt1, tempInt2);
    }

    cm->setCatmullSplineMode(true);
}

void ConsoleCommandsHandler::handleSetCameraFOVy(const std::string& arguments)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    if(arguments.empty())
    {
        mCommandOutput += "\nERROR:  you need to specify an angle in radians \n";
        return;
    }

    CameraManager* cm = frameListener->getCameraManager();
    double angle = Helper::toDouble(arguments);
    Ogre::Radian radianAngle((Ogre::Real)angle);

    cm->getActiveCamera()->setFOVy(radianAngle);
}

void ConsoleCommandsHandler::handleAddGold(const std::string& arguments)
{
    if(!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR : This command is available on the server only\n";
        return;
    }

    int seatId, gold = 0;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> seatId >> gold;

    ServerConsoleCommand* cc = new SCCAddGold(gold, seatId);
    ODServer::getSingleton().queueConsoleCommand(cc);
}

void ConsoleCommandsHandler::handleSetCreatureDest(const std::string& arguments)
{
    if(!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR : This command is available on the server only\n";
        return;
    }

    std::string creatureName;
    int x, y = 0;
    std::stringstream stringStr;
    stringStr.str(arguments);
    stringStr >> creatureName >> x >> y;

    ServerConsoleCommand* cc = new SCCSetCreatureDestination(creatureName, x, y);
    ODServer::getSingleton().queueConsoleCommand(cc);
}

void ConsoleCommandsHandler::handleLogFloodFill(const std::string& /*arguments*/)
{
    if(!ODServer::getSingleton().isConnected())
    {
        mCommandOutput += "\nERROR : This command is available on the server only\n";
        return;
    }

    ServerConsoleCommand* cc = new SCCLogFloodFill();
    ODServer::getSingleton().queueConsoleCommand(cc);
}

void ConsoleCommandsHandler::handleListMeshAnims(const std::string& arguments)
{
    std::string anims = RenderManager::consoleListAnimationsForMesh(arguments);
    mCommandOutput += "\nAnimations for " + arguments + ": " + anims;
}
