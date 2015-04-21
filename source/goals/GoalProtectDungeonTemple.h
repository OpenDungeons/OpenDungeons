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

#ifndef GOALPROTECTDUNGEONTEMPLE_H
#define GOALPROTECTDUNGEONTEMPLE_H

#include "goals/Goal.h"

class GoalProtectDungeonTemple: public Goal
{
public:
    // Constructors
    GoalProtectDungeonTemple(const std::string& nName, const std::string& nArguments);
    virtual ~GoalProtectDungeonTemple()
    {}

    // Inherited functions
    bool isMet(const Seat& s, const GameMap& gameMap);
    bool isUnmet(const Seat&, const GameMap& gameMap);
    bool isFailed(const Seat&, const GameMap& gameMap);
    std::string getDescription(const Seat&);
    std::string getSuccessMessage(const Seat&);
    std::string getFailedMessage(const Seat&);
};

#endif // GOALPROTECTDUNGEONTEMPLE_H
