/*!
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

#ifndef _CONSOLE_COMMANDS_HANDLER_
#define _CONSOLE_COMMANDS_HANDLER_

#include <string>
#include <map>

#include "modes/PrefixTree.h"

class ConsoleCommandsHandler
{
public:
    ConsoleCommandsHandler()
    { loadCommands(); }

    //! \brief Returns the general help message.
    const std::string& getGeneralHelpMessage() const;

    //! \brief Gets the corresponding help for
    const std::string& getHelpForCommand(const std::string& command);

    //! \brief Proxy for the auto-completing the current word with the given reference list
    bool autocomplete(const char* word, std::vector<std::string>& autocompletedWords);

    //! \brief Execute the given command.
    bool executePromptCommand(const std::string& command, const::std::string& arguments);
private:
    //! \brief General commands help
    std::map<std::string, std::string> mCommandsHelp;

    //! \brief The commands prefix tree used for autocompletion.
    PrefixTree mCommandsPrefixTree;

    //! \brief The console complete output
    std::string mCommandOutput;

    //! \brief Loads the console commands and description into memory,
    //! and set the prefix list.
    void loadCommands();

    //! \brief Handles the 'ambientlight' command.
    //! Change the ambient light color of the level.
    void handleAmbientLight(const::std::string& arguments);

    //! \brief Handles the 'fps' command.
    //! Gets or sets the max frames per second.
    void handleFPS(const::std::string& arguments);

    //! \brief Handles the 'nearclip' command.
    //! Set the camera near clip distance.
    void handleNearClip(const::std::string& arguments);

    //! \brief Handles the 'farclip' command.
    //! Set the camera far clip distance.
    void handleFarClip(const::std::string& arguments);

    //! \brief Handles the 'addcreature' command.
    //! Add the creature with the given stats.
    void handleAddCreature(const::std::string& arguments);

    //! \brief Handles the 'list' command.
    //! List different info.
    void handleList(const::std::string& arguments);

    //! \brief Handles the 'maxtime' command.
    //! Gets/Sets the max time a chat message is displayed.
    void handleMaxTime(const::std::string& arguments);

    //! \brief Handles the 'maxmessages' command.
    //! Gets/Sets the maximum chat lines displayed at once.
    void handleMaxMessages(const::std::string& arguments);

    //! \brief Handles the 'creaturevisdebug' command.
    //! Toggles the visual vision debug entities visibility for the given creature.
    void handleCreatureVisDebug(const::std::string& arguments);

    //! \brief Handles the 'seatvisdebug' command.
    //! Toggles the visual vision debug entities visibility for the given seat.
    void handleSeatVisDebug(const::std::string& arguments);

    //! \brief Handles the 'icanseedeadpeople' command.
    //! Toggles the Fog of War.
    void handleICanSeeDeadPeople(const::std::string& arguments);

    //! \brief Handles the 'setcreaturelevel' command.
    //! Set the creature level
    void handleSetCreatureLevel(const::std::string& arguments);

    //! \brief Handles the 'circlearound' command.
    //! Makes the camera do a circle around a given location and a given radius.
    void handleCircleAround(const::std::string& arguments);

    //! \brief Handles the 'catmullspline' command.
    //! Makes the camera move along a list of coordinates in an smooth manner.
    void handleCatmullSpline(const::std::string& arguments);

    //! \brief Handles the 'setcamerafovy' command.
    //! Sets the camera field of view Y dimension.
    void handleSetCameraFOVy(const::std::string& arguments);

    //! \brief Handles the 'addgold' command.
    //! Adds Gold to the given seat.
    void handleAddGold(const::std::string& arguments);

    //! \brief Handles the 'setcreaturedest' command.
    //! Sets a tile destination to the given creature
    void handleSetCreatureDest(const::std::string& arguments);

    //! \brief Handles the 'logfloodfill' command.
    //! Logs the pathfinding floodfill state.
    void handleLogFloodFill(const::std::string& arguments);

    //! \brief Handles the 'listmeshanims' command.
    //! Lists the available mesh animations.
    void handleListMeshAnims(const::std::string& arguments);
};

#endif // _CONSOLE_COMMANDS_HANDLER_
