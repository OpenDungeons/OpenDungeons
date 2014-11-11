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

#include "ModeManagerInterface.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

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
    using String_t = std::string;
    using ArgumentList_t = std::vector<String_t>;
    using CommandFunction_t = std::function<Result(const ArgumentList_t&, ConsoleInterface&, ModeManagerInterface*)>;

    Command(CommandFunction_t command, String_t description, std::initializer_list<ModeType> allowedModes)
        : mCommand(command), mDescription(description), mAllowedModes(allowedModes)
    {}
    Result execute(const ArgumentList_t&, ModeType mode, ConsoleInterface&, ModeManagerInterface*);
    const String_t& getDescription()
    {
        return mDescription;
    }
    bool isAllowedInMode(ModeType mode)
    {
        auto it = std::find(mAllowedModes.begin(), mAllowedModes.end(), mode);
        if(it != mAllowedModes.end())
        {
            return true;
        }
        it = std::find(mAllowedModes.begin(), mAllowedModes.end(), ModeType::ALL);
        return ( it != mAllowedModes.end());
    }

private:
    CommandFunction_t mCommand;
    String_t mDescription;
    std::vector<ModeType> mAllowedModes;
};

#endif // COMMAND_H
