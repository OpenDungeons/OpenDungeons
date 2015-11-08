/*
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

#ifndef CONSOLEINTERFACE_H
#define CONSOLEINTERFACE_H

#include "Command.h"

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <map>
#include <vector>

class GameMap;

//! \class ConsoleInterface
//! \brief A class that implements a console
class ConsoleInterface
{
public:
    using ModeType = AbstractModeManager::ModeType;
    using String_t = Command::String_t;
    using PrintFunction_t = std::function<void(const String_t&)>;
    using CommandClientFunction_t = Command::CommandClientFunction_t;
    using CommandServerFunction_t = Command::CommandServerFunction_t;

    ConsoleInterface(PrintFunction_t printFunction);

    //! \brief Calls the print function using the supplied string
    inline void print(String_t string)
    {
        mPrintFunction(string);
    }

    //! \brief Adds a new command
    //! \param name The name of the command
    //! \param description A description of what the command does
    //! \param command The commnad to be run
    //! \param allowedModes A list of modes this command is allowed to run in.
    //! \param aliases And optional list of aliases
    bool addCommand(String_t name, String_t description,
                    CommandClientFunction_t commandClient,
                    CommandServerFunction_t commandServer,
                    std::initializer_list<ModeType> allowedModes,
                    std::initializer_list<String_t> aliases = {});

    //! \brief Try executing a command on client side
    //! \param commandString The command string to be executed
    //! \returns Command::Result::SUCCESS if the command succeeds, Command::Result::WRONG_MODE if the
    //!     command is ran in the wrong mode, Command::Result::INVALID_ARGUMENT if one or more of the arguments
    //!     can't be parsed, and Command::Result::FAILED if the command fails.
    Command::Result tryExecuteClientCommand(String_t commandString, ModeType modeType, AbstractModeManager& modeManager);

    //! \brief Try executing a command on server side
    //! \param commandString The command string to be executed
    //! \returns Command::Result::SUCCESS if the command succeeds
    Command::Result tryExecuteServerCommand(const std::vector<std::string>& args, GameMap& gameMap);

    //! \brief Try to complete the command from the string prefix.
    //! \returns The completed string if the lookup succeeds, boost::none if the lookup
    //! fails or there are multiple alternatives.
    boost::optional<const String_t&> tryCompleteCommand(const String_t& prefix);

    //! \brief Try to scroll up in the command history.
    //! \returns The previous command if it exists, otherwise boost::none
    boost::optional<const String_t&> scrollCommandHistoryPositionUp(const String_t& currentPrompt);

    //! \brief Try to scroll down in the command history
    //! \returns Next command if it exists, otherwise boost::none
    boost::optional<const String_t&> scrollCommandHistoryPositionDown();

private:
    using CommandPtr_t = std::shared_ptr<Command>;

    inline bool isInHistory() const
    {
        return mHistoryPos != mCommandHistoryBuffer.end();
    }

    inline void resetCommandHistoryPosition()
    {
        mHistoryPos = mCommandHistoryBuffer.end();
        mTemporaryCommandString.clear();
    }

    //! \brief Look up the description for the command
    boost::optional<const String_t&> getCommandDescription(const String_t& command);
    static Command::Result helpCommand(const Command::ArgumentList_t& args, ConsoleInterface& console, AbstractModeManager&);

    ConsoleInterface(const ConsoleInterface&) = delete;
    ConsoleInterface& operator=(const ConsoleInterface&) = delete;

    PrintFunction_t mPrintFunction;
    using CommandMap_t = std::map<String_t,CommandPtr_t>;
    CommandMap_t mCommandMap;
    using CommandHistoryBufferList_t = std::list<String_t>;
    CommandHistoryBufferList_t mCommandHistoryBuffer;
    CommandHistoryBufferList_t::iterator mHistoryPos;
    String_t mTemporaryCommandString;
};

#endif // CONSOLEINTERFACE_H
