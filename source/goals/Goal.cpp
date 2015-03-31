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

#include "goals/Goal.h"

Goal::Goal(const std::string& nName, const std::string& nArguments) :
    mName(nName),
    mArguments(nArguments)
{
}

void Goal::doSuccessAction()
{
}

bool Goal::isVisible()
{
    return true;
}

bool Goal::isUnmet(const Seat& s, const GameMap& gameMap)
{
    return !isMet(s, gameMap);
}

bool Goal::isFailed(const Seat& s, const GameMap&)
{
    return false;
}

void Goal::addSuccessSubGoal(std::unique_ptr<Goal>&& g)
{
    mSuccessSubGoals.emplace_back(std::move(g));
}

unsigned int Goal::numSuccessSubGoals() const
{
    return mSuccessSubGoals.size();
}

Goal* Goal::getSuccessSubGoal(int index)
{
    return mSuccessSubGoals[index].get();
}

void Goal::addFailureSubGoal(std::unique_ptr<Goal>&& g)
{
    mFailureSubGoals.emplace_back(std::move(g));
}

unsigned int Goal::numFailureSubGoals() const
{
    return mFailureSubGoals.size();
}

Goal* Goal::getFailureSubGoal(int index)
{
    return mFailureSubGoals[index].get();
}

std::string Goal::getFormat()
{
    return "goalName\targuments";
}

