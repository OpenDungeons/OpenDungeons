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

#define BOOST_TEST_MODULE CreatureDefinition
#include "BoostTestTargetConfig.h"

#include "goals/Goal.h"
#include "utils/LogManager.h"
#include "utils/LogSinkConsole.h"

class GameMap
{
};
class Seat
{
};

class TestGoal : public Goal
{
public:
    TestGoal(const std::string& nName, const std::string& nArguments)
      : Goal(nName, nArguments)
    {}

    virtual bool isMet(const Seat&, const GameMap&) override
    {
        return true;
    }

    virtual std::string getDescription(const Seat&) override
    {
        return "desc";
    }

    virtual std::string getSuccessMessage(const Seat&) override
    {
        return "success";
    }
    virtual std::string getFailedMessage(const Seat&) override
    {
        return "failed";
    }
};

BOOST_AUTO_TEST_CASE(test_Goal)
{
    //TODO: Write tests once goal dependencies are testable
    LogManager logMgr;
    logMgr.addSink(std::unique_ptr<LogSink>(new LogSinkConsole()));
    TestGoal g("name", "arguments");
    BOOST_CHECK(g.isMet(Seat(), GameMap()));
}
