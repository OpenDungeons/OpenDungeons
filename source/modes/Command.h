/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef COMMAND_H
#define COMMAND_H

#include "AbstractModeManager.h"

#include <algorithm>
#include <functional>
#include <string>
#include <bitset>
#include <vector>

class ConsoleInterface;
class GameMap;

//! \class Command
//! \brief A class containing a console command, and information about it.
class Command
{
public:
    enum class Result
    {
        NOT_FOUND,
        INVALID_ARGUMENT,
        WRONG_MODE,
        FAILED,
        SUCCESS
    };
    using ModeType = AbstractModeManager::ModeType;
    using String_t = std::string;
    using ArgumentList_t = std::vector<String_t>;
    using CommandClientFunction_t = std::function<Result(const ArgumentList_t&, ConsoleInterface&, AbstractModeManager&)>;
    using CommandServerFunction_t = std::function<Result(const ArgumentList_t&, ConsoleInterface&, GameMap&)>;

    Command(CommandClientFunction_t commandClient, CommandServerFunction_t commandServer, String_t description, std::initializer_list<ModeType> allowedModes)
        : mCommandClient(commandClient), mCommandServer(commandServer), mDescription(description)
    {
        for(ModeType m : allowedModes)
            mAllowedModes.set(static_cast<std::size_t>(m), true);
    }

    /** \brief Run the client side command associated with this command object.
     *  \param args List of arguments to function.
     *  \param mode The current mode.
     *  \param c The console instance (mainly for printing).
     *  \param mm ModeManager, for communicating with the game.
     *  \returns Result containing info on wheter the command was successful or not.
     */
    Result executeClient(const ArgumentList_t& args, ModeType mode, ConsoleInterface& c, AbstractModeManager& mm);

    /** \brief Run the server side command associated with this command object.
     *  \param args List of arguments to function.
     *  \param mode The current mode.
     *  \param c The console instance (mainly for printing).
     *  \param mm ModeManager, for communicating with the game.
     *  \returns Result containing info on wheter the command was successful or not.
     */
    Result executeServer(const ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap);

    //! \brief Get the description of this command.
    const String_t& getDescription() const
    { return mDescription; }

    //! \brief Check if this command is allowed in the speficied mode.
    bool isAllowedInMode(ModeType mode) const
    { return mAllowedModes[static_cast<std::size_t>(mode)]; }

    static Result cStubServer(const ArgumentList_t&, ConsoleInterface&, GameMap&)
    { return Result::SUCCESS; }

    static Result cStubClient(const ArgumentList_t&, ConsoleInterface&, AbstractModeManager&)
    { return Result::SUCCESS; }


private:
    CommandClientFunction_t mCommandClient;
    CommandServerFunction_t mCommandServer;
    String_t mDescription;
    std::bitset<ModeType::NUM_ELEMS> mAllowedModes;
};

#endif // COMMAND_H
