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

#include "goals/Goal.h"

#include "goals/AllGoals.h"

#include "utils/Helper.h"

Goal::Goal(const std::string& nName, const std::string& nArguments, GameMap* gameMap) :
    mName(nName),
    mArguments(nArguments),
    mGameMap(gameMap)
{
}

void Goal::doSuccessAction()
{
}

bool Goal::isVisible()
{
    return true;
}

bool Goal::isUnmet(Seat *s)
{
    return !isMet(s);
}

bool Goal::isFailed(Seat *s)
{
    return false;
}

void Goal::addSuccessSubGoal(Goal *g)
{
    mSuccessSubGoals.push_back(g);
}

unsigned int Goal::numSuccessSubGoals() const
{
    return mSuccessSubGoals.size();
}

Goal* Goal::getSuccessSubGoal(int index)
{
    return mSuccessSubGoals[index];
}

void Goal::addFailureSubGoal(Goal *g)
{
    mFailureSubGoals.push_back(g);
}

unsigned int Goal::numFailureSubGoals() const
{
    return mFailureSubGoals.size();
}

Goal* Goal::getFailureSubGoal(int index)
{
    return mFailureSubGoals[index];
}

std::string Goal::getFormat()
{
    return "goalName\targuments";
}

std::ostream& operator<<(std::ostream& os, Goal *g)
{
    unsigned int subGoals;

    os << g->mName << "\t";
    os << (g->mArguments.size() > 0 ? g->mArguments : "nullptr") << "\n";

    subGoals = g->numSuccessSubGoals();
    if (subGoals > 0)
    {
        os << "+ " << subGoals << "\n";
        for (unsigned int i = 0; i < subGoals; ++i)
            os << g->getSuccessSubGoal(i);
    }

    subGoals = g->numFailureSubGoals();
    if (subGoals > 0)
    {
        os << "- " << subGoals << "\n";
        for (unsigned int i = 0; i < subGoals; ++i)
            os << g->getFailureSubGoal(i);
    }

    return os;
}

Goal* Goal::instantiateFromStream(const std::string& goalName, std::istream& is, GameMap* gameMap)
{
    std::string tempArguments;
    Goal* tempGoal = nullptr;

    // Store the name and arguments of the goal so we can instantiate a specific goal subclass below.
    getline(is, tempArguments);

    // Since getline leaves any leading whitespace we need to cut that off the beginning of the arguments string.
    int count = 0;
    while (tempArguments[count] == '\n' || tempArguments[count] == '\t'
            || tempArguments[count] == ' ')
        ++count;

    if (count > 0)
        tempArguments = tempArguments.substr(count, tempArguments.length());

    // Since entering an empty string in the file would break the file read we represent it with nullptr and then substitute it here.
    if (tempArguments.compare("nullptr") == 0)
        tempArguments = "";

    // Parse the goal type name to find out what subclass of goal tempGoal should be instantiated as.
    if (goalName.compare("KillAllEnemies") == 0)
    {
        tempGoal = new GoalKillAllEnemies(goalName, tempArguments, gameMap);
    }

    else if (goalName.compare("ProtectCreature") == 0)
    {
        tempGoal = new GoalProtectCreature(goalName, tempArguments, gameMap);
    }

    else if (goalName.compare("ClaimNTiles") == 0)
    {
        tempGoal = new GoalClaimNTiles(goalName, tempArguments, gameMap);
    }

    else if (goalName.compare("MineNGold") == 0)
    {
        tempGoal = new GoalMineNGold(goalName, tempArguments, gameMap);
    }

    else if (goalName.compare("ProtectDungeonTemple") == 0)
    {
        tempGoal = new GoalProtectDungeonTemple(goalName, tempArguments, gameMap);
    }

    // Now that the goal has been properly instantiated we check to see if there are subgoals to read in.
    char c;
    c = is.peek();
    std::string subGoalName;
    int numSubgoals;
    if (c == '+')
    {
        // There is a subgoal which should be added on success.
        is.ignore(1);
        is >> numSubgoals;
        is >> subGoalName;
        for (int i = 0; i < numSubgoals; ++i)
            tempGoal->addSuccessSubGoal(instantiateFromStream(subGoalName, is, gameMap));
    }
    else if (c == '-')
    {
        // There is a subgoal which should be added on failure.
        is.ignore(1);
        is >> numSubgoals;
        is >> subGoalName;
        for (int i = 0; i < numSubgoals; ++i)
            tempGoal->addFailureSubGoal(instantiateFromStream(subGoalName, is, gameMap));
    }

    return tempGoal;
}
