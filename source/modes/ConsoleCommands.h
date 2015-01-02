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

#ifndef _CONSOLE_COMMANDS_
#define _CONSOLE_COMMANDS_

#include <string>
#include <map>

#include "modes/PrefixTree.h"

//TODO Merge with ODConsoleCommands.
class ConsoleCommands
{
public:
    ConsoleCommands()
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

    //! \brief Specific help for the 'list' command.
    //! TODO: Support this.
    std::map<std::string, std::string> mListCommandHelp;

    //! \brief The commands prefix tree used for autocompletion.
    PrefixTree mCommandsPrefixTree;

    //! \brief Loads the console commands and description into memory,
    //! and set the prefix list.
    void loadCommands();
};

#endif // _CONSOLE_COMMANDS_
