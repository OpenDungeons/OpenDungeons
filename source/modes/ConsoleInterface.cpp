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

#include "ConsoleInterface.h"

#include <regex>

namespace
{
    const std::regex NO_WHITESPACE("\\S*");
}

ConsoleInterface::ConsoleInterface(PrintFunction_t printFunction)
    :mPrintFunction(printFunction), mCommandHistoryBuffer(), mHistoryPos(mCommandHistoryBuffer.begin())
{
   addCommand("help",
              ">help Lists available commands\n>help <command> displays description for <command>",
              ConsoleInterface::helpCommand,
              {ModeType::ALL}
              );
}

bool ConsoleInterface::addCommand(String_t name, String_t description, CommandFunction_t command,
                std::initializer_list<ModeType> allowedModes,
                std::initializer_list<String_t> aliases)
{
    if(std::regex_match(name, NO_WHITESPACE))
    {
        CommandPtr_t commandPtr = std::make_shared<Command>(command, description, allowedModes);
        mCommandMap.emplace(name, commandPtr);
        for(auto& alias : aliases)
        {
            if(std::regex_match(alias, NO_WHITESPACE))
            {
                mCommandMap.emplace(alias, commandPtr);
            }
            else
            {
                print("ERROR: Alias: '" + alias + "'' contains non-alphanumeric characters!");
            }
        }
        return true;
    }
    else
    {
        print("ERROR: Command: '" + name + "' contains non-alphanumeric characters!");
        return false;
    }
}

Command::Result ConsoleInterface::tryExecuteCommand(String_t commandString,
                                                    ModeType modeType,
                                                    AbstractModeManager& modeManager)
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
        try
        {
            Command::Result result = it->second->execute(tokenList, modeType, *this, modeManager);
            return result;
        }
        catch(const std::invalid_argument& e)
        {
            print(String_t("Invalid argument: ") + e.what());
            return Command::Result::INVALID_ARGUMENT;
        }
        catch(const std::out_of_range& e)
        {
            print(String_t("Argument out of range") + e.what());
            return Command::Result::INVALID_ARGUMENT;
        }
    }
    else
    {
        print("Unknown command: \"" + commandString + "\"");
        return Command::Result::NOT_FOUND;
    }
}

boost::optional<const ConsoleInterface::String_t&> ConsoleInterface::tryCompleteCommand(const String_t& prefix)
{
    std::vector<const String_t*> matches;
    for(auto& element : mCommandMap)
    {
        if(boost::algorithm::starts_with(element.first, prefix))
        {
            matches.push_back(&element.first);
        }
    }
    if(matches.size() > 1)
    {
        for(auto match : matches)
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

boost::optional<const ConsoleInterface::String_t&> ConsoleInterface::scrollCommandHistoryPositionUp(const ConsoleInterface::String_t& currentPrompt)
{
    //If the list is empty, or we are at the top, return none
    if(mCommandHistoryBuffer.empty() || mHistoryPos == mCommandHistoryBuffer.begin())
    {
        return boost::none;
    }
    else
    {
        if(!isInHistory())
        {
            mTemporaryCommandString = currentPrompt;
        }
        --mHistoryPos;
        return boost::optional<const String_t&>(*mHistoryPos);
    }
}

boost::optional<const ConsoleInterface::String_t&> ConsoleInterface::scrollCommandHistoryPositionDown()
{
    //If we are at the start, don't scroll
    if(!isInHistory())
    {
        return boost::none;
    }
    else
    {
        ++mHistoryPos;
        if(mHistoryPos == mCommandHistoryBuffer.end())
        {
            return boost::optional<const String_t&>(mTemporaryCommandString);
        }
        return boost::optional<const String_t&>(*mHistoryPos);
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


Command::Result ConsoleInterface::helpCommand(const Command::ArgumentList_t& args, ConsoleInterface& console, AbstractModeManager&)
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
