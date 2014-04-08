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
        findHome, // Try to find a "home" tile in a quarters somewhere where the creature can sleep.
        sleep, // Try to go to its home tile to and sleep when it gets there.
        train, // Check to see if our seat controls a dojo, and if so go there to train.
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
