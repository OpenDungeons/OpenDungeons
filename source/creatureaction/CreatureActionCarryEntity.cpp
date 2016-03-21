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

#include "creatureaction/CreatureActionCarryEntity.h"

#include "entities/Building.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

CreatureActionCarryEntity::CreatureActionCarryEntity(Creature& creature, GameEntity& entityToCarry, Building& buildingDest) :
    CreatureAction(creature),
    mEntityToCarry(&entityToCarry),
    mTileDest(nullptr),
    mBuildingDest(&buildingDest)
{
    mEntityToCarry->addGameEntityListener(this);
    mBuildingDest->addGameEntityListener(this);
    mEntityToCarry->setCarryLock(mCreature, true);
    mEntityToCarry->notifyEntityCarryOn(&mCreature);
    mCreature.carryEntity(mEntityToCarry);
    mTileDest = mBuildingDest->askSpotForCarriedEntity(mEntityToCarry);
    OD_LOG_INF("creature=" + mCreature.getName() + " is carrying " + mEntityToCarry->getName() + " to tile=" + Tile::displayAsString(mTileDest));
}

CreatureActionCarryEntity::~CreatureActionCarryEntity()
{
    if(mEntityToCarry != nullptr)
    {
        mEntityToCarry->removeGameEntityListener(this);
        mEntityToCarry->setCarryLock(mCreature, false);
        const Ogre::Vector3& pos = mCreature.getPosition();
        OD_LOG_INF("creature=" + mCreature.getName() + " is releasing carried " + mEntityToCarry->getName() + ", pos=" + Helper::toString(pos));
        mEntityToCarry->notifyEntityCarryOff(pos);
    }

    mCreature.releaseCarriedEntity();

    if(mBuildingDest != nullptr)
    {
        mBuildingDest->removeGameEntityListener(this);
        mBuildingDest->notifyCarryingStateChanged(&mCreature, mEntityToCarry);
    }
}

std::function<bool()> CreatureActionCarryEntity::action()
{
    return std::bind(&CreatureActionCarryEntity::handleCarryEntity,
        std::ref(mCreature), mEntityToCarry, mTileDest);
}

bool CreatureActionCarryEntity::handleCarryEntity(Creature& creature, GameEntity* entityToCarry, Tile* tileDest)
{
    if(tileDest == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        return false;
    }

    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(entityToCarry == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        return false;
    }

    // We check if we are on the entity position tile. If yes, we carry it to a building
    // that wants it (if any)
    if(myTile != tileDest)
    {
        if(!creature.setDestination(tileDest))
        {
            OD_LOG_ERR("creature=" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + ", tileDest=" + Tile::displayAsString(tileDest));
            creature.popAction();
        }
        return true;
    }

    // We are at the destination tile
    creature.popAction();
    return false;
}

std::string CreatureActionCarryEntity::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionCarryEntity::notifyDead(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        const Ogre::Vector3& pos = mCreature.getPosition();
        OD_LOG_INF("creature=" + mCreature.getName() + " is releasing carried " + mEntityToCarry->getName() + ", pos=" + Helper::toString(pos));
        mEntityToCarry->notifyEntityCarryOff(pos);
        mEntityToCarry = nullptr;
        return false;
    }
    if(entity == mBuildingDest)
    {
        mBuildingDest = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionCarryEntity::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        const Ogre::Vector3& pos = mCreature.getPosition();
        OD_LOG_INF("creature=" + mCreature.getName() + " is releasing carried " + mEntityToCarry->getName() + ", pos=" + Helper::toString(pos));
        mEntityToCarry->notifyEntityCarryOff(pos);
        mEntityToCarry = nullptr;
        return false;
    }
    if(entity == mBuildingDest)
    {
        mBuildingDest = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionCarryEntity::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntityToCarry)
    {
        mEntityToCarry->setCarryLock(mCreature, false);
        const Ogre::Vector3& pos = mCreature.getPosition();
        OD_LOG_INF("creature=" + mCreature.getName() + " is releasing carried " + mEntityToCarry->getName() + ", pos=" + Helper::toString(pos));
        mEntityToCarry->notifyEntityCarryOff(pos);
        mEntityToCarry = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionCarryEntity::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
