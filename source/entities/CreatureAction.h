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

#ifndef CREATUREACTION_H
#define CREATUREACTION_H

#include "entities/GameEntity.h"

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
        fight, // When seeing ennemy objects, the creature might decide to fight
        digTile, // (worker only) Dig out a tile, i.e. decrease its fullness.
        claimTile, // (worker only) "Dance" on tile to change its color.
        claimWallTile, // (worker only) "Dance" next to wall tile to change its color and set it as reinforced.
        depositGold, // (worker only) Carry gold that has been mined to a treasury.
        attackObject, // Do damage to an attackableObject withing range, if not in range begin maneuvering.
        findHome, // (fighters only) Try to find a "home" tile in a dormitory somewhere where the creature can sleep.
        findHomeForced, // Try to find a "home" tile in the dormitory where the creature is
        sleep, // (fighters only) Try to go to its home tile to and sleep when it gets there.
        jobdecided, // (fighters only) Check to see if our seat controls a room where we can work (train, forge, search, ...)
        jobforced, // (fighters only)Check to see if we have been dropped on a room where we can work
        eatdecided, // (fighters only) Try to find a hatchery to eat
        eatforced, // (fighters only) Force eating if the creature is dropped in a hatchery
        flee, // If a fighter is weak (low hp) or a worker is attacked by a fighter, he will flee
        carryEntity, // (worker only) Carry an entity to a suitable building
        getFee, // (fighter only) Gets the creature fee
        idle // Stand around doing nothing.
    };

    CreatureAction(const ActionType actionType, GameEntity::ObjectType entityType = GameEntity::ObjectType::unknown, const std::string& entityName = "", Tile* tile = nullptr);

    inline const ActionType getType() const
    { return mActionType; }

    inline void increaseNbTurn()
    { ++mNbTurns; }

    inline int32_t getNbTurns() const
    { return mNbTurns; }

    inline const std::string& getEntityName() const
    { return mEntityName; }

    inline GameEntity::ObjectType getEntityType() const
    { return mEntityType; }

    inline Tile* getTile() const
    { return mTile; }

    std::string toString() const;

private:
    ActionType mActionType;
    //! We save the creature name, not the pointer because in creature action, most of the time, we want to keep a reference
    //! for some time (for example when walking towards an enemy to attack). But the creature might be dead when we reach it.
    //! The rule of thumb would be to not keep a creature pointer here
    GameEntity::ObjectType mEntityType;
    std::string mEntityName;
    Tile* mTile;
    int32_t mNbTurns;
};

#endif // CREATUREACTION_H
