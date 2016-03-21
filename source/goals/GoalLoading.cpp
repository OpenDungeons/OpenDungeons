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

#include "goals/GoalLoading.h"

#include "goals/AllGoals.h"
#include "utils/MakeUnique.h"

#include <istream>
#include <ostream>

namespace Goals
{
std::unique_ptr<Goal> loadGoalFromStream(const std::string& goalName, std::istream& is)
{
    std::string tempArguments;
    std::unique_ptr<Goal> tempGoal;

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
        tempGoal = Utils::make_unique<GoalKillAllEnemies>(goalName, tempArguments);
    }

    else if (goalName.compare("ProtectCreature") == 0)
    {
        tempGoal = Utils::make_unique<GoalProtectCreature>(goalName, tempArguments);
    }

    else if (goalName.compare("ClaimNTiles") == 0)
    {
        tempGoal = Utils::make_unique<GoalClaimNTiles>(goalName, tempArguments);
    }

    else if (goalName.compare("MineNGold") == 0)
    {
        tempGoal = Utils::make_unique<GoalMineNGold>(goalName, tempArguments);
    }

    else if (goalName.compare("ProtectDungeonTemple") == 0)
    {
        tempGoal = Utils::make_unique<GoalProtectDungeonTemple>(goalName, tempArguments);
    }
    //FIXME: This is going to break if the level file is malformed.

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
          tempGoal->addSuccessSubGoal(loadGoalFromStream(subGoalName, is));
    }
    else if (c == '-')
    {
        // There is a subgoal which should be added on failure.
        is.ignore(1);
        is >> numSubgoals;
        is >> subGoalName;
        for (int i = 0; i < numSubgoals; ++i)
          tempGoal->addFailureSubGoal(loadGoalFromStream(subGoalName, is));
    }

    return tempGoal;
}
} // namespace Goals

std::ostream& operator<<(std::ostream& os, Goal &g)
{
    unsigned int subGoals;

    os << g.mName << "\t";
    os << (g.mArguments.size() > 0 ? g.mArguments : "nullptr") << "\n";

    subGoals = g.numSuccessSubGoals();
    if (subGoals > 0)
    {
        os << "+ " << subGoals << "\n";
        for (unsigned int i = 0; i < subGoals; ++i)
            os << *g.getSuccessSubGoal(i);
    }

    subGoals = g.numFailureSubGoals();
    if (subGoals > 0)
    {
        os << "- " << subGoals << "\n";
        for (unsigned int i = 0; i < subGoals; ++i)
            os << *g.getFailureSubGoal(i);
    }

    return os;
}
