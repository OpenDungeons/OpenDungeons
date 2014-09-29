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
    mNbChickensEaten(0),
    mSpawnChickenCooldown(0)
{
    mType = hatchery;
}

void RoomHatchery::addCoveredTile(Tile* t, double nHP, bool isRoomAbsorb)
{
    Room::addCoveredTile(t, nHP, isRoomAbsorb);
    // We add the tile to the used list if it is not already. It could already be there
    // in case of room absorbtion
    if(!isRoomAbsorb)
        mUnusedTiles.push_back(t);
}

void RoomHatchery::removeCoveredTile(Tile* tile, bool isRoomAbsorb)
{
    Room::removeCoveredTile(tile, isRoomAbsorb);

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

    // If no room absorb, we should not come here
    OD_ASSERT_TRUE_MSG(isRoomAbsorb, Tile::displayAsString(tile));
}

void RoomHatchery::absorbRoom(Room *r)
{
    RoomHatchery* rd = static_cast<RoomHatchery*>(r);
    mNbChickensEaten += rd->mNbChickensEaten;
    rd->mNbChickensEaten = 0;
    mUnusedTiles.insert(mUnusedTiles.end(), rd->mUnusedTiles.begin(), rd->mUnusedTiles.end());
    rd->mUnusedTiles.clear();
    mChickensFree.insert(mChickensFree.end(), rd->mChickensFree.begin(), rd->mChickensFree.end());
    rd->mChickensFree.clear();
    mCreaturesChickens.insert(rd->mCreaturesChickens.begin(), rd->mCreaturesChickens.end());
    rd->mCreaturesChickens.clear();
    mNbChickensEaten += rd->mCreaturesChickens.size();
    rd->mCreaturesChickens.clear();

    Room::absorbRoom(r);
}

RoomObject* RoomHatchery::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    ++mNbChickensEaten;
    if(place == ActiveSpotPlace::activeSpotCenter)
    {
        // The tile where the chicken coop is cannot hold a chicken
        std::vector<Tile*>::iterator it = std::find(mUnusedTiles.begin(), mUnusedTiles.end(), tile);
        if(it != mUnusedTiles.end())
        {
            mUnusedTiles.erase(it);
            return loadRoomObject(getGameMap(), "ChickenCoop", tile);
        }

        // We check if a creature is hunting this chicken. If is is the case, we free the chicken
        for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
        {
            Tile* tmpTile = it->second;
            // A creature is hunting this chicken. We remove it. Removing the creature from the hatchery will
            // release the chicken. Then, we will remove it from the list of free chickens
            if(tmpTile == tile)
            {
                Creature* creature = it->first;
                creature->changeEatRoom(NULL);
                break;
            }
        }

        // We check if a chicken is associated with this tile
        it = std::find(mChickensFree.begin(), mChickensFree.end(), tile);
        if(it != mChickensFree.end())
        {
            Tile* tmpTile = *it;
            removeRoomObject(tmpTile);
            mChickensFree.erase(it);
            return loadRoomObject(getGameMap(), "ChickenCoop", tile);
        }

        OD_ASSERT_TRUE(false);
    }
    return NULL;
}

void RoomHatchery::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    // This tile can now be used to spawn chickens
    mUnusedTiles.push_back(tile);

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

    if(mChickensFree.empty())
    {
        for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
        {
            // A creature is hunting a chicken. We remove it. Removing the creature from the hatchery will
            // release the chicken. Then, we will remove it from the list of free chickens
            Creature* creature = it->first;
            creature->changeEatRoom(NULL);
            break;
        }
    }

    // We remove one of the free chickens
    std::vector<Tile*>::iterator it = mChickensFree.begin();
    if(it != mChickensFree.end())
    {
        Tile* tmpTile = *it;
        removeRoomObject(tmpTile);
        mChickensFree.erase(it);
        mUnusedTiles.push_back(tmpTile);
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

    // We check if the creature is already registered. This can happen if
    // a creature is eating during room absorbtion
    if(mCreaturesChickens.count(creature) > 0)
        return true;

    uint32_t index = Random::Uint(0, mChickensFree.size() - 1);
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

        handleMoveChicken(ro);
    }

    for(std::map<Creature*,Tile*>::iterator it = mCreaturesChickens.begin(); it != mCreaturesChickens.end(); ++it)
    {
        Tile* tileChicken = it->second;
        RoomObject* ro = getRoomObjectFromTile(tileChicken);
        OD_ASSERT_TRUE_MSG(ro != NULL, "tile=" + Tile::displayAsString(tileChicken));
        if(ro == NULL)
            continue;

        handleMoveChicken(ro);
    }
}

void RoomHatchery::handleMoveChicken(RoomObject* chicken)
{
    if(chicken->isMoving())
        return;

    // We might not move
    if(Random::Int(1,2) == 1)
    {
        chicken->setAnimationState("Pick");
        return;
    }

    Tile* tmpTile = chicken->getPositionTile();
    int posChickenX = tmpTile->getX();
    int posChickenY = tmpTile->getY();
    std::vector<Tile*> possibleTileMove;
    // We move chickens from 1 tile only to avoid slow creatures from running
    // for ages when the hatchery is big
    tmpTile = getGameMap()->getTile(posChickenX - 1, posChickenY);
    if((tmpTile != NULL) &&
       (std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tmpTile) != mCoveredTiles.end()))
    {
        possibleTileMove.push_back(tmpTile);
    }
    tmpTile = getGameMap()->getTile(posChickenX + 1, posChickenY);
    if((tmpTile != NULL) &&
       (std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tmpTile) != mCoveredTiles.end()))
    {
        possibleTileMove.push_back(tmpTile);
    }
    tmpTile = getGameMap()->getTile(posChickenX, posChickenY - 1);
    if((tmpTile != NULL) &&
       (std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tmpTile) != mCoveredTiles.end()))
    {
        possibleTileMove.push_back(tmpTile);
    }
    tmpTile = getGameMap()->getTile(posChickenX, posChickenY + 1);
    if((tmpTile != NULL) &&
       (std::find(mCoveredTiles.begin(), mCoveredTiles.end(), tmpTile) != mCoveredTiles.end()))
    {
        possibleTileMove.push_back(tmpTile);
    }

    OD_ASSERT_TRUE(!possibleTileMove.empty());
    if(possibleTileMove.empty())
        return;

    uint32_t indexTile = Random::Uint(0, possibleTileMove.size() - 1);
    Tile* tileDest = possibleTileMove[indexTile];
    Ogre::Real x = static_cast<Ogre::Real>(tileDest->getX());
    Ogre::Real y = static_cast<Ogre::Real>(tileDest->getY());

    chicken->addDestination(x, y);
    chicken->setAnimationState("Walk");
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
            uint32_t index = Random::Uint(0, mUnusedTiles.size() - 1);
            Tile* tileChicken = mUnusedTiles[index];
            mUnusedTiles.erase(mUnusedTiles.begin() + index);

            --mNbChickensEaten;
            mChickensFree.push_back(tileChicken);
            RoomObject* chicken = loadRoomObject(getGameMap(), "Chicken", tileChicken);
            addRoomObject(tileChicken, chicken);
            chicken->setMoveSpeed(0.4);
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
        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == NULL)
        {
            ++it;
            continue;
        }

        RoomObject* ro = getRoomObjectFromTile(tileChicken);
        OD_ASSERT_TRUE_MSG(ro != NULL, "tile=" + Tile::displayAsString(tileChicken));
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
            Ogre::Vector3 walkDirection = chickenPosition - creaturePosition;
            walkDirection.normalise();
            creature->setAnimationState("Attack1", true, &walkDirection);
            ++mNbChickensEaten;
            mUnusedTiles.push_back(tileChicken);
            removeRoomObject(tileChicken);

            // We remove the link between the creature and the chicken because
            // removeCreatureUsingRoom would expect the chicken to exist
            it = mCreaturesChickens.erase(it);
            creature->foodEaten(10);
            creature->changeEatRoom(NULL);
            creature->setEatCooldown(Random::Int(3, 8));
        }
        else if(!creature->isMoving())
        {
            // Move to the chicken
            creature->addDestination(chickenPosition.x, chickenPosition.y);
            creature->setAnimationState("Walk");
            ++it;
        }
        else
            ++it;
    }

    return true;
}

