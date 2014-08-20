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

#ifndef CREATUREACTION_H
#define CREATUREACTION_H

#include <string>

class Tile;
class Creature;

//! \brief A data structure to be used in the creature AI calculations.
class CreatureAction
{
public:
    enum ActionType
    {
        walkToTile, // Calculate a path to the tile and follow it each turn.
        maneuver, // Like walkToTile but used for combat situations.
        digTile, // (worker only) Dig out a tile, i.e. decrease its fullness.
        claimTile, // (worker only) "Dance" on tile to change its color.
        claimWallTile, // (worker only) "Dance" next to wall tile to change its color and set it as reinforced.
        depositGold, // (worker only) Carry gold that has been mined to a treasury.
        attackObject, // Do damage to an attackableObject withing range, if not in range begin maneuvering.
        findHome, // Try to find a "home" tile in a dormitory somewhere where the creature can sleep.
        findHomeForced, // Try to find a "home" tile in the dormitory where the creature is
        sleep, // Try to go to its home tile to and sleep when it gets there.
        jobdecided, // (fighters only) Check to see if our seat controls a room where we can work (train, forge, search, ...)
        jobforced, // (fighters only)Check to see if we have been dropped on a room where we can work
        eatdecided, // (fighters only) Try to find a hatchery to eat
        eatforced, // (fighters only) Force eating if the creature is dropped in a hatchery
        idle // Stand around doing nothing.
    };

    CreatureAction();
    CreatureAction(const ActionType nType, Tile* nTile = NULL, Creature* nCreature = NULL);

    inline void setType(const ActionType nType)
    { mType = nType; }

    inline const ActionType getType() const
    { return mType; }

    std::string toString() const;

private:
    ActionType mType;
    Tile* mTile;
    Creature* mCreature;
};

#endif // CREATUREACTION_H
