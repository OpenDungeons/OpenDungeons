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

#include "Building.h"
#include "RoomObject.h"
#include "Tile.h"
#include "GameMap.h"

#include "LogManager.h"

const double Building::DEFAULT_TILE_HP = 10.0;

void Building::addRoomObject(Tile* targetTile, RoomObject* roomObject)
{
    if(roomObject == NULL)
        return;

    LogManager::getSingleton().logMessage("Adding game object " + roomObject->getName()
        + ",Building=" + getName() + ",MeshName=" + roomObject->getMeshName()
        + ",tile=" + Tile::displayAsString(targetTile));

    Ogre::Vector3 objPos(static_cast<Ogre::Real>(targetTile->x), static_cast<Ogre::Real>(targetTile->y), 0);
    roomObject->setPosition(objPos);
    mRoomObjects[targetTile] = roomObject;
    getGameMap()->addRoomObject(roomObject);
}

void Building::removeRoomObject(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return;

    RoomObject* roomObject = mRoomObjects[tile];
    LogManager::getSingleton().logMessage("Removing object " + roomObject->getName()
        + " in Building=" + getName());
    getGameMap()->removeRoomObject(roomObject);
    roomObject->deleteYourself();
    mRoomObjects.erase(tile);
}

void Building::removeRoomObject(RoomObject* roomObject)
{
    std::map<Tile*, RoomObject*>::iterator it;

    for (it = mRoomObjects.begin(); it != mRoomObjects.end(); ++it)
    {
        if(it->second == roomObject)
            break;
    }

    if(it != mRoomObjects.end())
    {
        LogManager::getSingleton().logMessage("Removing object " + roomObject->getName()
            + " in Building " + getName());
        getGameMap()->removeRoomObject(roomObject);
        roomObject->deleteYourself();
        mRoomObjects.erase(it);
    }
}

void Building::removeAllRoomObject()
{
    if(mRoomObjects.empty())
        return;

    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        RoomObject* roomObject = itr->second;
        getGameMap()->removeRoomObject(roomObject);
        roomObject->deleteYourself();
        ++itr;
    }
    mRoomObjects.clear();
}

RoomObject* Building::getRoomObjectFromTile(Tile* tile)
{
    if(mRoomObjects.count(tile) == 0)
        return NULL;

    RoomObject* tempRoomObject = mRoomObjects[tile];
    return tempRoomObject;
}

RoomObject* Building::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double rotationAngle)
{
    if (targetTile == NULL)
        targetTile = getCentralTile();

    OD_ASSERT_TRUE(targetTile != NULL);
    if(targetTile == NULL)
        return NULL;

    return loadRoomObject(gameMap, meshName, targetTile, static_cast<double>(targetTile->x),
        static_cast<double>(targetTile->y), rotationAngle);
}

RoomObject* Building::loadRoomObject(GameMap* gameMap, const std::string& meshName,
    Tile* targetTile, double x, double y, double rotationAngle)
{
    RoomObject* tempRoomObject = new RoomObject(gameMap, getName(), meshName);
    tempRoomObject->setPosition(Ogre::Vector3((Ogre::Real)x, (Ogre::Real)y, 0.0f));
    tempRoomObject->mRotationAngle = (Ogre::Real)rotationAngle;

    return tempRoomObject;
}

Tile* Building::getCentralTile()
{
    if (mCoveredTiles.empty())
        return NULL;

    int minX, maxX, minY, maxY;
    minX = maxX = mCoveredTiles[0]->getX();
    minY = maxY = mCoveredTiles[0]->getY();

    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        int tempX = mCoveredTiles[i]->getX();
        int tempY = mCoveredTiles[i]->getY();

        if (tempX < minX)
            minX = tempX;
        if (tempY < minY)
            minY = tempY;
        if (tempX > maxX)
            maxX = tempX;
        if (tempY > maxY)
            maxY = tempY;
    }

    return getGameMap()->getTile((minX + maxX) / 2, (minY + maxY) / 2);
}

void Building::addCoveredTile(Tile* t, double nHP)
{
    mCoveredTiles.push_back(t);
    mTileHP[t] = nHP;
}

bool Building::removeCoveredTile(Tile* t)
{
    for (std::vector<Tile*>::iterator it = mCoveredTiles.begin(); it != mCoveredTiles.end(); ++it)
    {
        if (t == *it)
        {
            mCoveredTiles.erase(it);
            mTileHP.erase(t);
            return true;
        }
    }
    return false;
}

Tile* Building::getCoveredTile(int index)
{
    return mCoveredTiles[index];
}

std::vector<Tile*> Building::getCoveredTiles()
{
    return mCoveredTiles;
}

unsigned int Building::numCoveredTiles()
{
    return mCoveredTiles.size();
}

void Building::clearCoveredTiles()
{
    mCoveredTiles.clear();
}

double Building::getHP(Tile *tile) const
{
    if (tile != NULL)
    {
        std::map<Tile*, double>::const_iterator tileSearched = mTileHP.find(tile);
        OD_ASSERT_TRUE(tileSearched != mTileHP.end());
        if(tileSearched == mTileHP.end())
            return 0.0;

        return tileSearched->second;
    }

    // If the tile given was NULL, we add the total HP of all the tiles in the room and return that.
    double total = 0.0;

    for(std::map<Tile*, double>::const_iterator itr = mTileHP.begin(), end = mTileHP.end();
        itr != end; ++itr)
    {
        total += itr->second;
    }

    return total;
}

double Building::getDefense() const
{
    return 0;
}

void Building::takeDamage(GameEntity* attacker, double damage, Tile *tileTakingDamage)
{
    mTileHP[tileTakingDamage] -= damage;

    GameMap* gameMap = getGameMap();
    if(!gameMap->isServerGameMap())
        return;

    Seat* seat = getSeat();
    if (seat == NULL)
        return;

    Player* player = gameMap->getPlayerBySeatId(seat->getId());
    if (player == NULL)
        return;

    // Tells the server game map the player is under attack.
    gameMap->playerIsFighting(player);
}

std::string Building::getNameTile(Tile* tile)
{
    return getName() + "_tile_" + Ogre::StringConverter::toString(tile->x)
        + "_" + Ogre::StringConverter::toString(tile->y);
}

bool Building::isAttackable() const
{
    if(getHP(NULL) <= 0.0)
        return false;

    return true;
}
