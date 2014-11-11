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

#include "ConsoleInterface.h"

Command::Result ConsoleInterface::tryExecuteCommand(String_t commandString,
                                                    ModeType modeType,
                                                    ModeManagerInterface* modeManager)
{
    mCommandHistoryBuffer.emplace_back(commandString);
    print(">> " + commandString);
    Command::ArgumentList_t tokenList;
    boost::algorithm::split(tokenList,
                            commandString, boost::algorithm::is_space(),
                            boost::algorithm::token_compress_on);

    const String_t& commandName = tokenList.front();
    auto it = mCommandMap.find(commandName);
    if(it != mCommandMap.end())
    {
        return it->second->execute(tokenList, modeType, *this, modeManager);
    }
    else
    {
        print("Unknown command: \"" + commandString + "\"");
        return Command::Result::NOT_FOUND;
    }
}

boost::optional<ConsoleInterface::String_t> ConsoleInterface::tryCompleteCommand(const String_t &prefix)
{

    std::list<const String_t*> matches;
    for(auto& element : mCommandMap)
    {
        if(boost::algorithm::starts_with(element.first, prefix))
        {
            matches.push_back(&element.first);
        }
    }
    if(matches.size() > 1)
    {
        matches.sort([](const String_t*& lhs, const String_t*& rhs){return *lhs < *rhs;});
        for(auto& match : matches)
        {
            print(*match);
        }
        print("\n");
        return boost::none;
    }
    else if(matches.size() == 1)
    {
        return *(*matches.begin());
    }
    else
    {
        return boost::none;
    }
}

boost::optional<const ConsoleInterface::String_t&> ConsoleInterface::getCommandDescription(const String_t& command)
{
    auto it = mCommandMap.find(command);
    if(it != mCommandMap.end())
    {
        return boost::optional<const String_t&>(it->second->getDescription());
    }
    else
    {
        return boost::none;
    }
}


Command::Result ConsoleInterface::helpCommand(const Command::ArgumentList_t& args, ConsoleInterface& console, ModeManagerInterface*)
{
    if(args.size() > 1)
    {
        auto result = console.getCommandDescription(args[1]);
        if(result)
        {
            console.print("Help for command \"" + args[1] + "\":\n");
            console.print(*result + "\n");
        }
        else
        {
            console.print("Help for command \"" + args[1] + "\" not found!\n");
        }
    }
    else
    {
        console.print("Commands:\n");
        for(auto& it : console.mCommandMap)
        {
            console.print(it.first);
        }
        console.print("\n");
    }
    return Command::Result::SUCCESS;
}
