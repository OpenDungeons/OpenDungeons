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

#include "RoomHatchery.h"

#include "RoomObject.h"
#include "Tile.h"
#include "GameMap.h"
#include "LogManager.h"

#include <Random.h>

RoomHatchery::RoomHatchery(GameMap* gameMap) :
    Room(gameMap),
    mSpawnChickenCooldown(0),
    mNbChickensEaten(0)
{
    mType = hatchery;
}

void RoomHatchery::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);
    mUnusedTiles.push_back(t);
}

void RoomHatchery::removeCoveredTile(Tile* tile)
{
    Room::removeCoveredTile(tile);

    for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
    {
        Tile* tmpTile = it->second;
        if(tmpTile == tile)
        {
            // A creature was hunting the chicken. We remove it. Removing the creature from the hatchery will
            // release the chicken. Then, we will remove it from the list of free chickens
            Creature* creature = it->first;
            creature->changeEatRoom(NULL);
            break;
        }
    }

    std::vector<Tile*>::iterator it;

    it = std::find(mChickensFree.begin(), mChickensFree.end(), tile);
    if(it != mChickensFree.end())
    {
        // There is a chicken associated with this tile. We remove it
        ++mNbChickensEaten;
        Tile* tile = *it;
        removeRoomObject(tile);
        mChickensFree.erase(it);
        return;
    }

    it = std::find(mUnusedTiles.begin(), mUnusedTiles.end(), tile);
    if(it != mUnusedTiles.end())
    {
        // This tile was not used, no need to do anything else
        mUnusedTiles.erase(it);
        return;
    }
}

void RoomHatchery::absorbRoom(Room *r)
{
    RoomHatchery* rd = static_cast<RoomHatchery*>(r);
    mNbChickensEaten += rd->mNbChickensEaten;
    mChickensFree.insert(mChickensFree.end(), rd->mChickensFree.begin(), rd->mChickensFree.end());
    rd->mChickensFree.clear();

    Room::absorbRoom(r);
}

RoomObject* RoomHatchery::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    ++mNbChickensEaten;
    OD_ASSERT_TRUE(mUnusedTiles.size() >= mNbChickensEaten);
    if(place == ActiveSpotPlace::activeSpotCenter)
    {
        // The tile where the chicken coop is cannot hold a chicken
        std::vector<Tile*>::iterator it = std::find(mUnusedTiles.begin(), mUnusedTiles.end(), tile);
        OD_ASSERT_TRUE(it != mUnusedTiles.end());
        if(it != mUnusedTiles.end())
            mUnusedTiles.erase(it);

        return loadRoomObject(getGameMap(), "ChickenCoop", tile);
    }
    return NULL;
}

void RoomHatchery::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    if(place == ActiveSpotPlace::activeSpotCenter)
    {
        // We remove the chicken coop
        removeRoomObject(tile);
    }

    // We try to remove one of the pending chickens. If there is not, we try to remove one free chicken. If there is
    // not, we remove a chicken hunted by a creature
    if(mNbChickensEaten > 0)
    {
        --mNbChickensEaten;
        return;
    }

    if(mChickensFree.size() == 0)
    {
        for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
        {
            Tile* tmpTile = it->second;
            // A creature is hunting a chicken. We remove it. Removing the creature from the hatchery will
            // release the chicken. Then, we will remove it from the list of free chickens
            Creature* creature = it->first;
            creature->changeEatRoom(NULL);
            break;
        }
    }

    std::vector<Tile*>::iterator it = std::find(mChickensFree.begin(), mChickensFree.end(), tile);
    if(it != mChickensFree.end())
    {
        Tile* tile = *it;
        removeRoomObject(tile);
        mChickensFree.erase(it);
        return;
    }

    // We should never come here
    OD_ASSERT_TRUE(false);
}

bool RoomHatchery::hasOpenCreatureSpot(Creature* c)
{
    // We accept all creatures as soon as there are free chickens
    return mChickensFree.size() > 0;
}

bool RoomHatchery::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    int index = Random::Uint(0, mChickensFree.size() - 1);
    Tile* tileChicken = mChickensFree[index];
    mChickensFree.erase(mChickensFree.begin() + index);
    mCreaturesChickens[creature] = tileChicken;

    return true;
}

void RoomHatchery::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreaturesChickens.count(c) > 0)
    {
        Tile* tileChicken = mCreaturesChickens[c];
        OD_ASSERT_TRUE(tileChicken != NULL);
        if(tileChicken == NULL)
            return;
        mChickensFree.push_back(tileChicken);
        mCreaturesChickens.erase(c);
    }
}

void RoomHatchery::moveChickens()
{
    for(std::vector<Tile*>::iterator it = mChickensFree.begin(); it != mChickensFree.end(); ++it)
    {
        Tile* tileChicken = *it;
        RoomObject* ro = getRoomObjectFromTile(tileChicken);
        OD_ASSERT_TRUE(ro != NULL);
        if(ro == NULL)
            continue;

        if(ro->isMoving())
            continue;

        int indexTile = Random::Uint(0, numCoveredTiles() - 1);
        Tile* tileDest = getCoveredTile(indexTile);
        Ogre::Real wantedX = static_cast<Ogre::Real>(tileDest->getX());
        Ogre::Real wantedY = static_cast<Ogre::Real>(tileDest->getY());
        ro->addDestination(wantedX, wantedY);
        ro->setAnimationState("Walk");
    }
    for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
    {
        Tile* tileChicken = it->second;
        RoomObject* ro = getRoomObjectFromTile(tileChicken);
        OD_ASSERT_TRUE(ro != NULL);
        if(ro == NULL)
            continue;

        if(ro->isMoving())
            continue;

        int indexTile = Random::Uint(0, numCoveredTiles() - 1);
        Tile* tileDest = getCoveredTile(indexTile);
        Ogre::Real wantedX = static_cast<Ogre::Real>(tileDest->getX());
        Ogre::Real wantedY = static_cast<Ogre::Real>(tileDest->getY());
        ro->addDestination(wantedX, wantedY);
        ro->setAnimationState("Walk");
    }
}

bool RoomHatchery::doUpkeep()
{
    if(!Room::doUpkeep())
        return false;

    if(mNbChickensEaten > 0)
    {
        // Chickens have been eaten. We check when we will spawn another one
        ++mSpawnChickenCooldown;
        if(mSpawnChickenCooldown >= 10)
        {
            mSpawnChickenCooldown = 0;
            Tile* tileChicken = mUnusedTiles[Random::Uint(0, mUnusedTiles.size() - 1)];
            --mNbChickensEaten;
            mChickensFree.push_back(tileChicken);
            RoomObject* chicken = loadRoomObject(getGameMap(), "Chicken", tileChicken);
            addRoomObject(tileChicken, chicken);
            chicken->createMesh();
        }
    }
    else
        mSpawnChickenCooldown = 0;

    moveChickens();

    std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin();
    while(it != mCreaturesChickens.end())
    {
        Creature* creature = it->first;
        Tile* tileChicken = it->second;
        Tile* tileCreature = creature->positionTile();
        if(tileCreature == NULL)
        {
            ++it;
            continue;
        }

        RoomObject* ro = getRoomObjectFromTile(tileChicken);
        OD_ASSERT_TRUE(ro != NULL);
        if(ro == NULL)
        {
            ++it;
            continue;
        }

        if (creature->getEatCooldown() > 0)
        {
            creature->setAnimationState("Idle");
            creature->setEatCooldown(creature->getEatCooldown() - 1);
            ++it;
            continue;
        }

        Ogre::Vector3 creaturePosition = creature->getPosition();
        Ogre::Vector3 chickenPosition = ro->getPosition();
        // If the creature is on the same tile as the chicken, it can eat it
        if(static_cast<int32_t>(creaturePosition.x) == static_cast<int32_t>(chickenPosition.x) &&
           static_cast<int32_t>(creaturePosition.y) == static_cast<int32_t>(chickenPosition.y))
        {
            creature->faceToward(chickenPosition.x, chickenPosition.y);
            creature->setAnimationState("Attack1", true, false);
            ++mNbChickensEaten;
            mUnusedTiles.push_back(tileChicken);
            removeRoomObject(tileChicken);
            // We remove the link between the creature and the chicken because
            // removeCreatureUsingRoom would expect the chicken to exist
            it = mCreaturesChickens.erase(it);
            creature->foodEaten(10);
            creature->changeEatRoom(NULL);
            creature->setEatCooldown(Random::Uint(3, 8));
        }
        else if(!creature->isMoving())
        {
            // Move to the chicken
            Ogre::Vector3 creaturePosition = creature->getPosition();
            creature->addDestination(chickenPosition.x, chickenPosition.y);
            creature->setAnimationState("Walk");
            ++it;
        }
        else
            ++it;
    }

    return true;
}

