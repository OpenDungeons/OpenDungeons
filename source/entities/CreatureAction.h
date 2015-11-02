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
class CreatureSkillData;

enum class CreatureActionType
{
    walkToTile, // Calculate a path to the tile and follow it each turn.
    fight, // When seeing enemy objects, the creature might decide to fight
    searchTileToDig, // (worker only) Searches a tile to dig
    digTile, // (worker only) Digs a tile
    searchGroundTileToClaim, // (worker only) Searches a ground tile to claim
    claimGroundTile, // (worker only) "Dance" on tile to change its color.
    searchWallTileToClaim, // (worker only) Searches a wall tile to claim
    claimWallTile, // (worker only) "Dance" around the tile to change its color and reinforce it.
    attackObject, // Do damage to an attackableObject within range, if not in range begin maneuvering.
    findHome, // (fighters only) Try to find a "home" tile in a dormitory somewhere where the creature can sleep.
    sleep, // (fighters only) Try to go to its home tile to and sleep when it gets there.
    job, // (fighters only) Check to see if our seat controls a room where we can work (train, workshop, forge, search, ...)
    eat, // (fighters only) Try to find a hatchery to eat
    flee, // If a fighter is weak (low hp) or a worker is attacked by a fighter, he will flee
    carryEntity, // (worker only) Carry an entity to a suitable building
    getFee, // (fighter only) Gets the creature fee
    leaveDungeon, // (fighter only) Try to go to the portal to leave the dungeon
    nb // Must be the last value of this enum
};

//! \brief A data structure to be used in the creature AI calculations.
class CreatureAction : public GameEntityListener
{
public:
    CreatureAction(Creature& creature, const CreatureActionType actionType, bool forcedAction, GameEntity* attackedEntity, Tile* tile, CreatureSkillData* creatureSkillData);
    virtual ~CreatureAction();

    inline const CreatureActionType getType() const
    { return mActionType; }

    inline bool getForcedAction() const
    { return mForcedAction; }

    inline void increaseNbTurn()
    { ++mNbTurns; }

    inline int32_t getNbTurns() const
    { return mNbTurns; }

    inline void increaseNbTurnActive()
    { ++mNbTurnsActive; }

    inline int32_t getNbTurnsActive() const
    { return mNbTurnsActive; }

    inline GameEntity* getAttackedEntity() const
    { return mAttackedEntity; }

    inline void clearNbTurnsActive()
    { mNbTurnsActive = 0; }

    inline Tile* getTile() const
    { return mTile; }

    inline CreatureSkillData* getCreatureSkillData() const
    { return mCreatureSkillData; }

    std::string getListenerName() const override;
    bool notifyDead(GameEntity* entity) override;
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override;
    bool notifyDropped(GameEntity* entity) override;

    static std::string toString(CreatureActionType actionType);

private:
    CreatureAction(const CreatureAction&) = delete;
    Creature& mCreature;
    CreatureActionType mActionType;
    bool mForcedAction;
    GameEntity* mAttackedEntity;
    Tile* mTile;
    CreatureSkillData* mCreatureSkillData;
    //! Number of turns the action is in the creature pending actions
    int32_t mNbTurns;
    //! Number of turns the action is the one active
    int32_t mNbTurnsActive;
};

#endif // CREATUREACTION_H
