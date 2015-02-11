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

#ifndef GOAL_H
#define GOAL_H

#include <string>
#include <vector>
#include <ostream>
#include <istream>

class Seat;
class GameMap;

class Goal
{
public:
    // Constructors
    Goal(const std::string& nName, const std::string& nArguments, GameMap* gameMap);
    virtual ~Goal() {}

    // Functions which must be overridden by child classes
    virtual bool isMet(Seat *s) = 0;
    virtual std::string getDescription(Seat *s) = 0;
    virtual std::string getSuccessMessage(Seat *s) = 0;
    virtual std::string getFailedMessage(Seat *s) = 0;

    // Functions which can be overridden (but do not have to be) by child classes
    virtual void doSuccessAction();
    virtual bool isVisible();
    virtual bool isUnmet(Seat *s);
    virtual bool isFailed(Seat *s);

    // Functions which cannot be overridden by child classes
    const std::string& getName() const
    { return mName; }

    void addSuccessSubGoal(Goal *g);
    unsigned numSuccessSubGoals() const;
    Goal* getSuccessSubGoal(int index);

    void addFailureSubGoal(Goal *g);
    unsigned numFailureSubGoals() const;
    Goal* getFailureSubGoal(int index);

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Goal *g);
    static Goal* instantiateFromStream(const std::string& goalName, std::istream& is, GameMap* gameMap);

protected:
    std::string mName;
    std::string mArguments;
    std::vector<Goal*> mSuccessSubGoals;
    std::vector<Goal*> mFailureSubGoals;
    GameMap* mGameMap;
};

#endif // GOAL_H
