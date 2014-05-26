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

#include "Room.h"

#include "RoomDojo.h"
#include "RoomDungeonTemple.h"
#include "RoomForge.h"
#include "RoomPortal.h"
#include "RoomQuarters.h"
#include "RoomTreasury.h"
#include "RoomObject.h"

#include "Player.h"
#include "Tile.h"
#include "Creature.h"
#include "RenderRequest.h"
#include "GameMap.h"
#include "RenderManager.h"
#include "Seat.h"

#include <sstream>

Room::Room():
    mType(nullRoomType),
    mNumActiveSpots(0)
{
    setObjectType(GameEntity::room);
}

Room* Room::createRoom(RoomType nType, const std::vector<Tile*>& nCoveredTiles, int nColor)
{
    Room* tempRoom = NULL;

    switch (nType)
    {
    default:
    case nullRoomType:
        tempRoom = NULL;
        break;
    case quarters:
        tempRoom = new RoomQuarters();
        break;
    case treasury:
        tempRoom = new RoomTreasury();
        break;
    case portal:
        tempRoom = new RoomPortal();
        break;
    case dungeonTemple:
        tempRoom = new RoomDungeonTemple();
        break;
    case forge:
        tempRoom = new RoomForge();
        break;
    case dojo:
        tempRoom = new RoomDojo();
        break;
    }

    if (tempRoom == NULL)
    {
        std::cerr << "ERROR: Trying to create a room of unknown type." << std::endl
                  << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__
                  << std::endl;
        return tempRoom;
    }

    tempRoom->setMeshExisting(false);
    tempRoom->setColor(nColor);

    //TODO: This should actually just call setType() but this will require a change to the >> operator.
    tempRoom->setMeshName(getMeshNameFromRoomType(nType));
    tempRoom->mType = nType;

    static int uniqueNumber = 0;
    std::stringstream tempSS;

    tempSS.str("");
    tempSS << tempRoom->getMeshName() << "_" << --uniqueNumber;
    tempRoom->setName(tempSS.str());

    for (unsigned int i = 0; i < nCoveredTiles.size(); ++i)
        tempRoom->addCoveredTile(nCoveredTiles[i]);

    tempRoom->updateActiveSpots();

    return tempRoom;
}

Room* Room::buildRoom(GameMap* gameMap, Room::RoomType nType, const std::vector< Tile* >& coveredTiles,
                      Player* player, bool inEditor)
{
    int goldRequired = coveredTiles.size() * Room::costPerTile(nType);
    Room* newRoom = NULL;

    if(!inEditor && player->getSeat()->getGold() < goldRequired)
        return newRoom;

    newRoom = createRoom(nType, coveredTiles, player->getSeat()->getColor());
    gameMap->addRoom(newRoom);

    if(!inEditor)
        gameMap->withdrawFromTreasuries(goldRequired, player->getSeat()->getColor());

    // Check all the tiles that border the newly created room and see if they
    // contain rooms which can be absorbed into this newly created room.
    std::vector<Tile*> borderTiles = gameMap->tilesBorderedByRegion(coveredTiles);
    for (unsigned int i = 0; i < borderTiles.size(); ++i)
    {
        Room* borderingRoom = borderTiles[i]->getCoveringRoom();
        if (borderingRoom != NULL && borderingRoom->getType() == newRoom->getType()
            && borderingRoom != newRoom)
        {
            newRoom->absorbRoom(borderingRoom);
            gameMap->removeRoom(borderingRoom);
            borderingRoom->deleteYourself();
        }
    }

    newRoom->updateActiveSpots();

    newRoom->createMesh();

    SoundEffectsHelper::getSingleton().playInterfaceSound(SoundEffectsHelper::BUILDROOM, false);

    return newRoom;
}

void Room::absorbRoom(Room *r)
{
    while (r->numCoveredTiles() > 0)
    {
        Tile *tempTile = r->getCoveredTile(0);
        r->removeCoveredTile(tempTile);
        addCoveredTile(tempTile);
    }

    mRoomObjects.insert(r->mRoomObjects.begin(), r->mRoomObjects.end());
    r->mRoomObjects.clear();

    updateActiveSpots();
}

void Room::addCoveredTile(Tile* t, double nHP)
{
    mCoveredTiles.push_back(t);
    mTileHP[t] = nHP;
    t->setCoveringRoom(this);
}

void Room::removeCoveredTile(Tile* t)
{
    bool removedTile = false;
    for (unsigned int i = 0; i < mCoveredTiles.size(); ++i)
    {
        if (t == mCoveredTiles[i])
        {
            mCoveredTiles.erase(mCoveredTiles.begin() + i);
            t->setCoveringRoom(NULL);
            mTileHP.erase(t);
            removedTile = true;
            break;
        }
    }

    if (!removedTile)
        return;

    // Destroy the mesh for this tile.
    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyRoom;
    request->p = this;
    request->p2 = t;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);

    updateActiveSpots();
}

Tile* Room::getCoveredTile(unsigned index)
{
    if (index >= mCoveredTiles.size())
        return NULL;

    return mCoveredTiles[index];
}

void Room::addCreatureUsingRoom(Creature* c)
{
    //FIXME: When the room is destroyed, any creatures holding pointers to this room
    // should be notified so they can purge them.
    // This is a somewhat non-trivial task.
    mCreaturesUsingRoom.push_back(c);
}

void Room::removeCreatureUsingRoom(Creature *c)
{
    for (unsigned int i = 0; i < mCreaturesUsingRoom.size(); ++i)
    {
        if (mCreaturesUsingRoom[i] == c)
        {
            mCreaturesUsingRoom.erase(mCreaturesUsingRoom.begin() + i);
            break;
        }
    }
}

Creature* Room::getCreatureUsingRoom(unsigned index)
{
    if (index >= mCreaturesUsingRoom.size())
        return NULL;

    return mCreaturesUsingRoom[index];
}

Tile* Room::getCentralTile()
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

RoomObject* Room::loadRoomObject(const std::string& meshName, Tile* targetTile,
                                 double rotationAngle)
{
    // TODO - proper random distrubition of room objects
    if (targetTile == NULL)
        targetTile = getCentralTile();

    return loadRoomObject(meshName, targetTile, targetTile->x, targetTile->y,
            rotationAngle);
}

RoomObject* Room::loadRoomObject(const std::string& meshName, Tile* targetTile,
                                 double x, double y, double rotationAngle)
{
    RoomObject* tempRoomObject = new RoomObject(this, meshName);
    mRoomObjects[targetTile] = tempRoomObject;
    tempRoomObject->mX = (Ogre::Real)x;
    tempRoomObject->mY = (Ogre::Real)y;
    tempRoomObject->mRotationAngle = (Ogre::Real)rotationAngle;

    return tempRoomObject;
}

void Room::createRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and create each mesh individually.
    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        itr->second->createMesh();
        ++itr;
    }
}

void Room::destroyRoomObjectMeshes()
{
    // Loop over all the RoomObjects that are children of this room and destroy each mesh individually.
    std::map<Tile*, RoomObject*>::iterator itr = mRoomObjects.begin();
    while (itr != mRoomObjects.end())
    {
        itr->second->destroyMesh();
        ++itr;
    }
}

std::string Room::getFormat()
{
    return "meshName\tcolor\t\tNextLine: numTiles\t\tSubsequent Lines: tileX\ttileY";
}

bool Room::doUpkeep()
{
    // Loop over the tiles in Room r and remove any whose HP has dropped to zero.
    unsigned int i = 0;
    bool tileRemoved = false;
    while (i < mCoveredTiles.size())
    {
        if (mTileHP[mCoveredTiles[i]] <= 0.0)
        {
            removeCoveredTile(mCoveredTiles[i]);
            tileRemoved = true;
        }
        else
            ++i;
    }

    // updateActiveSpots
    if (tileRemoved)
        updateActiveSpots();

    //TODO: This could return false if the whole room has been destroyed and then the activeObject processor should destroy it.
    return true;
}

Room* Room::createRoomFromStream(const std::string& roomName, std::istream& is, GameMap* gameMap)
{
    Room tempRoom;
    tempRoom.setMeshName(roomName);
    tempRoom.setGameMap(gameMap);
    is >> &tempRoom;

    return createRoom(tempRoom.mType, tempRoom.mCoveredTiles, tempRoom.getColor());
}

std::istream& operator>>(std::istream& is, Room* r)
{
    assert(r);

    static int uniqueNumber = 0;
    int tilesToLoad, tempX, tempY;
    std::stringstream tempSS;

    int tempInt = 0;
    is >> tempInt;
    r->setColor(tempInt);

    tempSS.str("");
    tempSS << r->getMeshName() << "_" << ++uniqueNumber;
    r->setName(tempSS.str());

    is >> tilesToLoad;
    for (int i = 0; i < tilesToLoad; ++i)
    {
        is >> tempX >> tempY;
        Tile* tempTile = r->getGameMap()->getTile(tempX, tempY);
        if (tempTile != NULL)
            r->addCoveredTile(tempTile);
    }

    r->mType = Room::getRoomTypeFromMeshName(r->getMeshName());
    return is;
}

std::ostream& operator<<(std::ostream& os, Room *r)
{
    if (r == NULL)
        return os;

    os << r->getMeshName() << "\t" << r->getColor() << "\n";
    os << r->mCoveredTiles.size() << "\n";
    for (unsigned int i = 0; i < r->mCoveredTiles.size(); ++i)
    {
        Tile *tempTile = r->mCoveredTiles[i];
        os << tempTile->x << "\t" << tempTile->y << "\n";
    }

    return os;
}

const char* Room::getMeshNameFromRoomType(RoomType t)
{
    switch (t)
    {
    case nullRoomType:
        return "NullRoomType";

    case dungeonTemple:
        return "DungeonTemple";

    case quarters:
        return "Quarters";

    case treasury:
        return "Treasury";

    case portal:
        return "Portal";

    case forge:
        return "Forge";

    case dojo:
        return "Dojo";

    default:
        return "UnknownRoomType";
    }
}

Room::RoomType Room::getRoomTypeFromMeshName(const std::string& s)
{
    if (s.compare("DungeonTemple") == 0)
        return dungeonTemple;
    else if (s.compare("Quarters") == 0)
        return quarters;
    else if (s.compare("Treasury") == 0)
        return treasury;
    else if (s.compare("Portal") == 0)
        return portal;
    else if (s.compare("Forge") == 0)
        return forge;
    else if (s.compare("Dojo") == 0)
        return dojo;
    else
    {
        std::cerr << "\n\n\nERROR:  Trying to get room type from unknown mesh name, bailing out.\n";
        std::cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
        return nullRoomType;
    }
}

int Room::costPerTile(RoomType t)
{
    switch (t)
    {
    case nullRoomType:
        return 0;

    case dungeonTemple:
        return 0;

    case portal:
        return 0;

    case quarters:
        return 75;

    case treasury:
        return 25;

    case forge:
        return 150;

    case dojo:
        return 175;

    default:
        return 0;
    }
}

double Room::getHP(Tile *tile)
{
    //NOTE: This function is the same as Trap::getHP(), consider making a base class to inherit this from.
    if (tile != NULL)
        return mTileHP[tile];

    // If the tile give was NULL, we add the total HP of all the tiles in the room and return that.
    double total = 0.0;

    for(std::map<Tile*, double>::iterator itr = mTileHP.begin(), end = mTileHP.end();
        itr != end; ++itr)
    {
        total += itr->second;
    }

    return total;
}

void Room::takeDamage(double damage, Tile* tileTakingDamage)
{
    if (tileTakingDamage == NULL)
        return;

    mTileHP[tileTakingDamage] -= damage;
}

void Room::updateActiveSpots()
{
    mCentralActiveSpotTiles.clear();

    // Detect the centers of 3x3 squares tiles
    for(unsigned int i = 0, size = mCoveredTiles.size(); i < size; ++i)
    {
        bool foundTop = false;
        bool foundTopLeft = false;
        bool foundTopRight = false;
        bool foundLeft = false;
        bool foundRight = false;
        bool foundBottomLeft = false;
        bool foundBottomRight = false;
        bool foundBottom = false;
        Tile* tile = mCoveredTiles[i];
        int tileX = tile->getX();
        int tileY = tile->getY();

        // Check all other tiles to know whether we have one center tile.
        for(unsigned int j = 0; j < size; ++j)
        {
            // Don't check the tile itself
            if (tile == mCoveredTiles[j])
                continue;

            // Check whether the tile around the tile checked is already a center spot
            // as we can't have two center spots next to one another.
            bool foundSpotNextToTile = false;
            for(unsigned int k = 0; k < mCentralActiveSpotTiles.size(); ++k)
            {
                if (mCoveredTiles[j] == mCentralActiveSpotTiles[k])
                {
                    foundSpotNextToTile = true;
                    break;
                }
            }
            if (foundSpotNextToTile)
                continue;

            int tile2X = mCoveredTiles[j]->getX();
            int tile2Y = mCoveredTiles[j]->getY();

            if(tile2X == tileX - 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopLeft = true;
                else if (tile2Y == tileY)
                    foundLeft = true;
                else if (tile2Y == tileY - 1)
                    foundBottomLeft = true;
            }
            else if(tile2X == tileX)
            {
                if (tile2Y == tileY + 1)
                    foundTop = true;
                else if (tile2Y == tileY - 1)
                    foundBottom = true;
            }
            else if(tile2X == tileX + 1)
            {
                if (tile2Y == tileY + 1)
                    foundTopRight = true;
                else if (tile2Y == tileY)
                    foundRight = true;
                else if (tile2Y == tileY - 1)
                    foundBottomRight = true;
            }
        }

        // Check whether we found a tile at the center of others
        if (foundTop && foundTopLeft && foundTopRight && foundLeft && foundRight
                && foundBottomLeft && foundBottomRight && foundBottom)
        {
            mCentralActiveSpotTiles.push_back(tile);
        }
    }

    // Now that we've got the center tiles, we can test the tile around for walls.
    mLeftWallsActiveSpotTiles.clear();
    mRightWallsActiveSpotTiles.clear();
    mBottomWallsActiveSpotTiles.clear();
    mTopWallsActiveSpotTiles.clear();

    GameMap* gameMap = getGameMap();
    // The game map can be null at map load time.
    if (gameMap == NULL)
    {
        // Updates the number of active spots.
        mNumActiveSpots = mCentralActiveSpotTiles.size();
        return;
    }

    for (unsigned int i = 0, size = mCentralActiveSpotTiles.size(); i < size; ++i)
    {
        Tile* centerTile = mCentralActiveSpotTiles[i];
        if (centerTile == NULL)
            continue;

        // Test for walls around
        // Up
        Tile* testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 2);
        if (testTile != NULL && testTile->isWallClaimedForColor(getColor()))
        {
            Tile* topTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() + 1);
            if (topTile != NULL)
                mTopWallsActiveSpotTiles.push_back(topTile);
        }

        // Down
        testTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 2);
        if (testTile != NULL && testTile->isWallClaimedForColor(getColor()))
        {
            Tile* bottomTile = getGameMap()->getTile(centerTile->getX(), centerTile->getY() - 1);
            if (bottomTile != NULL)
                mBottomWallsActiveSpotTiles.push_back(bottomTile);
        }

        // Left
        testTile = getGameMap()->getTile(centerTile->getX() - 2, centerTile->getY());
        if (testTile != NULL && testTile->isWallClaimedForColor(getColor()))
        {
            Tile* leftTile = getGameMap()->getTile(centerTile->getX() - 1, centerTile->getY());
            if (leftTile != NULL)
                mLeftWallsActiveSpotTiles.push_back(leftTile);
        }

        // Right
        testTile = getGameMap()->getTile(centerTile->getX() + 2, centerTile->getY());
        if (testTile != NULL && testTile->isWallClaimedForColor(getColor()))
        {
            Tile* rightTile = getGameMap()->getTile(centerTile->getX() + 1, centerTile->getY());
            if (rightTile != NULL)
                mRightWallsActiveSpotTiles.push_back(rightTile);
        }
    }

    // Updates the number of active spots
    mNumActiveSpots = mCentralActiveSpotTiles.size()
                      + mLeftWallsActiveSpotTiles.size() + mRightWallsActiveSpotTiles.size()
                      + mTopWallsActiveSpotTiles.size() + mBottomWallsActiveSpotTiles.size();
}
