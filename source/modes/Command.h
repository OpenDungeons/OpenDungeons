/*
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

#ifndef COMMAND_H
#define COMMAND_H

#include "AbstractModeManager.h"

#include <algorithm>
#include <functional>
#include <string>
#include <bitset>

class ConsoleInterface;

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
    using CommandFunction_t = std::function<Result(const ArgumentList_t&, ConsoleInterface&, AbstractModeManager&)>;

    Command(CommandFunction_t command, String_t description, std::initializer_list<ModeType> allowedModes)
        : mCommand(command), mDescription(description)
    {
        for(ModeType m : allowedModes)
        {
            mAllowedModes.set(static_cast<std::size_t>(m), true);
        }
    }

    /** \brief Run the command associated with this command object.
     *  \param args List of arguments to function.
     *  \param mode The current mode.
     *  \param c The console instance (mainly for printing).
     *  \param mm ModeManager, for communicating with the game.
     *  \returns Result containing info on wheter the command was successful or not.
     */
    Result execute(const ArgumentList_t& args, ModeType mode, ConsoleInterface& c, AbstractModeManager& mm);

    //! \brief Get the description of this command.
    const String_t& getDescription() const
    {
        return mDescription;
    }

    //! \brief Check if this command is allowed in the speficied mode.
    bool isAllowedInMode(ModeType mode) const
    {
        return mAllowedModes[static_cast<std::size_t>(ModeType::ALL)] || mAllowedModes[static_cast<std::size_t>(mode)];
    }

private:
    CommandFunction_t mCommand;
    String_t mDescription;
    std::bitset<ModeType::NUM_ELEMS> mAllowedModes;
};

#endif // COMMAND_H
